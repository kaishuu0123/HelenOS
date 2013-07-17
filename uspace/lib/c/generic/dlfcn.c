/*
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup rtld rtld
 * @brief
 * @{
 */ 
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <rtld/module.h>
#include <rtld/symbol.h>

void *dlopen(const char *path, int flag)
{
	module_t *m;

	if (runtime_env == NULL) {
		printf("Dynamic linker not set up -- initializing.\n");
		rtld_init_static();
	}

	printf("dlopen(\"%s\", %d)\n", path, flag);

	printf("module_find('%s')\n", path);
	m = module_find(path);
	if (m == NULL) {
		printf("NULL. module_load('%s')\n", path);
		m = module_load(path);
		printf("module_load_deps(m)\n");
		module_load_deps(m);
		/* Now relocate. */
		printf("module_process_relocs(m)\n");
		module_process_relocs(m);
	} else {
		printf("not NULL\n");
	}

	return (void *) m;
}

/*
 * @note Symbols with NULL values are not accounted for.
 */
void *dlsym(void *mod, const char *sym_name)
{
	elf_symbol_t *sd;
	module_t *sm;

	printf("dlsym(0x%lx, \"%s\")\n", (long)mod, sym_name);
	sd = symbol_bfs_find(sym_name, (module_t *) mod, &sm);
	if (sd != NULL) {
		return symbol_get_addr(sd, sm);
	}

	return NULL;
}

/** @}
 */
