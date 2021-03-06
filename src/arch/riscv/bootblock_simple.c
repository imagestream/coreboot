/*
 * This file is part of the coreboot project.
 *
 * Copyright 2014 Google Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#include <bootblock_common.h>
#include <arch/cache.h>
#include <arch/hlt.h>
#include <arch/stages.h>
#include <arch/exception.h>
#include <cbfs.h>
#include <console/console.h>

static int boot_cpu(void)
{
	/*
	 * FIXME: This is a stub for now. All non-boot CPUs should be
	 * waiting for an interrupt. We could move the chunk of assembly
	 * which puts them to sleep in here...
	 */
	return 1;
}

void main(void)
{
	const char *stage_name = CONFIG_CBFS_PREFIX"/romstage";
	void *entry = NULL;

	/* Globally disable MMU, caches, and branch prediction (these should
	 * be disabled by default on reset) */
	dcache_mmu_disable();

	/*
	 * Re-enable icache and branch prediction. MMU and dcache will be
	 * set up later.
	 *
	 * Note: If booting from USB, we need to disable branch prediction
	 * before copying from USB into RAM (FIXME: why?)
	 */

	if (boot_cpu()) {
		//bootblock_cpu_init();
		//bootblock_mainboard_init();
	}

#ifdef CONFIG_BOOTBLOCK_CONSOLE
	console_init();
	exception_init();
#endif

	entry = cbfs_load_stage(CBFS_DEFAULT_MEDIA, stage_name);

	printk(BIOS_SPEW, "stage_name %s, entry %p\n", stage_name, entry);

	if (entry) stage_exit(entry);
	hlt();
}
