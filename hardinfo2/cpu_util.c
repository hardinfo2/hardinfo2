/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 L. A. F. Pereira <l@tia.mat.br>
 *    This file by Burt P. <pburt0@gmail.com>
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
#include "hardinfo.h"
#include "cpu_util.h"

#define CPU_TOPO_NULL -9877

const gchar *byte_order_str() {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    return _("Little Endian");
#else
    return _("Big Endian");
#endif
}

int processor_has_flag(gchar * strflags, gchar * strflag)
{
    gchar **flags;
    gint ret = 0;
    if (strflags == NULL || strflag == NULL)
        return 0;
    flags = g_strsplit(strflags, " ", 0);
    ret = g_strv_contains((const gchar * const *)flags, strflag);
    g_strfreev(flags);
    return ret;
}

gchar* get_cpu_str(const gchar* file, gint cpuid) {
    gchar *tmp0 = NULL;
    gchar *tmp1 = NULL;
    tmp0 = g_strdup_printf("/sys/devices/system/cpu/cpu%d/%s", cpuid, file);
    g_file_get_contents(tmp0, &tmp1, NULL, NULL);
    g_free(tmp0);
    return tmp1;
}

gint get_cpu_int(const char* item, int cpuid, int null_val) {
    gchar *fc = NULL;
    int ret = null_val;
    fc = get_cpu_str(item, cpuid);
    if (fc) {
        ret = atol(fc);
        g_free(fc);
    }
    return ret;
}

#define MAX_BITS 65536
/* core_ids are not unique among physical_ids*/
#define MAX_CORES_PER_PACK 256
unsigned int cpubits_count(guint64 *base){
    int i=0,count=0;
    while(i<(MAX_BITS>>6)) {
       if(*(base+i)) {
	   int t=0;
	   while(t<64) {
	       if(*(base+i) & (1L<<t)) count++;
	       t++;
	   }
       }
       i++;
    }
    return count;
}
unsigned int count_from_str(gchar *str){
    int count=0;
    gchar *p=str,*np=NULL,*sp;
    while(p){
        np=strstr(p,",");
        if(np) *np=0;
        if(sp=strstr(p,"-")) {
	    *sp=0;
	    count+=atoi(sp+1)+1-atoi(p);
        } else {
	  count++;
        }
        if(np) p=np+1; else p=NULL;
    }
    return count;
}

static int cached=0;
static int cpu_cached[4];
int cpu_procs_cores_threads_nodes(int *p, int *c, int *t, int *n)
{
    guint64 *cores, *packs;
    char *tmp;
    int i, pack_id, core_id;

    if(cached){
        *p=cpu_cached[0];
        *c=cpu_cached[1];
        *t=cpu_cached[2];
        *n=cpu_cached[3];
	return 1;
    }

    *p = *c = *t = *n = 0;
    g_file_get_contents("/sys/devices/system/cpu/present", &tmp, NULL, NULL);
    if (!tmp) return 0;
    *t=count_from_str(tmp);
    g_free(tmp);

    cores = g_malloc0(MAX_BITS>>3);
    packs = g_malloc0(MAX_BITS>>3);
    for (i = 0; i < *t; i++) {
        pack_id = get_cpu_int("topology/physical_package_id", i, CPU_TOPO_NULL);
        if (pack_id < 0 || pack_id>=MAX_BITS) pack_id = 0;
        *(packs+(pack_id>>6)) |= (1L<<(pack_id%64));
        core_id = pack_id*MAX_CORES_PER_PACK + get_cpu_int("topology/core_id", i, CPU_TOPO_NULL);
	//printf("Thread=%3d PACKID=%3d COREID=%5d\n",i,pack_id,core_id);
        if (core_id < 0 || core_id>=MAX_BITS) core_id = 0;
	*(cores+(core_id>>6)) |= (1L<<(core_id%64));
    }

    *c = cpubits_count(cores);
//HACK: Arms cores are described different in topology, only Cortex-A65 is multithreaded so this fix is for 99%
#ifdef ARCH_arm
    *c = *t;
#endif
    *p = cpubits_count(packs);
    *n = 1;
    g_file_get_contents("/sys/devices/system/node/possible", &tmp, NULL, NULL);
    if (tmp) *n = count_from_str(tmp);
    g_free(tmp);

    //sanitity check
    if (*c<1) *c = *t; //if no cores, set to threads - probably SBC, best for benchmark
    if (*p<1) *p = 1;
    if (*n<1) *n = 1;

    g_free(cores);
    g_free(packs);
    //printf("CPU: %d %d %d %d\n",*p,*c,*t,*n);

    cpu_cached[0]=*p;
    cpu_cached[1]=*c;
    cpu_cached[2]=*t;
    cpu_cached[3]=*n;
    cached=1;

    return 1;
}

cpufreq_data *cpufreq_new(gint id)
{
    cpufreq_data *cpufd;
    cpufd = g_malloc(sizeof(cpufreq_data));
    if (cpufd) {
        memset(cpufd, 0, sizeof(cpufreq_data));
        cpufd->id = id;
        cpufreq_update(cpufd, 0);
    }
    return cpufd;
}

void cpufreq_update(cpufreq_data *cpufd, int cur_only)
{
    if (cpufd) {
        cpufd->cpukhz_cur = get_cpu_int("cpufreq/scaling_cur_freq", cpufd->id, 0);
        if (cur_only) return;
        cpufd->scaling_driver = get_cpu_str("cpufreq/scaling_driver", cpufd->id);
        cpufd->scaling_governor = get_cpu_str("cpufreq/scaling_governor", cpufd->id);
        cpufd->transition_latency = get_cpu_int("cpufreq/cpuinfo_transition_latency", cpufd->id, 0);
        cpufd->cpukhz_min = get_cpu_int("cpufreq/scaling_min_freq", cpufd->id, 0);
        cpufd->cpukhz_max = get_cpu_int("cpufreq/scaling_max_freq", cpufd->id, 0);
        if (cpufd->scaling_driver == NULL) cpufd->scaling_driver = g_strdup("(Unknown)");
        if (cpufd->scaling_governor == NULL) cpufd->scaling_governor = g_strdup("(Unknown)");

        /* x86 uses freqdomain_cpus, all others use affected_cpus */
        cpufd->shared_list = get_cpu_str("cpufreq/freqdomain_cpus", cpufd->id);
        if (cpufd->shared_list == NULL) cpufd->shared_list = get_cpu_str("cpufreq/affected_cpus", cpufd->id);
        if (cpufd->shared_list == NULL) cpufd->shared_list = g_strdup_printf("%d", cpufd->id);
    }
}

void cpufreq_free(cpufreq_data *cpufd)
{
    if (cpufd) {
        g_free(cpufd->scaling_driver);
        g_free(cpufd->scaling_governor);
        g_free(cpufd->shared_list);
    }
    g_free(cpufd);
}

cpu_topology_data *cputopo_new(gint id)
{
    cpu_topology_data *cputd;
    cputd = g_malloc(sizeof(cpu_topology_data));
    if (cputd) {
        memset(cputd, 0, sizeof(cpu_topology_data));
        cputd->id = id;
        cputd->socket_id = get_cpu_int("topology/physical_package_id", id, CPU_TOPO_NULL);
        cputd->core_id = get_cpu_int("topology/core_id", id, CPU_TOPO_NULL);
        cputd->book_id = get_cpu_int("topology/book_id", id, CPU_TOPO_NULL);
        cputd->drawer_id = get_cpu_int("topology/drawer_id", id, CPU_TOPO_NULL);
    }
    return cputd;

}

void cputopo_free(cpu_topology_data *cputd)
{
    g_free(cputd);
}

gchar *cpufreq_section_str(cpufreq_data *cpufd)
{
    if (cpufd == NULL)
        return g_strdup("");

    if (cpufd->cpukhz_min || cpufd->cpukhz_max || cpufd->cpukhz_cur) {
        return g_strdup_printf(
                    "[%s]\n"
                    "%s=%d %s\n"
                    "%s=%d %s\n"
                    "%s=%d %s\n"
                    "%s=%d %s\n"
                    "%s=%s\n"
                    "%s=%s\n",
                   _("Frequency Scaling"),
                   _("Minimum"), cpufd->cpukhz_min, _("kHz"),
                   _("Maximum"), cpufd->cpukhz_max, _("kHz"),
                   _("Current"), cpufd->cpukhz_cur, _("kHz"),
                   _("Transition Latency"), cpufd->transition_latency, _("ns"),
                   _("Governor"), cpufd->scaling_governor,
                   _("Driver"), cpufd->scaling_driver);
    } else {
        return g_strdup_printf(
                    "[%s]\n"
                    "%s=%s\n",
                   _("Frequency Scaling"),
                   _("Driver"), cpufd->scaling_driver);
    }
}

gchar *cputopo_section_str(cpu_topology_data *cputd)
{
    static const char na[] = N_("(Not Available)");
    char sock_str[64] = "", core_str[64] = "";
    char book_str[64] = "", drawer_str[64] = "";

    if (cputd == NULL)
        return g_strdup("");

    if (cputd->socket_id != CPU_TOPO_NULL && cputd->socket_id != -1)
        g_snprintf(sock_str, sizeof(sock_str), "%s=%d\n", _("Socket"), cputd->socket_id);
    else
        g_snprintf(sock_str, sizeof(sock_str), "%s=%s\n", _("Socket"), na);

    if (cputd->core_id != CPU_TOPO_NULL)
        g_snprintf(core_str, sizeof(core_str), "%s=%d\n", _("Core"), cputd->core_id);
    else
        g_snprintf(core_str, sizeof(core_str), "%s=%s\n", _("Core"), na);

    if (cputd->book_id != CPU_TOPO_NULL)
        g_snprintf(book_str, sizeof(book_str), "%s=%d\n", _("Book"), cputd->book_id);
    if (cputd->drawer_id != CPU_TOPO_NULL)
        g_snprintf(drawer_str, sizeof(drawer_str), "%s=%d\n", _("Drawer"), cputd->drawer_id);

    return g_strdup_printf(
                    "[%s]\n"
                    "%s=%d\n"
                    "%s%s%s%s",
                   _("Topology"),
                   _("ID"), cputd->id,
                   sock_str, core_str, book_str, drawer_str );
}
