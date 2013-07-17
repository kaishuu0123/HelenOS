/*
 * Copyright (c) 2011 Maurizio Lombardi
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

/** @addtogroup fs
 * @{
 */

#include <stdlib.h>
#include <fibril_synch.h>
#include <align.h>
#include <adt/hash_table.h>
#include "mfs.h"

#define OPEN_NODES_KEYS 2
#define OPEN_NODES_SERVICE_KEY 0
#define OPEN_NODES_INODE_KEY 1
#define OPEN_NODES_BUCKETS 256

static bool check_magic_number(uint16_t magic, bool *native,
    mfs_version_t *version, bool *longfilenames);
static int mfs_node_core_get(fs_node_t **rfn, struct mfs_instance *inst,
    fs_index_t index);
static int mfs_node_put(fs_node_t *fsnode);
static int mfs_node_open(fs_node_t *fsnode);
static fs_index_t mfs_index_get(fs_node_t *fsnode);
static unsigned mfs_lnkcnt_get(fs_node_t *fsnode);
static bool mfs_is_directory(fs_node_t *fsnode);
static bool mfs_is_file(fs_node_t *fsnode);
static int mfs_has_children(bool *has_children, fs_node_t *fsnode);
static int mfs_root_get(fs_node_t **rfn, service_id_t service_id);
static service_id_t mfs_service_get(fs_node_t *fsnode);
static aoff64_t mfs_size_get(fs_node_t *node);
static int mfs_match(fs_node_t **rfn, fs_node_t *pfn, const char *component);
static int mfs_create_node(fs_node_t **rfn, service_id_t service_id, int flags);
static int mfs_link(fs_node_t *pfn, fs_node_t *cfn, const char *name);
static int mfs_unlink(fs_node_t *, fs_node_t *, const char *name);
static int mfs_destroy_node(fs_node_t *fn);
static hash_index_t open_nodes_hash(unsigned long key[]);
static int open_nodes_compare(unsigned long key[], hash_count_t keys,
    link_t *item);
static void open_nodes_remove_cb(link_t *link);
static int mfs_node_get(fs_node_t **rfn, service_id_t service_id,
    fs_index_t index);
static int mfs_instance_get(service_id_t service_id,
    struct mfs_instance **instance);
static int mfs_check_sanity(struct mfs_sb_info *sbi);
static bool is_power_of_two(uint32_t n);

static hash_table_t open_nodes;
static FIBRIL_MUTEX_INITIALIZE(open_nodes_lock);

libfs_ops_t mfs_libfs_ops = {
	.size_get = mfs_size_get,
	.root_get = mfs_root_get,
	.service_get = mfs_service_get,
	.is_directory = mfs_is_directory,
	.is_file = mfs_is_file,
	.node_get = mfs_node_get,
	.node_put = mfs_node_put,
	.node_open = mfs_node_open,
	.index_get = mfs_index_get,
	.match = mfs_match,
	.create = mfs_create_node,
	.link = mfs_link,
	.unlink = mfs_unlink,
	.destroy = mfs_destroy_node,
	.has_children = mfs_has_children,
	.lnkcnt_get = mfs_lnkcnt_get
};

/* Hash table interface for open nodes hash table */
static hash_index_t
open_nodes_hash(unsigned long key[])
{
	/* TODO: This is very simple and probably can be improved */
	return key[OPEN_NODES_INODE_KEY] % OPEN_NODES_BUCKETS;
}

static int
open_nodes_compare(unsigned long key[], hash_count_t keys,
    link_t *item)
{
	struct mfs_node *mnode = hash_table_get_instance(item, struct mfs_node, link);
	assert(keys > 0);
	if (mnode->instance->service_id !=
	    ((service_id_t) key[OPEN_NODES_SERVICE_KEY])) {
		return false;
	}
	if (keys == 1) {
		return true;
	}
	assert(keys == 2);
	return (mnode->ino_i->index == key[OPEN_NODES_INODE_KEY]);
}

static void
open_nodes_remove_cb(link_t *link)
{
	/* We don't use remove callback for this hash table */
}

static hash_table_operations_t open_nodes_ops = {
	.hash = open_nodes_hash,
	.compare = open_nodes_compare,
	.remove_callback = open_nodes_remove_cb,
};

int
mfs_global_init(void)
{
	if (!hash_table_create(&open_nodes, OPEN_NODES_BUCKETS,
	    OPEN_NODES_KEYS, &open_nodes_ops)) {
		return ENOMEM;
	}
	return EOK;
}

static int
mfs_mounted(service_id_t service_id, const char *opts, fs_index_t *index,
    aoff64_t *size, unsigned *linkcnt)
{
	enum cache_mode cmode;
	struct mfs_superblock *sb = NULL;
	struct mfs3_superblock *sb3 = NULL;
	struct mfs_sb_info *sbi = NULL;
	struct mfs_instance *instance = NULL;
	bool native, longnames;
	mfs_version_t version;
	uint16_t magic;
	int rc;

	/* Check for option enabling write through. */
	if (str_cmp(opts, "wtcache") == 0)
		cmode = CACHE_MODE_WT;
	else
		cmode = CACHE_MODE_WB;

	/* initialize libblock */
	rc = block_init(EXCHANGE_SERIALIZE, service_id, 4096);
	if (rc != EOK)
		return rc;

	/* Allocate space for generic MFS superblock */
	sbi = malloc(sizeof(*sbi));
	if (!sbi) {
		rc = ENOMEM;
		goto out_error;
	}

	/* Allocate space for filesystem instance */
	instance = malloc(sizeof(*instance));
	if (!instance) {
		rc = ENOMEM;
		goto out_error;
	}

	sb = malloc(MFS_SUPERBLOCK_SIZE);
	if (!sb) {
		rc = ENOMEM;
		goto out_error;
	}

	/* Read the superblock */
	rc = block_read_direct(service_id, MFS_SUPERBLOCK << 1, 2, sb);
	if (rc != EOK)
		goto out_error;

	sb3 = (struct mfs3_superblock *) sb;

	if (check_magic_number(sb->s_magic, &native, &version, &longnames)) {
		/* This is a V1 or V2 Minix filesystem */
		magic = sb->s_magic;
	} else if (check_magic_number(sb3->s_magic, &native, &version, &longnames)) {
		/* This is a V3 Minix filesystem */
		magic = sb3->s_magic;
	} else {
		/* Not recognized */
		mfsdebug("magic number not recognized\n");
		rc = ENOTSUP;
		goto out_error;
	}

	mfsdebug("magic number recognized = %04x\n", magic);

	/* Fill superblock info structure */

	sbi->fs_version = version;
	sbi->long_names = longnames;
	sbi->native = native;
	sbi->magic = magic;
	sbi->isearch = 0;
	sbi->zsearch = 0;

	if (version == MFS_VERSION_V3) {
		sbi->ninodes = conv32(native, sb3->s_ninodes);
		sbi->ibmap_blocks = conv16(native, sb3->s_ibmap_blocks);
		sbi->zbmap_blocks = conv16(native, sb3->s_zbmap_blocks);
		sbi->firstdatazone = conv16(native, sb3->s_first_data_zone);
		sbi->log2_zone_size = conv16(native, sb3->s_log2_zone_size);
		sbi->max_file_size = conv32(native, sb3->s_max_file_size);
		sbi->nzones = conv32(native, sb3->s_nzones);
		sbi->block_size = conv16(native, sb3->s_block_size);
		sbi->ino_per_block = V3_INODES_PER_BLOCK(sbi->block_size);
		sbi->dirsize = MFS3_DIRSIZE;
		sbi->max_name_len = MFS3_MAX_NAME_LEN;
	} else {
		sbi->ninodes = conv16(native, sb->s_ninodes);
		sbi->ibmap_blocks = conv16(native, sb->s_ibmap_blocks);
		sbi->zbmap_blocks = conv16(native, sb->s_zbmap_blocks);
		sbi->firstdatazone = conv16(native, sb->s_first_data_zone);
		sbi->log2_zone_size = conv16(native, sb->s_log2_zone_size);
		sbi->max_file_size = conv32(native, sb->s_max_file_size);
		sbi->block_size = MFS_BLOCKSIZE;
		if (version == MFS_VERSION_V2) {
			sbi->nzones = conv32(native, sb->s_nzones2);
			sbi->ino_per_block = V2_INODES_PER_BLOCK;
		} else {
			sbi->nzones = conv16(native, sb->s_nzones);
			sbi->ino_per_block = V1_INODES_PER_BLOCK;
		}
		sbi->dirsize = longnames ? MFSL_DIRSIZE : MFS_DIRSIZE;
		sbi->max_name_len = longnames ? MFS_L_MAX_NAME_LEN :
		    MFS_MAX_NAME_LEN;
	}

	if (sbi->log2_zone_size != 0) {
		/* In MFS, file space is allocated per zones.
		 * Zones are a collection of consecutive blocks on disk.
		 *
		 * The current MFS implementation supports only filesystems
		 * where the size of a zone is equal to the
		 * size of a block.
		 */
		rc = ENOTSUP;
		goto out_error;
	}

	sbi->itable_off = 2 + sbi->ibmap_blocks + sbi->zbmap_blocks;
	if ((rc = mfs_check_sanity(sbi)) != EOK) {
		fprintf(stderr, "Filesystem corrupted, invalid superblock");
		goto out_error;
	}

	rc = block_cache_init(service_id, sbi->block_size, 0, cmode);
	if (rc != EOK) {
		mfsdebug("block cache initialization failed\n");
		rc = EINVAL;
		goto out_error;
	}

	/* Initialize the instance structure and remember it */
	instance->service_id = service_id;
	instance->sbi = sbi;
	instance->open_nodes_cnt = 0;
	rc = fs_instance_create(service_id, instance);
	if (rc != EOK) {
		block_cache_fini(service_id);
		mfsdebug("fs instance creation failed\n");
		goto out_error;
	}

	mfsdebug("mount successful\n");

	fs_node_t *fn;
	mfs_node_get(&fn, service_id, MFS_ROOT_INO);

	struct mfs_node *mroot = fn->data;

	*index = mroot->ino_i->index;
	*size = mroot->ino_i->i_size;
	*linkcnt = 1;

	free(sb);

	return mfs_node_put(fn);

out_error:
	block_fini(service_id);
	if (sb)
		free(sb);
	if (sbi)
		free(sbi);
	if(instance)
		free(instance);
	return rc;
}

static int
mfs_unmounted(service_id_t service_id)
{
	struct mfs_instance *inst;

	mfsdebug("%s()\n", __FUNCTION__);

	int r = mfs_instance_get(service_id, &inst);
	if (r != EOK)
		return r;

	if (inst->open_nodes_cnt != 0)
		return EBUSY;

	(void) block_cache_fini(service_id);
	block_fini(service_id);

	/* Remove and destroy the instance */
	(void) fs_instance_destroy(service_id);
	free(inst->sbi);
	free(inst);
	return EOK;
}

service_id_t
mfs_service_get(fs_node_t *fsnode)
{
	struct mfs_node *node = fsnode->data;
	return node->instance->service_id;
}

static int
mfs_create_node(fs_node_t **rfn, service_id_t service_id, int flags)
{
	int r;
	struct mfs_instance *inst;
	struct mfs_node *mnode;
	fs_node_t *fsnode;
	uint32_t inum;

	mfsdebug("%s()\n", __FUNCTION__);

	r = mfs_instance_get(service_id, &inst);
	if (r != EOK)
		return r;

	/* Alloc a new inode */
	r = mfs_alloc_inode(inst, &inum);
	if (r != EOK)
		return r;

	struct mfs_ino_info *ino_i;

	ino_i = malloc(sizeof(*ino_i));
	if (!ino_i) {
		r = ENOMEM;
		goto out_err;
	}

	mnode = malloc(sizeof(*mnode));
	if (!mnode) {
		r = ENOMEM;
		goto out_err_1;
	}

	fsnode = malloc(sizeof(fs_node_t));
	if (!fsnode) {
		r = ENOMEM;
		goto out_err_2;
	}

	if (flags & L_DIRECTORY) {
		ino_i->i_mode = S_IFDIR;
		ino_i->i_nlinks = 1; /* This accounts for the '.' dentry */
	} else
		ino_i->i_mode = S_IFREG;

	ino_i->i_uid = 0;
	ino_i->i_gid = 0;
	ino_i->i_size = 0;
	ino_i->i_atime = 0;
	ino_i->i_mtime = 0;
	ino_i->i_ctime = 0;

	memset(ino_i->i_dzone, 0, sizeof(uint32_t) * V2_NR_DIRECT_ZONES);
	memset(ino_i->i_izone, 0, sizeof(uint32_t) * V2_NR_INDIRECT_ZONES);

	mfsdebug("new node idx = %d\n", (int) inum);

	ino_i->index = inum;
	ino_i->dirty = true;
	mnode->ino_i = ino_i;
	mnode->instance = inst;
	mnode->refcnt = 1;

	link_initialize(&mnode->link);

	unsigned long key[] = {
		[OPEN_NODES_SERVICE_KEY] = inst->service_id,
		[OPEN_NODES_INODE_KEY] = inum,
	};

	fibril_mutex_lock(&open_nodes_lock);
	hash_table_insert(&open_nodes, key, &mnode->link);
	fibril_mutex_unlock(&open_nodes_lock);
	inst->open_nodes_cnt++;

	mnode->ino_i->dirty = true;

	fs_node_initialize(fsnode);
	fsnode->data = mnode;
	mnode->fsnode = fsnode;
	*rfn = fsnode;

	return EOK;

out_err_2:
	free(mnode);
out_err_1:
	free(ino_i);
out_err:
	return r;
}

static int
mfs_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	struct mfs_node *mnode = pfn->data;
	struct mfs_ino_info *ino_i = mnode->ino_i;
	struct mfs_dentry_info d_info;
	int r;

	mfsdebug("%s()\n", __FUNCTION__);

	if (!S_ISDIR(ino_i->i_mode))
		return ENOTDIR;

	struct mfs_sb_info *sbi = mnode->instance->sbi;
	const size_t comp_size = str_size(component);

	unsigned i;
	for (i = 0; i < mnode->ino_i->i_size / sbi->dirsize; ++i) {
		r = mfs_read_dentry(mnode, &d_info, i);
		if (r != EOK)
			return r;

		if (!d_info.d_inum) {
			/* This entry is not used */
			continue;
		}

		const size_t dentry_name_size = str_size(d_info.d_name);

		if (comp_size == dentry_name_size &&
		    !bcmp(component, d_info.d_name, dentry_name_size)) {
			/* Hit! */
			mfs_node_core_get(rfn, mnode->instance,
			    d_info.d_inum);
			goto found;
		}
	}
	*rfn = NULL;
found:
	return EOK;
}

static aoff64_t
mfs_size_get(fs_node_t *node)
{
	const struct mfs_node *mnode = node->data;
	return mnode->ino_i->i_size;
}

static int
mfs_node_get(fs_node_t **rfn, service_id_t service_id,
    fs_index_t index)
{
	int rc;
	struct mfs_instance *instance;

	mfsdebug("%s()\n", __FUNCTION__);

	rc = mfs_instance_get(service_id, &instance);
	if (rc != EOK)
		return rc;

	return mfs_node_core_get(rfn, instance, index);
}

static int
mfs_node_put(fs_node_t *fsnode)
{
	int rc = EOK;
	struct mfs_node *mnode = fsnode->data;

	mfsdebug("%s()\n", __FUNCTION__);

	fibril_mutex_lock(&open_nodes_lock);

	assert(mnode->refcnt > 0);
	mnode->refcnt--;
	if (mnode->refcnt == 0) {
		unsigned long key[] = {
			[OPEN_NODES_SERVICE_KEY] = mnode->instance->service_id,
			[OPEN_NODES_INODE_KEY] = mnode->ino_i->index
		};
		hash_table_remove(&open_nodes, key, OPEN_NODES_KEYS);
		assert(mnode->instance->open_nodes_cnt > 0);
		mnode->instance->open_nodes_cnt--;
		rc = mfs_put_inode(mnode);
		free(mnode->ino_i);
		free(mnode);
		free(fsnode);
	}

	fibril_mutex_unlock(&open_nodes_lock);
	return rc;
}

static int
mfs_node_open(fs_node_t *fsnode)
{
	/*
	 * Opening a file is stateless, nothing
	 * to be done here.
	 */
	return EOK;
}

static fs_index_t
mfs_index_get(fs_node_t *fsnode)
{
	struct mfs_node *mnode = fsnode->data;
	return mnode->ino_i->index;
}

static unsigned
mfs_lnkcnt_get(fs_node_t *fsnode)
{
	struct mfs_node *mnode = fsnode->data;

	mfsdebug("%s() %d\n", __FUNCTION__, mnode->ino_i->i_nlinks);

	if (S_ISDIR(mnode->ino_i->i_mode)) {
		if (mnode->ino_i->i_nlinks > 1)
			return 1;
		else
			return 0;
	} else
		return mnode->ino_i->i_nlinks;
}

static int
mfs_node_core_get(fs_node_t **rfn, struct mfs_instance *inst,
    fs_index_t index)
{
	fs_node_t *node = NULL;
	struct mfs_node *mnode = NULL;
	int rc;

	mfsdebug("%s()\n", __FUNCTION__);

	fibril_mutex_lock(&open_nodes_lock);

	/* Check if the node is not already open */
	unsigned long key[] = {
		[OPEN_NODES_SERVICE_KEY] = inst->service_id,
		[OPEN_NODES_INODE_KEY] = index,
	};
	link_t *already_open = hash_table_find(&open_nodes, key);

	if (already_open) {
		mnode = hash_table_get_instance(already_open, struct mfs_node, link);
		*rfn = mnode->fsnode;
		mnode->refcnt++;

		fibril_mutex_unlock(&open_nodes_lock);
		return EOK;
	}

	node = malloc(sizeof(fs_node_t));
	if (!node) {
		rc = ENOMEM;
		goto out_err;
	}

	fs_node_initialize(node);

	mnode = malloc(sizeof(*mnode));
	if (!mnode) {
		rc = ENOMEM;
		goto out_err;
	}

	struct mfs_ino_info *ino_i;

	rc = mfs_get_inode(inst, &ino_i, index);
	if (rc != EOK)
		goto out_err;

	ino_i->index = index;
	mnode->ino_i = ino_i;
	mnode->refcnt = 1;
	link_initialize(&mnode->link);

	mnode->instance = inst;
	node->data = mnode;
	mnode->fsnode = node;
	*rfn = node;

	hash_table_insert(&open_nodes, key, &mnode->link);
	inst->open_nodes_cnt++;

	fibril_mutex_unlock(&open_nodes_lock);

	return EOK;

out_err:
	if (node)
		free(node);
	if (mnode)
		free(mnode);
	fibril_mutex_unlock(&open_nodes_lock);
	return rc;
}

static bool
mfs_is_directory(fs_node_t *fsnode)
{
	const struct mfs_node *node = fsnode->data;
	return S_ISDIR(node->ino_i->i_mode);
}

static bool
mfs_is_file(fs_node_t *fsnode)
{
	struct mfs_node *node = fsnode->data;
	return S_ISREG(node->ino_i->i_mode);
}

static int
mfs_root_get(fs_node_t **rfn, service_id_t service_id)
{
	int rc = mfs_node_get(rfn, service_id, MFS_ROOT_INO);
	return rc;
}

static int
mfs_link(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	struct mfs_node *parent = pfn->data;
	struct mfs_node *child = cfn->data;
	struct mfs_sb_info *sbi = parent->instance->sbi;
	bool destroy_dentry = false;

	mfsdebug("%s()\n", __FUNCTION__);

	if (str_size(name) > sbi->max_name_len)
		return ENAMETOOLONG;

	int r = mfs_insert_dentry(parent, name, child->ino_i->index);
	if (r != EOK)
		return r;

	if (S_ISDIR(child->ino_i->i_mode)) {
		if (child->ino_i->i_nlinks != 1) {
			/* It's not possible to hardlink directories in MFS */
			destroy_dentry = true;
			r = EMLINK;
			goto exit;
		}
		r = mfs_insert_dentry(child, ".", child->ino_i->index);
		if (r != EOK) {
			destroy_dentry = true;
			goto exit;
		}

		r = mfs_insert_dentry(child, "..", parent->ino_i->index);
		if (r != EOK) {
			destroy_dentry = true;
			goto exit;
		}

		parent->ino_i->i_nlinks++;
		parent->ino_i->dirty = true;
	}

exit:
	if (destroy_dentry) {
		int r2 = mfs_remove_dentry(parent, name);
		if (r2 != EOK)
			r = r2;
	} else {
		child->ino_i->i_nlinks++;
		child->ino_i->dirty = true;
	}
	return r;
}

static int
mfs_unlink(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	struct mfs_node *parent = pfn->data;
	struct mfs_node *child = cfn->data;
	bool has_children;
	int r;

	mfsdebug("%s()\n", __FUNCTION__);

	if (!parent)
		return EBUSY;

	r = mfs_has_children(&has_children, cfn);
	if (r != EOK)
		return r;

	if (has_children)
		return ENOTEMPTY;

	r = mfs_remove_dentry(parent, name);
	if (r != EOK)
		return r;

	struct mfs_ino_info *chino = child->ino_i;

	assert(chino->i_nlinks >= 1);
	chino->i_nlinks--;
	mfsdebug("Links: %d\n", chino->i_nlinks);

	if (chino->i_nlinks <= 1 && S_ISDIR(chino->i_mode)) {
		/* The child directory will be destroyed, decrease the
		 * parent hard links counter.
		 */
		parent->ino_i->i_nlinks--;
		parent->ino_i->dirty = true;
	}

	chino->dirty = true;

	return r;
}

static int
mfs_has_children(bool *has_children, fs_node_t *fsnode)
{
	struct mfs_node *mnode = fsnode->data;
	struct mfs_sb_info *sbi = mnode->instance->sbi;
	int r;

	*has_children = false;

	if (!S_ISDIR(mnode->ino_i->i_mode))
		goto out;

	struct mfs_dentry_info d_info;

	/* The first two dentries are always . and .. */
	unsigned i;
	for (i = 2; i < mnode->ino_i->i_size / sbi->dirsize; ++i) {
		r = mfs_read_dentry(mnode, &d_info, i);
		if (r != EOK)
			return r;

		if (d_info.d_inum) {
			/* A valid entry has been found */
			*has_children = true;
			break;
		}
	}
out:

	return EOK;
}

static int
mfs_read(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *rbytes)
{
	int rc;
	fs_node_t *fn;

	rc = mfs_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	if (!fn)
		return ENOENT;

	struct mfs_node *mnode;
	struct mfs_ino_info *ino_i;
	size_t len, bytes = 0;
	ipc_callid_t callid;

	mnode = fn->data;
	ino_i = mnode->ino_i;

	if (!async_data_read_receive(&callid, &len)) {
		rc = EINVAL;
		goto out_error;
	}

	if (S_ISDIR(ino_i->i_mode)) {
		aoff64_t spos = pos;
		struct mfs_dentry_info d_info;
		struct mfs_sb_info *sbi = mnode->instance->sbi;

		if (pos < 2) {
			/* Skip the first two dentries ('.' and '..') */
			pos = 2;
		}

		for (; pos < mnode->ino_i->i_size / sbi->dirsize; ++pos) {
			rc = mfs_read_dentry(mnode, &d_info, pos);
			if (rc != EOK)
				goto out_error;

			if (d_info.d_inum) {
				/* Dentry found! */
				goto found;
			}
		}

		rc = mfs_node_put(fn);
		async_answer_0(callid, rc != EOK ? rc : ENOENT);
		return rc;
found:
		async_data_read_finalize(callid, d_info.d_name,
		    str_size(d_info.d_name) + 1);
		bytes = ((pos - spos) + 1);
	} else {
		struct mfs_sb_info *sbi = mnode->instance->sbi;

		if (pos >= (size_t) ino_i->i_size) {
			/* Trying to read beyond the end of file */
			bytes = 0;
			(void) async_data_read_finalize(callid, NULL, 0);
			goto out_success;
		}

		bytes = min(len, sbi->block_size - pos % sbi->block_size);
		bytes = min(bytes, ino_i->i_size - pos);

		uint32_t zone;
		block_t *b;

		rc = mfs_read_map(&zone, mnode, pos);
		if (rc != EOK)
			goto out_error;

		if (zone == 0) {
			/* sparse file */
			uint8_t *buf = malloc(sbi->block_size);
			if (!buf) {
				rc = ENOMEM;
				goto out_error;
			}
			memset(buf, 0, sizeof(sbi->block_size));
			async_data_read_finalize(callid,
			    buf + pos % sbi->block_size, bytes);
			free(buf);
			goto out_success;
		}

		rc = block_get(&b, service_id, zone, BLOCK_FLAGS_NONE);
		if (rc != EOK)
			goto out_error;

		async_data_read_finalize(callid, b->data +
		    pos % sbi->block_size, bytes);

		rc = block_put(b);
		if (rc != EOK) {
			mfs_node_put(fn);
			return rc;
		}
	}
out_success:
	rc = mfs_node_put(fn);
	*rbytes = bytes;
	return rc;
out_error:
	;
	int tmp = mfs_node_put(fn);
	async_answer_0(callid, tmp != EOK ? tmp : rc);
	return tmp != EOK ? tmp : rc;
}

static int
mfs_write(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *wbytes, aoff64_t *nsize)
{
	fs_node_t *fn;
	int r;
	int flags = BLOCK_FLAGS_NONE;

	r = mfs_node_get(&fn, service_id, index);
	if (r != EOK)
		return r;
	if (!fn)
		return ENOENT;

	ipc_callid_t callid;
	size_t len;

	if (!async_data_write_receive(&callid, &len)) {
		r = EINVAL;
		goto out_err;
	}

	struct mfs_node *mnode = fn->data;
	struct mfs_sb_info *sbi = mnode->instance->sbi;
	struct mfs_ino_info *ino_i = mnode->ino_i;
	const size_t bs = sbi->block_size;
	size_t bytes = min(len, bs - (pos % bs));
	uint32_t block;

	if (bytes == bs)
		flags = BLOCK_FLAGS_NOREAD;

	r = mfs_read_map(&block, mnode, pos);
	if (r != EOK)
		goto out_err;

	if (block == 0) {
		uint32_t dummy;

		r = mfs_alloc_zone(mnode->instance, &block);
		if (r != EOK)
			goto out_err;
		
		r = mfs_write_map(mnode, pos, block, &dummy);
		if (r != EOK)
			goto out_err;

		flags = BLOCK_FLAGS_NOREAD;
	}

	block_t *b;
	r = block_get(&b, service_id, block, flags);
	if (r != EOK)
		goto out_err;

	if (flags == BLOCK_FLAGS_NOREAD)
		memset(b->data, 0, sbi->block_size);

	async_data_write_finalize(callid, b->data + (pos % bs), bytes);
	b->dirty = true;

	r = block_put(b);
	if (r != EOK) {
		mfs_node_put(fn);
		return r;
	}

	if (pos + bytes > ino_i->i_size) {
		ino_i->i_size = pos + bytes;
		ino_i->dirty = true;
	}
	r = mfs_node_put(fn);
	*nsize = ino_i->i_size;
	*wbytes = bytes;
	return r;

out_err:
	mfs_node_put(fn);
	async_answer_0(callid, r);
	return r;
}

static int
mfs_destroy(service_id_t service_id, fs_index_t index)
{
	fs_node_t *fn;
	int r;

	r = mfs_node_get(&fn, service_id, index);
	if (r != EOK)
		return r;
	if (!fn)
		return ENOENT;

	/* Destroy the inode */
	return mfs_destroy_node(fn);
}

static int
mfs_destroy_node(fs_node_t *fn)
{
	struct mfs_node *mnode = fn->data;
	bool has_children;
	int r;

	mfsdebug("mfs_destroy_node %d\n", mnode->ino_i->index);

	r = mfs_has_children(&has_children, fn);
	if (r != EOK)
		goto out;

	assert(!has_children);

	/* Free the entire inode content */
	r = mfs_inode_shrink(mnode, mnode->ino_i->i_size);
	if (r != EOK)
		goto out;

	/* Mark the inode as free in the bitmap */
	r = mfs_free_inode(mnode->instance, mnode->ino_i->index);

out:
	mfs_node_put(fn);
	return r;
}

static int
mfs_truncate(service_id_t service_id, fs_index_t index, aoff64_t size)
{
	fs_node_t *fn;
	int r;

	r = mfs_node_get(&fn, service_id, index);
	if (r != EOK)
		return r;
	if (!fn)
		return r;

	struct mfs_node *mnode = fn->data;
	struct mfs_ino_info *ino_i = mnode->ino_i;

	if (ino_i->i_size == size)
		r = EOK;
	else
		r = mfs_inode_shrink(mnode, ino_i->i_size - size);

	mfs_node_put(fn);
	return r;
}

static int
mfs_instance_get(service_id_t service_id, struct mfs_instance **instance)
{
	void *data;
	int rc;

	rc = fs_instance_get(service_id, &data);
	if (rc == EOK)
		*instance = (struct mfs_instance *) data;
	else {
		mfsdebug("instance not found\n");
	}

	return rc;
}

static bool
check_magic_number(uint16_t magic, bool *native,
		mfs_version_t *version, bool *longfilenames)
{
	bool rc = true;
	*longfilenames = false;

	if (magic == MFS_MAGIC_V1 || magic == MFS_MAGIC_V1R) {
		*native = magic == MFS_MAGIC_V1;
		*version = MFS_VERSION_V1;
	} else if (magic == MFS_MAGIC_V1L || magic == MFS_MAGIC_V1LR) {
		*native = magic == MFS_MAGIC_V1L;
		*version = MFS_VERSION_V1;
		*longfilenames = true;
	} else if (magic == MFS_MAGIC_V2 || magic == MFS_MAGIC_V2R) {
		*native = magic == MFS_MAGIC_V2;
		*version = MFS_VERSION_V2;
	} else if (magic == MFS_MAGIC_V2L || magic == MFS_MAGIC_V2LR) {
		*native = magic == MFS_MAGIC_V2L;
		*version = MFS_VERSION_V2;
		*longfilenames = true;
	} else if (magic == MFS_MAGIC_V3 || magic == MFS_MAGIC_V3R) {
		*native = magic == MFS_MAGIC_V3;
		*version = MFS_VERSION_V3;
	} else
		rc = false;

	return rc;
}

/** Filesystem sanity check
 *
 * @param Pointer to the MFS superblock.
 *
 * @return EOK on success, ENOTSUP otherwise.
 */
static int
mfs_check_sanity(struct mfs_sb_info *sbi)
{
	if (!is_power_of_two(sbi->block_size) ||
	    sbi->block_size < MFS_MIN_BLOCKSIZE ||
	    sbi->block_size > MFS_MAX_BLOCKSIZE)
		return ENOTSUP;
	else if (sbi->ibmap_blocks == 0 || sbi->zbmap_blocks == 0)
		return ENOTSUP;
	else if (sbi->ninodes == 0 || sbi->nzones == 0)
		return ENOTSUP;
	else if (sbi->firstdatazone == 0)
		return ENOTSUP;

	return EOK;
}

static int
mfs_close(service_id_t service_id, fs_index_t index)
{
	return 0;
}

static int
mfs_sync(service_id_t service_id, fs_index_t index)
{
	fs_node_t *fn;
	int rc = mfs_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	if (!fn)
		return ENOENT;

	struct mfs_node *mnode = fn->data;
	mnode->ino_i->dirty = true;

	return mfs_node_put(fn);
}

/** Check if a given number is a power of two.
 *
 * @param n	The number to check.
 *
 * @return	true if it is a power of two, false otherwise.
 */
static bool
is_power_of_two(uint32_t n)
{
	if (n == 0)
		return false;

	return (n & (n - 1)) == 0;
}

vfs_out_ops_t mfs_ops = {
	.mounted = mfs_mounted,
	.unmounted = mfs_unmounted,
	.read = mfs_read,
	.write = mfs_write,
	.truncate = mfs_truncate,
	.close = mfs_close,
	.destroy = mfs_destroy,
	.sync = mfs_sync,
};

/**
 * @}
 */

