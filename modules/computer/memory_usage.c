/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2007 L. A. F. Pereira <l@tia.mat.br>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, version 2 or later.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <string.h>
#include "devices.h"

GHashTable *memlabels = NULL;

void scan_memory_do(void)
{
    gchar **keys, *tmp, *tmp_label, *trans_val;
    static gint offset = -1;
    gint i;

    if (offset == -1) {
        /* gah. linux 2.4 adds three lines of data we don't need in
         * /proc/meminfo.
         * The lines look something like this:
         *  total: used: free: shared: buffers: cached:
         *  Mem: 3301101568 1523159040 1777942528 0 3514368 1450356736
         *  Swap: 0 0 0
         */
        gchar *os_kernel = module_call_method("computer::getOSKernel");
        if (os_kernel) {
            offset = strstr(os_kernel, "Linux 2.4") ? 3 : 0;
            g_free(os_kernel);
        } else {
            offset = 0;
        }
    }

    g_file_get_contents("/proc/meminfo", &meminfo, NULL, NULL);

    keys = g_strsplit(meminfo, "\n", 0);

    g_free(meminfo);
    g_free(lginterval);

    meminfo = g_strdup("");
    lginterval = g_strdup("");

    for (i = offset; keys[i]; i++) {
        gchar **newkeys = g_strsplit(keys[i], ":", 0);

        if (!newkeys[0]) {
            g_strfreev(newkeys);
            break;
        }

        g_strstrip(newkeys[0]);
        g_strstrip(newkeys[1]);

        /* try to find a localizable label */
        tmp = g_hash_table_lookup(memlabels, newkeys[0]);
        if (tmp)
            tmp_label = _(tmp);
        else
            tmp_label = ""; /* or newkeys[0] */
        /* although it doesn't matter... */

        if (strstr(newkeys[1], "kB")) {
            trans_val = g_strdup_printf("%d %s", atoi(newkeys[1]), _("KiB") );
        } else {
            trans_val = strdup(newkeys[1]);
        }

        moreinfo_add_with_prefix("DEV", newkeys[0], g_strdup(trans_val));

        tmp = g_strconcat(meminfo, newkeys[0], "=", trans_val, "|", tmp_label, "\n", NULL);
        g_free(meminfo);
        meminfo = tmp;

        g_free(trans_val);

        tmp = g_strconcat(lginterval,
                          "UpdateInterval$", newkeys[0], "=1000\n", NULL);
        g_free(lginterval);
        lginterval = tmp;

        g_strfreev(newkeys);
    }
    g_strfreev(keys);
}

void init_memory_labels(void)
{
    static const struct {
        char *proc_label;
        char *real_label;
    } proc2real[] = {
        /* https://docs.kernel.org/filesystems/proc.html */
        { "MemTotal",       N_("Total physical memory usable by the system") },
        { "MemFree",        N_("Free memory which is not used for anything at all") },
        { "SwapTotal",      N_("Virtual memory, total swap space available") },
        { "SwapFree",       N_("Free Virtual memory, the remaining swap space available") },
        { "SwapCached",     N_("Memory that is present within main memory, but also in the swapfile") },
        { "HighTotal",      N_("The HighTotal value can vary based on the type of kernel used") },
        { "HighFree",       N_("The total and free amount of memory, in kilobytes, that is not directly mapped into kernel space") },
        { "LowTotal",       N_("The LowTotal value can vary based on the type of kernel used") },
        { "LowFree",        N_("The total and free amount of memory, in kilobytes, that is directly mapped into kernel space") },
        { "MemAvailable",   N_("Available memory for allocation to any process, without swapping") },
        { "Buffers",        N_("Memory in buffer cache, so relatively temporary storage for raw disk blocks") },
        { "Cached",         N_("Memory in the page cache (diskcache and shared memory), including tmpfs and shmem but excludes SwapCached") },
        { "Active",         N_("Memory that has been used more recently and usually not swapped out or reclaimed") },
        { "Inactive",       N_("Memory that has not been used recently and can be swapped out or reclaimed") },
        { "Active(anon)",   N_("Anonymous memory that has been used more recently and usually not swapped out") },
        { "Inactive(anon)", N_("Anonymous memory that has not been used recently and can be swapped out") },
        { "Active(file)",   N_("Pagecache memory that has been used more recently and usually not reclaimed until needed") },
        { "Inactive(file)", N_("Pagecache memory that can be reclaimed without huge performance impact") },
        { "Unevictable",    N_("Unevictable pages can't be swapped out for a variety of reasons") },
        { "Mlocked",        N_("Pages locked to memory using the mlock() system call. Mlocked pages are also Unevictable") },
        { "Zswap",          N_("Memory consumed by the zswap backend (compressed size)") },
        { "Zswapped",       N_("Amount of anonymous memory stored in zswap (original size)") },
        { "Dirty",          N_("Memory waiting to be written back to disk") },
        { "Writeback",      N_(" Memory which is actively being written back to disk") },
        { "AnonPages",      N_("Non-file backed pages mapped into userspace page tables") },
        { "Mapped",         N_("Files which have been mmapped, such as libraries") },
        { "Shmem",          N_("Total memory used by shared memory (shmem) and tmpfs") },
        { "KReclaimable",   N_("Kernel allocations that the kernel will attempt to reclaim under memory pressure") },
        { "Slab",           N_("In-kernel data structures cache") },
        { "SReclaimable",   N_("Part of Slab, that might be reclaimed, such as caches") },
        { "SUnreclaim",     N_("Part of Slab, that cannot be reclaimed on memory pressure") },
        { "KernelStack",    N_("Memory consumed by the kernel stacks of all tasks") },
        { "PageTables",     N_("Memory consumed by userspace page tables") },
        { "SecPageTables",  N_("Memory consumed by secondary page tables, this currently currently includes KVM mmu allocations on x86 and arm64") },
        { "NFS_Unstable",   N_("Always zero. Previous counted pages which had been written to the server, but has not been committed to stable storage") },
        { "Bounce",         N_("Memory used for block device bounce buffers") },
        { "WritebackTmp",   N_("Memory used by FUSE for temporary writeback buffers") },
        { "CommitLimit",    N_("Total amount of memory currently available to be allocated on the system") },
        { "Committed_AS",   N_("The amount of memory presently allocated on the system") },
        { "VmallocTotal",   N_("Total size of vmalloc virtual address space") },
        { "VmallocUsed",    N_("Amount of vmalloc area which is used") },
        { "VmallocChunk",   N_("Largest contiguous block of vmalloc area which is free") },
        { "Percpu",         N_("Memory allocated to the percpu allocator used to back percpu allocations. This stat excludes the cost of metadata") },
        { "HardwareCorrupted", N_("The amount of RAM/memory in KB, the kernel identifies as corrupted") },
        { "AnonHugePages",  N_("Non-file backed huge pages mapped into userspace page tables") },
        { "ShmemHugePages", N_("Memory used by shared memory (shmem) and tmpfs allocated with huge pages") },
        { "ShmemPmdMapped", N_("Shared memory mapped into userspace with huge pages") },
        { "FileHugePages",  N_("Memory used for filesystem data (page cache) allocated with huge pages") },
        { "FilePmdMapped",  N_("Page cache mapped into userspace with huge pages") },
        { "CmaTotal",       N_("Memory reserved for the Contiguous Memory Allocator (CMA)") },
        { "CmaFree",        N_("Free remaining memory in the CMA reserves") },
        { "Unaccepted",     N_("Amount of unaccepted memory usable by the kernel") },
        { "HugePages_Total", N_("Size of the pool of huge pages") },
        { "HugePages_Free", N_("Number of huge pages in the pool that are not yet allocated") },
        { "HugePages_Rsvd", N_("The number of huge pages for which a commitment to allocate from the pool has been made, but no allocation has yet been made") },
        { "HugePages_Surp", N_("Number of huge pages in the pool above the persistent huge pages in the kernel huge page pool") },
        { "Hugepagesize",   N_("Huge page size, used in conjunction with hugepages parameter to preallocate a number of huge pages of the specified size") },
        { "Hugetlb",        N_("Total amount of memory consumed by huge pages of all sizes") },
        { "DirectMap4k",    N_("Breakdown of page table sizes of 4k size used in the kernel identity mapping of RAM") },
        { "DirectMap2M",    N_("Breakdown of page table sizes of 2M size used in the kernel identity mapping of RAM") },
        { "DirectMap1G",    N_("Breakdown of page table sizes of 1G size used in the kernel identity mapping of RAM") },
        { NULL },
    };
    gint i;

    memlabels = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; proc2real[i].proc_label; i++) {
        g_hash_table_insert(memlabels, proc2real[i].proc_label,
            _(proc2real[i].real_label));
    }
}
