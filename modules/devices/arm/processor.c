/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2006 L. A. F. Pereira <l@tia.mat.br>
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

#include "hardinfo.h"
#include "devices.h"
#include "cpu_util.h"
#include "dt_util.h"

#include "arm_data.h"
#include "arm_data.c"

enum {
    ARM_A32 = 0,
    ARM_A64 = 1,
    ARM_A32_ON_A64 = 2,
};

static const gchar *arm_mode_str[] = {
    "A32",
    "A64",
    "A32 on A64",
};

static gchar *__cache_get_info_as_string(GSList *cpu_cache)
{
    gchar *result = g_strdup("");
    ProcessorCache *cache;
    GSList *cache_list;

    if (!cpu_cache) {
        return g_strdup(_("Cache information not available=\n"));
    }

    for (cache_list = cpu_cache; cache_list; cache_list = cache_list->next) {
        cache = (ProcessorCache *)cache_list->data;

        result = h_strdup_cprintf(_("Level %d (%s)=%d-way set-associative, %d sets, %dKB size\n"),
                                  result,
                                  cache->level,
                                  C_("cache-type", cache->type),
                                  cache->ways_of_associativity,
                                  cache->number_of_sets,
                                  cache->size);
    }

    return result;
}

/* This is not used directly, but creates translatable strings for
 * the type string returned from /sys/.../cache */
//static const char* cache_types[] = {
//    NC_("cache-type", /*/cache type, as appears in: Level 1 (Data)*/ "Data"),
//    NC_("cache-type", /*/cache type, as appears in: Level 1 (Instruction)*/ "Instruction"),
//    NC_("cache-type", /*/cache type, as appears in: Level 2 (Unified)*/ "Unified")
//};

static void __cache_obtain_info(Processor *processor)
{
    ProcessorCache *cache;
    gchar *endpoint, *entry, *index;
    gchar *uref = NULL;
    gint i;
    gint processor_number = processor->id;

    endpoint = g_strdup_printf("/sys/devices/system/cpu/cpu%d/cache", processor_number);

    for (i = 0; ; i++) {
      cache = g_new0(ProcessorCache, 1);

      index = g_strdup_printf("index%d/", i);

      entry = g_strconcat(index, "type", NULL);
      cache->type = h_sysfs_read_string(endpoint, entry);
      g_free(entry);

      if (!cache->type) {
        g_free(cache);
        g_free(index);
        goto fail;
      }

      entry = g_strconcat(index, "level", NULL);
      cache->level = h_sysfs_read_int(endpoint, entry);
      g_free(entry);

      entry = g_strconcat(index, "number_of_sets", NULL);
      cache->number_of_sets = h_sysfs_read_int(endpoint, entry);
      g_free(entry);

      entry = g_strconcat(index, "physical_line_partition", NULL);
      cache->physical_line_partition = h_sysfs_read_int(endpoint, entry);
      g_free(entry);

      entry = g_strconcat(index, "size", NULL);
      cache->size = h_sysfs_read_int(endpoint, entry);
      g_free(entry);

      entry = g_strconcat(index, "ways_of_associativity", NULL);
      cache->ways_of_associativity = h_sysfs_read_int(endpoint, entry);
      g_free(entry);

      /* unique cache references: id is nice, but share_cpu_list can be
       * used if it is not available. */
      entry = g_strconcat(index, "id", NULL);
      uref = h_sysfs_read_string(endpoint, entry);
      g_free(entry);
      if (uref != NULL && *uref != 0 )
        cache->uid = atoi(uref);
      else
        cache->uid = -1;
      g_free(uref);
      entry = g_strconcat(index, "shared_cpu_list", NULL);
      cache->shared_cpu_list = h_sysfs_read_string(endpoint, entry);
      g_free(entry);

      /* reacharound */
      entry = g_strconcat(index, "../../topology/physical_package_id", NULL);
      cache->phy_sock = h_sysfs_read_int(endpoint, entry);
      g_free(entry);

      g_free(index);

      processor->cache = g_slist_append(processor->cache, cache);
    }

fail:
    g_free(endpoint);
}


#define cmp_cache_test(f) if (a->f < b->f) return -1; if (a->f > b->f) return 1;

static gint cmp_cache(ProcessorCache *a, ProcessorCache *b) {
        gint i = 0;
        cmp_cache_test(phy_sock);
        i = g_strcmp0(a->type, b->type); if (i!=0) return i;
        cmp_cache_test(level);
        cmp_cache_test(size);
        cmp_cache_test(uid); /* uid is unique among caches with the same (type, level) */
        if (a->uid == -1) {
            /* if id wasn't available, use shared_cpu_list as a unique ref */
            i = g_strcmp0(a->shared_cpu_list, b->shared_cpu_list); if (i!=0)
            return i;
        }
        return 0;
}

static gint cmp_cache_ignore_id(ProcessorCache *a, ProcessorCache *b) {
        gint i = 0;
        cmp_cache_test(phy_sock);
        i = g_strcmp0(a->type, b->type); if (i!=0) return i;
        cmp_cache_test(level);
        cmp_cache_test(size);
        return 0;
}

gchar *caches_summary(GSList * processors)
{
    gchar *ret = g_strdup_printf("[%s]\n", _("Caches"));
    GSList *all_cache = NULL, *uniq_cache = NULL;
    GSList *tmp, *l;
    Processor *p;
    ProcessorCache *c, *cur = NULL;
    gint cur_count = 0;

    /* create list of all cache references */
    for (l = processors; l; l = l->next) {
        p = (Processor*)l->data;
        if (p->cache) {
            tmp = g_slist_copy(p->cache);
            if (all_cache) {
                all_cache = g_slist_concat(all_cache, tmp);
            } else {
                all_cache = tmp;
            }
        }
    }

    if (g_slist_length(all_cache) == 0) {
        ret = h_strdup_cprintf("%s=\n", ret, _("(Not Available)") );
        g_slist_free(all_cache);
        return ret;
    }

    /* ignore duplicate references */
    all_cache = g_slist_sort(all_cache, (GCompareFunc)cmp_cache);
    for (l = all_cache; l; l = l->next) {
        c = (ProcessorCache*)l->data;
        if (!cur) {
            cur = c;
        } else {
            if (cmp_cache(cur, c) != 0) {
                uniq_cache = g_slist_prepend(uniq_cache, cur);
                cur = c;
            }
        }
    }
    uniq_cache = g_slist_prepend(uniq_cache, cur);
    uniq_cache = g_slist_reverse(uniq_cache);
    cur = 0, cur_count = 0;

    /* count and list caches */
    for (l = uniq_cache; l; l = l->next) {
        c = (ProcessorCache*)l->data;
        if (!cur) {
            cur = c;
            cur_count = 1;
        } else {
            if (cmp_cache_ignore_id(cur, c) != 0) {
                ret = h_strdup_cprintf(_("Level %d (%s)#%d=%dx %dKB (%dKB), %d-way set-associative, %d sets\n"),
                                      ret,
                                      cur->level,
                                      C_("cache-type", cur->type),
                                      cur->phy_sock,
                                      cur_count,
                                      cur->size,
                                      cur->size * cur_count,
                                      cur->ways_of_associativity,
                                      cur->number_of_sets);
                cur = c;
                cur_count = 1;
            } else {
                cur_count++;
            }
        }
    }
    ret = h_strdup_cprintf(_("Level %d (%s)#%d=%dx %dKB (%dKB), %d-way set-associative, %d sets\n"),
                          ret,
                          cur->level,
                          C_("cache-type", cur->type),
                          cur->phy_sock,
                          cur_count,
                          cur->size,
                          cur->size * cur_count,
                          cur->ways_of_associativity,
                          cur->number_of_sets);

    g_slist_free(all_cache);
    g_slist_free(uniq_cache);
    return ret;
}


GSList *
processor_scan(void)
{
    GSList *procs = NULL;
    Processor *processor = NULL;
    FILE *cpuinfo;
    gchar buffer[128];
    gchar *rep_pname = NULL;
    GSList *pi = NULL;
    dtr *dt = dtr_new(NULL);

    cpuinfo = fopen(PROC_CPUINFO, "r");
    if (!cpuinfo)
    return NULL;

#define CHECK_FOR(k) (g_str_has_prefix(tmp[0], k))
    while (fgets(buffer, 128, cpuinfo)) {
        gchar **tmp = g_strsplit(buffer, ":", 2);
        if (tmp[0] && tmp[1]) {
            tmp[0] = g_strstrip(tmp[0]);
            tmp[1] = g_strstrip(tmp[1]);
        } else {
            g_strfreev(tmp);
            continue;
        }

        get_str("Processor", rep_pname);

        if ( CHECK_FOR("processor") ) {
            /* finish previous */
            if (processor) {
                procs = g_slist_append(procs, processor);
            }

            /* start next */
            processor = g_new0(Processor, 1);
            processor->id = atol(tmp[1]);

            if (rep_pname)
                processor->linux_name = g_strdup(rep_pname);

            g_strfreev(tmp);
            continue;
        }

        if (!processor &&
            (  CHECK_FOR("model name")
            || CHECK_FOR("Features")
            || CHECK_FOR("BogoMIPS") ) ) {

            /* single proc/core may not have "processor : n" */
            processor = g_new0(Processor, 1);
            processor->id = 0;

            if (rep_pname)
                processor->linux_name = g_strdup(rep_pname);
        }

        if (processor) {
            get_str("model name", processor->linux_name);
            get_str("Features", processor->flags);
            get_float("BogoMIPS", processor->bogomips);

            get_str("CPU implementer", processor->cpu_implementer);
            get_str("CPU architecture", processor->cpu_architecture);
            get_str("CPU variant", processor->cpu_variant);
            get_str("CPU part", processor->cpu_part);
            get_str("CPU revision", processor->cpu_revision);
        }
        g_strfreev(tmp);
    }

    if (processor)
        procs = g_slist_append(procs, processor);

    g_free(rep_pname);
    fclose(cpuinfo);

    /* re-duplicate missing data for /proc/cpuinfo variant that de-duplicated it */
#define REDUP(f) if (dproc && dproc->f && !processor->f) processor->f = g_strdup(dproc->f);
    Processor *dproc=NULL;
    GSList *l;
    l = procs = g_slist_reverse(procs);
    while (l) {
        processor = l->data;
        if (processor->flags) {
            dproc = processor;
        } else if (dproc) {
            REDUP(flags);
            REDUP(cpu_implementer);
            REDUP(cpu_architecture);
            REDUP(cpu_variant);
            REDUP(cpu_part);
            REDUP(cpu_revision);
        }
        l = g_slist_next(l);
    }
    procs = g_slist_reverse(procs);

    /* data not from /proc/cpuinfo */
    for (pi = procs; pi; pi = pi->next) {
        processor = (Processor *) pi->data;

	__cache_obtain_info(processor);

        /* strings can't be null or segfault later */
        STRIFNULL(processor->linux_name, _("ARM Processor") );
        EMPIFNULL(processor->flags);
        UNKIFNULL(processor->cpu_implementer);
        UNKIFNULL(processor->cpu_architecture);
        UNKIFNULL(processor->cpu_variant);
        UNKIFNULL(processor->cpu_part);
        UNKIFNULL(processor->cpu_revision);

        processor->model_name = arm_decoded_name(
            processor->cpu_implementer, processor->cpu_part,
            processor->cpu_variant, processor->cpu_revision,
            processor->cpu_architecture, processor->linux_name);
        UNKIFNULL(processor->model_name);

        /* topo & freq */
        processor->cpufreq = cpufreq_new(processor->id);
        processor->cputopo = cputopo_new(processor->id);

        if (processor->cpufreq->cpukhz_max)
            processor->cpu_mhz = processor->cpufreq->cpukhz_max / 1000;
        else
            processor->cpu_mhz = 0.0f;

        /* Try OPP, although if it exists, it should have been available
         * via cpufreq. */
        if (dt && processor->cpu_mhz == 0.0f) {
            gchar *dt_cpu_path = g_strdup_printf("/cpus/cpu@%d", processor->id);
            dt_opp_range *opp = dtr_get_opp_range(dt, dt_cpu_path);
            if (opp) {
                processor->cpu_mhz = (double)opp->khz_max / 1000;
                g_free(opp);
            }
            g_free(dt_cpu_path);
        }

        /* mode */
        processor->mode = ARM_A32;
        if ( processor_has_flag(processor->flags, "pmull")
             || processor_has_flag(processor->flags, "crc32") ) {
#ifdef __aarch64__
                processor->mode = ARM_A64;
#else
                processor->mode = ARM_A32_ON_A64;
#endif
        }
    }
    dtr_free(dt);

    return procs;
}

gchar *processor_get_capabilities_from_flags(gchar * strflags)
{
    gchar **flags, **old;
    gchar *tmp = NULL;
    gint j = 0;

    flags = g_strsplit(strflags, " ", 0);
    old = flags;

    while (flags[j]) {
        const gchar *meaning = arm_flag_meaning( flags[j] );

        if (meaning) {
            tmp = h_strdup_cprintf("%s=%s\n", tmp, flags[j], meaning);
        } else {
            tmp = h_strdup_cprintf("%s=\n", tmp, flags[j]);
        }
        j++;
    }
    if (tmp == NULL || g_strcmp0(tmp, "") == 0)
        tmp = g_strdup_printf("%s=%s\n", "empty", _("Empty List"));

    g_strfreev(old);
    return tmp;
}

#define khzint_to_mhzdouble(k) (((double)k)/1000)
#define cmp_clocks_test(f) if (a->f < b->f) return -1; if (a->f > b->f) return 1;

static gint cmp_cpufreq_data(cpufreq_data *a, cpufreq_data *b) {
        gint i = 0;
        i = g_strcmp0(a->shared_list, b->shared_list); if (i!=0) return i;
        cmp_clocks_test(cpukhz_max);
        cmp_clocks_test(cpukhz_min);
        return 0;
}

static gint cmp_cpufreq_data_ignore_affected(cpufreq_data *a, cpufreq_data *b) {
        cmp_clocks_test(cpukhz_max);
        cmp_clocks_test(cpukhz_min);
        return 0;
}

gchar *clocks_summary(GSList * processors)
{
    gchar *ret = g_strdup_printf("[%s]\n", _("Clocks"));
    GSList *all_clocks = NULL, *uniq_clocks = NULL;
    GSList *l;
    Processor *p;
    cpufreq_data *c, *cur = NULL;
    gint cur_count = 0;

    /* create list of all clock references */
    for (l = processors; l; l = l->next) {
        p = (Processor*)l->data;
        if (p->cpufreq && p->cpufreq->cpukhz_max > 0) {
            all_clocks = g_slist_prepend(all_clocks, p->cpufreq);
        }
    }

    if (g_slist_length(all_clocks) == 0) {
        ret = h_strdup_cprintf("%s=\n", ret, _("(Not Available)") );
        g_slist_free(all_clocks);
        return ret;
    }

    /* ignore duplicate references */
    all_clocks = g_slist_sort(all_clocks, (GCompareFunc)cmp_cpufreq_data);
    for (l = all_clocks; l; l = l->next) {
        c = (cpufreq_data*)l->data;
        if (!cur) {
            cur = c;
        } else {
            if (cmp_cpufreq_data(cur, c) != 0) {
                uniq_clocks = g_slist_prepend(uniq_clocks, cur);
                cur = c;
            }
        }
    }
    uniq_clocks = g_slist_prepend(uniq_clocks, cur);
    uniq_clocks = g_slist_reverse(uniq_clocks);
    cur = 0, cur_count = 0;

    /* count and list clocks */
    for (l = uniq_clocks; l; l = l->next) {
        c = (cpufreq_data*)l->data;
        if (!cur) {
            cur = c;
            cur_count = 1;
        } else {
            if (cmp_cpufreq_data_ignore_affected(cur, c) != 0) {
                ret = h_strdup_cprintf(_("%.2f-%.2f %s=%dx\n"),
                                ret,
                                khzint_to_mhzdouble(cur->cpukhz_min),
                                khzint_to_mhzdouble(cur->cpukhz_max),
                                _("MHz"),
                                cur_count);
                cur = c;
                cur_count = 1;
            } else {
                cur_count++;
            }
        }
    }
    ret = h_strdup_cprintf(_("%.2f-%.2f %s=%dx\n"),
                    ret,
                    khzint_to_mhzdouble(cur->cpukhz_min),
                    khzint_to_mhzdouble(cur->cpukhz_max),
                    _("MHz"),
                    cur_count);

    g_slist_free(all_clocks);
    g_slist_free(uniq_clocks);
    return ret;
}

gchar *
processor_get_detailed_info(Processor *processor)
{
    gchar *tmp_flags, *tmp_imp = NULL, *tmp_part = NULL, *tmp_cache;
    gchar *tmp_arch, *tmp_cpufreq, *tmp_topology, *ret;
    tmp_flags = processor_get_capabilities_from_flags(processor->flags);
    arm_part(processor->cpu_implementer, processor->cpu_part, &tmp_imp, &tmp_part);
    tmp_arch = (char*)arm_arch_more(processor->cpu_architecture);

    tmp_topology = cputopo_section_str(processor->cputopo);
    tmp_cpufreq = cpufreq_section_str(processor->cpufreq);
    tmp_cache = __cache_get_info_as_string(processor->cache);

    ret = g_strdup_printf("[%s]\n"
                       "%s=%s\n"       /* linux name */
                       "%s=%s\n"       /* decoded name */
                       "%s=%s\n"       /* mode */
                       "%s=%.2f %s\n"  /* frequency */
                       "%s=%.2f\n"     /* bogomips */
                       "%s=%s\n"       /* byte order */
                       "%s"            /* topology */
                       "%s"            /* frequency scaling */
		       "[%s]\n%s\n"    /* cache */
                       "[%s]\n"        /* ARM */
                       "%s=[%s] %s\n"  /* implementer */
                       "%s=[%s] %s\n"  /* part */
                       "%s=[%s] %s\n"  /* architecture */
                       "%s=%s\n"       /* variant */
                       "%s=%s\n"       /* revision */
                       "[%s]\n"        /* flags */
		       "%s",
                   _("Processor"),
                   _("Linux Name"), processor->linux_name,
                   _("Decoded Name"), processor->model_name,
                   _("Mode"), arm_mode_str[processor->mode],
                   _("Frequency"), processor->cpu_mhz, _("MHz"),
                   _("BogoMips"), processor->bogomips,
                   _("Byte Order"), byte_order_str(),
                   tmp_topology,
                   tmp_cpufreq,
		   _("Cache"), tmp_cache,
                   _("ARM"),
                   _("Implementer"), processor->cpu_implementer, (tmp_imp) ? tmp_imp : "",
                   _("Part"), processor->cpu_part, (tmp_part) ? tmp_part : "",
                   _("Architecture"), processor->cpu_architecture, (tmp_arch) ? tmp_arch : "",
                   _("Variant"), processor->cpu_variant,
                   _("Revision"), processor->cpu_revision,
                   _("Capabilities"), tmp_flags);
    g_free(tmp_flags);
    g_free(tmp_cpufreq);
    g_free(tmp_topology);
    g_free(tmp_cache);
    return ret;
}

gchar *processor_name(GSList *processors) {
    /* compatible contains a list of compatible hardware, so be careful
     * with matching order.
     * ex: "ti,omap3-beagleboard-xm", "ti,omap3450", "ti,omap3";
     * matches "omap3 family" first.
     * ex: "brcm,bcm2837", "brcm,bcm2836";
     * would match 2836 when it is a 2837.
     */
#define UNKSOC "(Unknown)" /* don't translate this */
    const struct {
        char *search_str;
        char *vendor;
        char *soc;
    } dt_compat_searches[] = {
        { "allwinner,sun8i-h3", "Allwinner", "H3" },
        { "amlogic,a311d", "Amlogic", "A311D (Vim3)" }, // VIM3
        { "amlogic,s905w", "Amlogic", "S905W" },
        { "amlogic,s912", "Amlogic", "S912" },
        { "apple,t8132", "Apple", "T8132 (M4)" },
        { "apple,t8122", "Apple", "T8122 (M3)" },
        { "apple,t8112", "Apple", "T8112 (M2)" },
        { "apple,t8103", "Apple", "T8103 (M1 A14X)" },
        { "apple,t8101", "Apple", "T8101 (M1 A14)" },
        { "apple,t6042", "Apple", "T6042 (M4 Ultra)" },
        { "apple,t6041", "Apple", "T6041 (M4 Max)" },
        { "apple,t6040", "Apple", "T6040 (M4 Pro)" },
        { "apple,t6032", "Apple", "T6032 (M3 Ultra)" },
        { "apple,t6031", "Apple", "T6031 (M3 Max)" },
        { "apple,t6030", "Apple", "T6030 (M3 Pro)" },
        { "apple,t6022", "Apple", "T6022 (M2 Ultra)" },
        { "apple,t6021", "Apple", "T6021 (M2 Max)" },
        { "apple,t6020", "Apple", "T6020 (M2 Pro)" },
        { "apple,t6002", "Apple", "T6002 (M1 Ultra)" },
        { "apple,t6001", "Apple", "T6001 (M1 Max)" },
        { "apple,t6000", "Apple", "T6000 (M1 Pro)" },
        { "brcm,bcm2712", "Broadcom", "BCM2712 (RPi5)" }, // RPi 5
        { "brcm,bcm2711", "Broadcom", "BCM2711 (RPi4)" }, // RPi 4
        { "brcm,bcm2837", "Broadcom", "BCM2837 (RPi3)" }, // RPi 3
        { "brcm,bcm2836", "Broadcom", "BCM2836 (RPi2)" }, // RPi 2
        { "brcm,bcm2835", "Broadcom", "BCM2835 (RPi1)" }, // RPi 1
        { "friendlyelec,nanopi-k1-plus", "Allwinner", "H5 (K1+)" }, // K1+
        { "hardkernel,odroid-c2", "Amlogic", "S905 (C2)" }, // C2
        { "hardkernel,odroid-n2", "Amlogic", "S922x (N2)" }, // N2
        { "mediatek,mt8173", "MediaTek", "MT8173" },
        { "mediatek,mt8183", "MediaTek", "MT8183" },
        { "mediatek,mt8167", "MediaTek", "MT8167" },
        { "mediatek,mt6895", "MediaTek", "MT6895" },
        { "mediatek,mt6799", "MediaTek", "MT6799 (Helio X30)" },
        { "mediatek,mt6799", "MediaTek", "MT6799 (Helio X30)" },
        { "mediatek,mt6797x", "MediaTek", "MT6797X (Helio X27)" },
        { "mediatek,mt6797t", "MediaTek", "MT6797T (Helio X25)" },
        { "mediatek,mt6797", "MediaTek", "MT6797 (Helio X20)" },
        { "mediatek,mt6757T", "MediaTek", "MT6757T (Helio P25)" },
        { "mediatek,mt6757", "MediaTek", "MT6757 (Helio P20)" },
        { "mediatek,mt6795", "MediaTek", "MT6795 (Helio X10)" },
        { "mediatek,mt6755", "MediaTek", "MT6755 (Helio P10)" },
        { "mediatek,mt6750t", "MediaTek", "MT6750T" },
        { "mediatek,mt6750", "MediaTek", "MT6750" },
        { "mediatek,mt6753", "MediaTek", "MT6753" },
        { "mediatek,mt6752", "MediaTek", "MT6752" },
        { "mediatek,mt6738", "MediaTek", "MT6738" },
        { "mediatek,mt6737t", "MediaTek", "MT6737T" },
        { "mediatek,mt6735", "MediaTek", "MT6735" },
        { "mediatek,mt6732", "MediaTek", "MT6732" },
        { "nvidia,tegra", "nVidia", "Tegra-family" },
        { "qcom,pineapple", "Qualcomm", "Pineapple (SD8g3)"},
        { "qcom,sm8150", "Qualcomm", "SM8150"},
        { "qcom,sm8550", "Qualcomm", "SM8550 (SD8g2)"},
        { "qcom,msm8939", "Qualcomm", "Snapdragon 615"},
        { "qcom,msm", "Qualcomm", "Snapdragon-family"},
        { "qcom,x1e80100", "Qualcomm", "Snapdragon X Elite"},
        { "qcom,x1p42100", "Qualcomm", "Snapdragon X Plus"},
        { "rockchip,rk3288", "Rockchip", "RK3288" }, // Asus Tinkerboard
        { "rockchip,rk3328", "Rockchip", "RK3328" }, // Firefly Renegade
        { "rockchip,rk3399", "Rockchip", "RK3399" }, // Firefly Renegade Elite
        { "rockchip,rk32", "Rockchip", "RK32xx-family" },
        { "rockchip,rk33", "Rockchip", "RK33xx-family" },
        { "rockchip,rk3588", "Rockchip", "RK3588" }, // rk3588-orangepi-5-max
        { "sprd,sc9863a", "Unisoc", "SC9864A" },
        { "ti,omap5432", "Texas Instruments", "OMAP5432" },
        { "ti,omap5430", "Texas Instruments", "OMAP5430" },
        { "ti,omap4470", "Texas Instruments", "OMAP4470" },
        { "ti,omap4460", "Texas Instruments", "OMAP4460" },
        { "ti,omap4430", "Texas Instruments", "OMAP4430" },
        { "ti,omap3620", "Texas Instruments", "OMAP3620" },
        { "ti,omap3450", "Texas Instruments", "OMAP3450" },
        { "ti,omap5", "Texas Instruments", "OMAP5-family" },
        { "ti,omap4", "Texas Instruments", "OMAP4-family" },
        { "ti,omap3", "Texas Instruments", "OMAP3-family" },
        { "ti,omap2", "Texas Instruments", "OMAP2-family" },
        { "ti,omap1", "Texas Instruments", "OMAP1-family" },
	
        /* Generic vendor names only*/
        { "amlogic,", "Amlogic", UNKSOC },
        { "allwinner,", "Allwinner", UNKSOC },
        { "brcm,", "Broadcom", UNKSOC },
        { "mediatek,", "MediaTek", UNKSOC },
        { "nvidia,", "nVidia", UNKSOC },
        { "qcom,", "Qualcom", UNKSOC },
        { "rockchip,", "Rockchip", UNKSOC },
        { "ti,", "Texas Instruments", UNKSOC },
        { NULL, NULL }
    };
    gchar *ret = NULL;
    gchar *compat = NULL;
    int i;

    compat = dtr_get_string("/compatible", 1);

    if (compat != NULL) {
        i = 0;
        while(dt_compat_searches[i].search_str != NULL) {
            if (strstr(compat, dt_compat_searches[i].search_str) != NULL) {
	        if(strstr(dt_compat_searches[i].soc,"Unknown"))
		    ret = g_strdup_printf("%s %s (%s)", dt_compat_searches[i].vendor, dt_compat_searches[i].soc, compat);
		else
                    ret = g_strdup_printf("%s %s", dt_compat_searches[i].vendor, dt_compat_searches[i].soc);
                break;
            }
            i++;
        }
        if(!ret) ret = g_strdup_printf("ARM Processor (%s)", compat);
        g_free(compat);
    }

    if(!ret) ret = g_strdup("ARM Processor (NoDT)");
    return ret;
}

gchar *processor_describe(GSList * processors) {
    return processor_describe_by_counting_names(processors);
}

gchar *processor_meta(GSList * processors) {
    gchar *meta_soc = processor_name(processors);
    gchar *meta_cpu_desc = processor_describe(processors);
    gchar *meta_cpu_topo = processor_describe_default(processors);
    gchar *meta_freq_desc = processor_frequency_desc(processors);
    gchar *meta_clocks = clocks_summary(processors);
    gchar *meta_caches = caches_summary(processors);
    gchar *meta_hwcaps = ldlinux_hwcaps_info();
    gchar *ret = NULL;
    UNKIFNULL(meta_cpu_desc);
    ret = g_strdup_printf("[%s]\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s=%s\n"
                            "%s"
                            "%s"
                            "%s",
                            _("SOC/Package"),
                            _("Name"), meta_soc,
                            _("Description"), meta_cpu_desc,
                            _("Topology"), meta_cpu_topo,
                            _("Logical CPU Config"), meta_freq_desc,
			    meta_hwcaps,
			    meta_clocks, meta_caches );
    g_free(meta_soc);
    g_free(meta_cpu_desc);
    g_free(meta_cpu_topo);
    g_free(meta_freq_desc);
    g_free(meta_hwcaps);
    g_free(meta_clocks);
    g_free(meta_caches);
    return ret;
}

gchar *processor_get_info(GSList * processors)
{
    Processor *processor;
    gchar *ret, *tmp, *hashkey;
    gchar *meta; /* becomes owned by more_info? no need to free? */
    GSList *l;
    gchar *icons=g_strdup("");

    tmp = g_strdup_printf("$!CPU_META$%s=\n", _("SOC/Package Information") );

    meta = processor_meta(processors);
    moreinfo_add_with_prefix("DEV", "CPU_META", meta);

    for (l = processors; l; l = l->next) {
        processor = (Processor *) l->data;

        icons = h_strdup_cprintf("Icon$CPU%d$cpu%d=processor.svg\n", icons, processor->id, processor->id);

        tmp = g_strdup_printf("%s$CPU%d$%s=%.2f %s\n",
                  tmp, processor->id,
                  processor->model_name,
                  processor->cpu_mhz, _("MHz"));

        hashkey = g_strdup_printf("CPU%d", processor->id);
        moreinfo_add_with_prefix("DEV", hashkey,
                processor_get_detailed_info(processor));
        g_free(hashkey);
    }

    ret = g_strdup_printf("[$ShellParam$]\n"
                  "ViewType=1\n"
                  "ColumnTitle$TextValue=%s\n"
                  "ColumnTitle$Value=%s\n"
                  "ColumnTitle$Extra1=%s\n"
                  "ColumnTitle$Extra2=%s\n"
                  "ShowColumnHeaders=true\n"
                  "%s"
                  "[Processors]\n"
                  "%s", _("Device"), _("Frequency"), _("Model"), _("Socket:Core"), icons, tmp);
    g_free(tmp);
    g_free(icons);

    // now here's something fun...
    struct Info *i = info_unflatten(ret);
    g_free(ret);
    ret = info_flatten(i);

    return ret;
}
