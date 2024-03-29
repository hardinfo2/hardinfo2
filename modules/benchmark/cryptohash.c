/*
 *    HardInfo - System Information and Benchmark
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

#include "md5.h"
#include "sha1.h"
#include "benchmark.h"

/* if anything changes in this block, increment revision */
#define BENCH_REVISION 2
#define BENCH_DATA_SIZE 65536
#define CRUNCH_TIME 5
#define BENCH_DATA_MD5 "c25cf5c889f7bead2ff39788eedae37b"
#define STEPS 250

inline void md5_step(char *data, glong srclen)
{
    struct MD5Context ctx;
    guchar checksum[16];

    MD5Init(&ctx);
    MD5Update(&ctx, (guchar *)data, srclen);
    MD5Final(checksum, &ctx);
}

inline void sha1_step(char *data, glong srclen)
{
    SHA1_CTX ctx;
    guchar checksum[20];

    SHA1Init(&ctx);
    SHA1Update(&ctx, (guchar*)data, srclen);
    SHA1Final(checksum, &ctx);
}

static gpointer cryptohash_for(void *in_data, gint thread_number)
{
    unsigned int i;

    for (i = 0;i <= STEPS; i++) {
        if (i & 1) {
            md5_step(in_data, BENCH_DATA_SIZE);
        } else {
            sha1_step(in_data, BENCH_DATA_SIZE);
        }
    }

    return NULL;
}

void
benchmark_cryptohash(void)
{
    bench_value r = EMPTY_BENCH_VALUE;
    gchar *test_data = get_test_data(BENCH_DATA_SIZE);
    if (!test_data) return;

    shell_view_set_enabled(FALSE);
    shell_status_update("Running CryptoHash benchmark...");

    gchar *d = md5_digest_str(test_data, BENCH_DATA_SIZE);
    if (!SEQ(d, BENCH_DATA_MD5))
        bench_msg("test data has different md5sum: expected %s, actual %s", BENCH_DATA_MD5, d);

    r = benchmark_crunch_for(CRUNCH_TIME, 0, cryptohash_for, test_data);
    r.revision = BENCH_REVISION;
    snprintf(r.extra, 255, "r:%d, d:%s", STEPS, d);

    g_free(test_data);
    g_free(d);

    r.result /= 10;

    bench_results[BENCHMARK_CRYPTOHASH] = r;
}
