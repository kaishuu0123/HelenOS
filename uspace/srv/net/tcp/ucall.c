/*
 * Copyright (c) 2011 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup tcp
 * @{
 */

/**
 * @file TCP entry points (close to those defined in the RFC)
 */

#include <fibril_synch.h>
#include <io/log.h>
#include <macros.h>
#include <mem.h>
#include "conn.h"
#include "tcp_type.h"
#include "tqueue.h"
#include "ucall.h"

/*
 * User calls
 */

/** OPEN user call
 *
 * @param lsock		Local socket
 * @param fsock		Foreign socket
 * @param acpass	Active/passive
 * @param oflags	Open flags
 * @param conn		Connection
 *
 * Unlike in the spec we allow specifying the local address. This means
 * the implementation does not need to magically guess it, especially
 * considering there can be more than one local address.
 *
 * XXX We should be able to call active open on an existing listening
 * connection.
 * XXX We should be able to get connection structure immediately, before
 * establishment.
 */
tcp_error_t tcp_uc_open(tcp_sock_t *lsock, tcp_sock_t *fsock, acpass_t acpass,
    tcp_open_flags_t oflags, tcp_conn_t **conn)
{
	tcp_conn_t *nconn;

	log_msg(LVL_DEBUG, "tcp_uc_open(%p, %p, %s, %s, %p)",
	    lsock, fsock, acpass == ap_active ? "active" : "passive",
	    oflags == tcp_open_nonblock ? "nonblock" : "none", conn);

	nconn = tcp_conn_new(lsock, fsock);
	tcp_conn_add(nconn);

	if (acpass == ap_active) {
		/* Synchronize (initiate) connection */
		tcp_conn_sync(nconn);
	}

	if (oflags == tcp_open_nonblock) {
		*conn = nconn;
		return TCP_EOK;
	}

	/* Wait for connection to be established or reset */
	log_msg(LVL_DEBUG, "tcp_uc_open: Wait for connection.");
	fibril_mutex_lock(&nconn->lock);
	while (nconn->cstate == st_listen ||
	    nconn->cstate == st_syn_sent ||
	    nconn->cstate == st_syn_received) {
		fibril_condvar_wait(&nconn->cstate_cv, &nconn->lock);
	}

	if (nconn->cstate != st_established) {
		log_msg(LVL_DEBUG, "tcp_uc_open: Connection was reset.");
		assert(nconn->cstate == st_closed);
		fibril_mutex_unlock(&nconn->lock);
		return TCP_ERESET;
	}

	fibril_mutex_unlock(&nconn->lock);
	log_msg(LVL_DEBUG, "tcp_uc_open: Connection was established.");

	*conn = nconn;
	log_msg(LVL_DEBUG, "tcp_uc_open -> %p", nconn);
	return TCP_EOK;
}

/** SEND user call */
tcp_error_t tcp_uc_send(tcp_conn_t *conn, void *data, size_t size,
    xflags_t flags)
{
	size_t buf_free;
	size_t xfer_size;

	log_msg(LVL_DEBUG, "%s: tcp_uc_send()", conn->name);

	fibril_mutex_lock(&conn->lock);

	if (conn->cstate == st_closed) {
		fibril_mutex_unlock(&conn->lock);
		return TCP_ENOTEXIST;
	}

	if (conn->cstate == st_listen) {
		/* Change connection to active */
		tcp_conn_sync(conn);
	}


	if (conn->snd_buf_fin) {
		fibril_mutex_unlock(&conn->lock);
		return TCP_ECLOSING;
	}

	while (size > 0) {
		buf_free = conn->snd_buf_size - conn->snd_buf_used;
		while (buf_free == 0 && !conn->reset) {
			log_msg(LVL_DEBUG, "%s: buf_free == 0, waiting.",
			    conn->name);
			fibril_condvar_wait(&conn->snd_buf_cv, &conn->lock);
			buf_free = conn->snd_buf_size - conn->snd_buf_used;
		}

		if (conn->reset) {
			fibril_mutex_unlock(&conn->lock);
			return TCP_ERESET;
		}

		xfer_size = min(size, buf_free);

		/* Copy data to buffer */
		memcpy(conn->snd_buf + conn->snd_buf_used, data, xfer_size);
		data += xfer_size;
		conn->snd_buf_used += xfer_size;
		size -= xfer_size;

		tcp_tqueue_new_data(conn);
	}

	tcp_tqueue_new_data(conn);
	fibril_mutex_unlock(&conn->lock);

	return TCP_EOK;
}

/** RECEIVE user call */
tcp_error_t tcp_uc_receive(tcp_conn_t *conn, void *buf, size_t size,
    size_t *rcvd, xflags_t *xflags)
{
	size_t xfer_size;

	log_msg(LVL_DEBUG, "%s: tcp_uc_receive()", conn->name);

	fibril_mutex_lock(&conn->lock);

	if (conn->cstate == st_closed) {
		fibril_mutex_unlock(&conn->lock);
		return TCP_ENOTEXIST;
	}

	/* Wait for data to become available */
	while (conn->rcv_buf_used == 0 && !conn->rcv_buf_fin && !conn->reset) {
		log_msg(LVL_DEBUG, "tcp_uc_receive() - wait for data");
		fibril_condvar_wait(&conn->rcv_buf_cv, &conn->lock);
	}

	if (conn->rcv_buf_used == 0) {
		*rcvd = 0;
		*xflags = 0;

		if (conn->rcv_buf_fin) {
			/* End of data, peer closed connection */
			fibril_mutex_unlock(&conn->lock);
			return TCP_ECLOSING;
		} else {
			/* Connection was reset */
			assert(conn->reset);
			fibril_mutex_unlock(&conn->lock);
			return TCP_ERESET;
		}
	}

	/* Copy data from receive buffer to user buffer */
	xfer_size = min(size, conn->rcv_buf_used);
	memcpy(buf, conn->rcv_buf, xfer_size);
	*rcvd = xfer_size;

	/* Remove data from receive buffer */
	memmove(conn->rcv_buf, conn->rcv_buf + xfer_size, conn->rcv_buf_used -
	    xfer_size);
	conn->rcv_buf_used -= xfer_size;
	conn->rcv_wnd += xfer_size;

	/* TODO */
	*xflags = 0;

	/* Send new size of receive window */
	tcp_tqueue_ctrl_seg(conn, CTL_ACK);

	log_msg(LVL_DEBUG, "%s: tcp_uc_receive() - returning %zu bytes",
	    conn->name, xfer_size);

	fibril_mutex_unlock(&conn->lock);

	return TCP_EOK;
}

/** CLOSE user call */
tcp_error_t tcp_uc_close(tcp_conn_t *conn)
{
	log_msg(LVL_DEBUG, "%s: tcp_uc_close()", conn->name);

	fibril_mutex_lock(&conn->lock);

	if (conn->cstate == st_closed) {
		fibril_mutex_unlock(&conn->lock);
		return TCP_ENOTEXIST;
	}

	if (conn->snd_buf_fin) {
		fibril_mutex_unlock(&conn->lock);
		return TCP_ECLOSING;
	}

	conn->snd_buf_fin = true;
	tcp_tqueue_new_data(conn);

	fibril_mutex_unlock(&conn->lock);
	return TCP_EOK;
}

/** ABORT user call */
void tcp_uc_abort(tcp_conn_t *conn)
{
	log_msg(LVL_DEBUG, "tcp_uc_abort()");
}

/** STATUS user call */
void tcp_uc_status(tcp_conn_t *conn, tcp_conn_status_t *cstatus)
{
	log_msg(LVL_DEBUG, "tcp_uc_status()");
	cstatus->cstate = conn->cstate;
}

/** Delete connection user call.
 *
 * (Not in spec.) Inform TCP that the user is done with this connection
 * and will not make any further calls/references to it. TCP can deallocate
 * the connection from now on.
 */
void tcp_uc_delete(tcp_conn_t *conn)
{
	log_msg(LVL_DEBUG, "tcp_uc_delete()");
	tcp_conn_delete(conn);
}

void tcp_uc_set_cstate_cb(tcp_conn_t *conn, tcp_cstate_cb_t cb, void *arg)
{
	log_msg(LVL_DEBUG, "tcp_uc_set_ctate_cb(%p, %p, %p)",
	    conn, cb, arg);

	conn->cstate_cb = cb;
	conn->cstate_cb_arg = arg;
}

/*
 * Arriving segments
 */

/** Segment arrived */
void tcp_as_segment_arrived(tcp_sockpair_t *sp, tcp_segment_t *seg)
{
	tcp_conn_t *conn;

	log_msg(LVL_DEBUG, "tcp_as_segment_arrived(f:(%x,%u), l:(%x,%u))",
	    sp->foreign.addr.ipv4, sp->foreign.port,
	    sp->local.addr.ipv4, sp->local.port);

	conn = tcp_conn_find_ref(sp);
	if (conn == NULL) {
		log_msg(LVL_WARN, "No connection found.");
		tcp_unexpected_segment(sp, seg);
		return;
	}

	fibril_mutex_lock(&conn->lock);

	if (conn->cstate == st_closed) {
		log_msg(LVL_WARN, "Connection is closed.");
		tcp_unexpected_segment(sp, seg);
		fibril_mutex_unlock(&conn->lock);
		tcp_conn_delref(conn);
		return;
	}

	if (conn->ident.foreign.addr.ipv4 == TCP_IPV4_ANY)
		conn->ident.foreign.addr.ipv4 = sp->foreign.addr.ipv4;
	if (conn->ident.foreign.port == TCP_PORT_ANY)
		conn->ident.foreign.port = sp->foreign.port;
	if (conn->ident.local.addr.ipv4 == TCP_IPV4_ANY)
		conn->ident.local.addr.ipv4 = sp->local.addr.ipv4;

	tcp_conn_segment_arrived(conn, seg);

	fibril_mutex_unlock(&conn->lock);
	tcp_conn_delref(conn);
}

/*
 * Timeouts
 */

/** User timeout */
void tcp_to_user(void)
{
	log_msg(LVL_DEBUG, "tcp_to_user()");
}

/**
 * @}
 */
