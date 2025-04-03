/*
 *    hardinfo2 - System Information and Benchmark
 *    Copyright (C) 2025 hardinfo2 project
 *    License: GPL2+
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License v2.0 or later.
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

#include "hardinfo.h"
#include "benchmark.h"
#include <math.h>

/* if anything changes in this block, increment revision */
#define BENCH_REVISION 1

static bench_value vulkan_bench(int darkmode) {
    bench_value ret = EMPTY_BENCH_VALUE;
    gboolean spawned;
    gchar *out=NULL, *err=NULL;
    //int count, ms,gl;
    int ver;
    float fps;
    char *cmd_line;

    darkmode=0;//FIXME
    cmd_line=g_strdup_printf("%s/modules/vk1_gears %s",params.path_lib, (darkmode ? "-dark" : ""));

    spawned = g_spawn_command_line_sync(cmd_line, &out, &err, NULL, NULL);
    g_free(cmd_line);
    if (spawned && (sscanf(out,"Ver=%d, Result:%f\n", &ver, &fps)==2)) {
            strncpy(ret.extra, out, sizeof(ret.extra)-1);
	    ret.extra[sizeof(ret.extra)-1]=0;
            ret.threads_used = 1;
            ret.elapsed_time = 3;//(double)ms/1000;
	    ret.revision = (BENCH_REVISION*100) + ver;
            ret.result = fps;
    }
    g_free(out);
    g_free(err);

    return ret;
}

void benchmark_vulkan(void) {
    bench_value r = EMPTY_BENCH_VALUE;

    shell_view_set_enabled(FALSE);
    shell_status_update("Performing vulkan benchmark...");

    r = vulkan_bench((params.max_bench_results==1?1:0));

    bench_results[BENCHMARK_VULKAN] = r;
}
