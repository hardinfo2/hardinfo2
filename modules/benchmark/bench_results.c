/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2020 L. A. F. Pereira <l@tia.mat.br>
 *    This file:
 *    Copyright (C) 2017 Burt P. <pburt0@gmail.com>
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

#include <stdlib.h>
#include <ctype.h>
#include <locale.h>
#include <inttypes.h>
#include <json-glib/json-glib.h>
#include "nice_name.h"

/* in dmi_memory.c */
uint64_t memory_devices_get_system_memory_MiB();
gchar *memory_devices_get_system_memory_types_str();

/*!/ Used for an unknown value. Having it in only one place cleans up the .po
 * line references */
static const char *unk = N_("(Unknown)");

typedef struct {
    char *board;
    uint64_t memory_kiB; /* from /proc/meminfo -> MemTotal */
    char *cpu_name;
    char *cpu_desc;
    char *cpu_config;
    char *cpu_hwcaps;
    char *ogl_renderer;
    char *gpu_desc;
    int processors;
    int cores;
    int threads;
    int nodes;
    char *mid;
    int ptr_bits;             /* 32, 64... BENCH_PTR_BITS; 0 for unspecified */
    int is_su_data;           /* 1 = data collected as root */
    uint64_t memory_phys_MiB; /* from DMI/SPD/DTree/Table/Blocks, etc. */
    char *ram_types;
    int machine_data_version;
    char *machine_type;
    char *linux_kernel;       /*kernelarch*/
    char *linux_os;           /*distroversion*/
    char *power_state;
    char *gpu_name;
    char *storage;
    char *vulkanDriver;
    char *vulkanDevice;
    char *vulkanVersions;
} bench_machine;

typedef struct {
    char *name;
    bench_value bvalue;
    bench_machine *machine;
    int legacy; /* an old benchmark.conf result */
} bench_result;


/* "2x 1400.00 MHz + 2x 800.00 MHz" -> 4400.0 */
static float cpu_config_val(char *str)
{
    char *c = str;
    int t;
    float f, r = 0.0;
    if (str != NULL) {
        if (strchr(str, 'x')) {
	    while (c != NULL && (sscanf(c, "%dx %f", &t, &f)==2)) {
                r += f * t;
                c = strchr(c + 1, '+');
                if (c)
                    c++; /* move past the + */
            }
        } else {
            sscanf(c, "%f", &r);
        }
    }
    return r;
}

static void gen_machine_id(bench_machine *m)
{
    char *s;

    if (m) {
        if (m->mid != NULL)
            free(m->mid);

        /* Don't try and translate unknown. The mid string needs to be made of
         * all untranslated elements.*/
        m->mid = g_strdup_printf("%s;%s;%.2f",
                                 (m->board != NULL) ? m->board : "(Unknown)",
                                 m->cpu_name, cpu_config_val(m->cpu_config));
        for (s = m->mid; *s; s++) {
            if (!isalnum(*s) && *s != '(' && *s != ')' && *s != ';')
                *s = '_';
        }
    }
}

bench_machine *bench_machine_new()
{
    return calloc(1, sizeof(bench_machine));
}

bench_machine *bench_machine_this()
{
    bench_machine *m = NULL;
    char *tmp;

    m = bench_machine_new();
    if (m) {
        m->ptr_bits = BENCH_PTR_BITS;
        m->is_su_data = (getuid() == 0);
        m->board = module_call_method("devices::getMotherboard");
        m->cpu_name = module_call_method("devices::getProcessorName");
        m->cpu_desc = module_call_method("devices::getProcessorDesc");
        m->cpu_config = module_call_method("devices::getProcessorFrequencyDesc");
	m->cpu_hwcaps = module_call_method("devices::getProcessorHwCaps");
        m->gpu_desc = module_call_method("devices::getGPUList");
        m->ogl_renderer = module_call_method("computer::getOGLRenderer");
        tmp = module_call_method("computer::getMemoryTotal");
        m->memory_kiB = strtoull(tmp, NULL, 10);
        free(tmp);
        m->memory_phys_MiB = memory_devices_get_system_memory_MiB();
        m->ram_types = memory_devices_get_system_memory_types_str();
        m->machine_type = module_call_method("computer::getMachineTypeEnglish");
	m->linux_kernel = module_call_method("computer::getOSKernel");
	m->linux_os = module_call_method("computer::getOS");
	m->power_state = module_call_method("devices::getPowerState");
	m->gpu_name = module_call_method("devices::getGPUname");
	//m->storage = module_call_method("devices::getStorageDevicesModels");
	m->storage = module_call_method("devices::getStorageHomeModels");
	m->vulkanDriver = module_call_method("computer::getVulkanDriver");
	m->vulkanDevice = module_call_method("computer::getVulkanDevice");
	m->vulkanVersions = module_call_method("computer::getVulkanVersions");

        cpu_procs_cores_threads_nodes(&m->processors, &m->cores, &m->threads, &m->nodes);
        gen_machine_id(m);
    }
    return m;
}

void bench_machine_free(bench_machine *s)
{
    if (s) {
        free(s->board);
        free(s->cpu_name);
        free(s->cpu_desc);
        free(s->cpu_config);
        free(s->mid);
        free(s->ram_types);
        free(s->machine_type);
	free(s->linux_kernel);
	free(s->linux_os);
        free(s);
    }
}

void bench_result_free(bench_result *s)
{
    if (s) {
        free(s->name);
        bench_machine_free(s->machine);
        g_free(s);
    }
}

bench_result *bench_result_this_machine(const char *bench_name, bench_value r)
{
    bench_result *b = NULL;

    b = malloc(sizeof(bench_result));
    if (b) {
        memset(b, 0, sizeof(bench_result));
        b->machine = bench_machine_this();
        b->name = strdup(bench_name);
        b->bvalue = r;
        b->legacy = 0;
    }
    return b;
}


static void filter_invalid_chars(gchar *str)
{
    gchar *p;

    for (p = str; *p; p++) {
        if (*p == '\n' || *p == ';' || *p == '|')
            *p = '_';
    }
}

static gboolean json_get_boolean(JsonObject *obj, const gchar *key)
{
    if (!json_object_has_member(obj, key))
        return FALSE;
    return json_object_get_boolean_member(obj, key);
}

static double json_get_double(JsonObject *obj, const gchar *key)
{
    if (!json_object_has_member(obj, key))
        return 0;
    return json_object_get_double_member(obj, key);
}

static int json_get_int(JsonObject *obj, const gchar *key)
{
    if (!json_object_has_member(obj, key))
        return 0;
    return json_object_get_int_member(obj, key);
}

static const gchar *json_get_string(JsonObject *obj, const gchar *key)
{
    if (!json_object_has_member(obj, key))
        return "";
    return json_object_get_string_member(obj, key);
}

static gchar *json_get_string_dup(JsonObject *obj, const gchar *key)
{
    return g_strdup(json_get_string(obj, key));
}

static double parse_frequency(const char *freq)
{
    static locale_t locale;

    if (!locale)
        locale = newlocale(LC_NUMERIC_MASK, "C", NULL);

    return strtod_l(freq, NULL, locale);
}

static void append_cpu_config(JsonObject *object,
                              const gchar *member_name,
                              JsonNode *member_node,
                              gpointer user_data)
{
    GString *output = user_data;

    if (output->len)
        g_string_append(output, ", ");

    g_string_append_printf(output, "%ldx %.2f %s", (long int)json_node_get_int(member_node),
                           parse_frequency(member_name), _("MHz"));
}

static gchar *get_cpu_config(JsonObject *machine)
{
    JsonObject *cpu_config_map=NULL;

    if(json_object_has_member(machine, "CpuConfigMap"))
        cpu_config_map=json_object_get_object_member(machine, "CpuConfigMap");

    if (!cpu_config_map)
        return json_get_string_dup(machine, "CpuConfig");

    GString *output = g_string_new(NULL);
    json_object_foreach_member(cpu_config_map, append_cpu_config, output);
    return g_string_free(output, FALSE);
}

static gchar *get_cpu_desc(JsonObject *machine)
{
    int num_cpus = json_get_int(machine, "NumCpus");

    if (!num_cpus)
        return json_get_string_dup(machine, "CpuDesc");

    /* FIXME: This is adapted from processor_describe_default() in
     * devices.c! */

    int num_cores = json_get_int(machine, "NumCores");
    int num_threads = json_get_int(machine, "NumThreads");
    int num_nodes = json_get_int(machine, "NumNodes");
    const char *packs_fmt =
        ngettext("%d physical processor", "%d physical processors", num_cpus);
    const char *cores_fmt = ngettext("%d core", "%d cores", num_cores);
    const char *threads_fmt = ngettext("%d thread", "%d threads", num_threads);
    char *full_fmt, *ret;

    if (num_nodes > 1) {
        const char *nodes_fmt =
            ngettext("%d NUMA node", "%d NUMA nodes", num_nodes);

        full_fmt = g_strdup_printf(
            _(/*!/NP procs; NC cores across NN nodes; NT threads*/
              "%s; %s across %s; %s"),
            packs_fmt, cores_fmt, nodes_fmt, threads_fmt);
        ret = g_strdup_printf(full_fmt, num_cpus, num_cores * num_nodes, num_nodes, num_threads);
    } else {
        full_fmt =
            g_strdup_printf(_(/*!/NP procs; NC cores; NT threads*/ "%s; %s; %s"),
                            packs_fmt, cores_fmt, threads_fmt);
        ret = g_strdup_printf(full_fmt, num_cpus, num_cores, num_threads);
    }

    free(full_fmt);
    return ret;
}

bench_result *bench_result_benchmarkjson(const gchar *bench_name,
                                         JsonNode *node)
{
    JsonObject *machine;
    bench_result *b;

    if (!node || (json_node_get_node_type(node) != JSON_NODE_OBJECT))
        return NULL;

    machine = json_node_get_object(node);

    b = g_new0(bench_result, 1);
    b->name = g_strdup(bench_name);
    b->legacy = json_get_boolean(machine, "Legacy");

    b->bvalue = (bench_value){
        .result = json_get_double(machine, "BenchmarkResult"),
        .elapsed_time = json_get_double(machine, "ElapsedTime"),
        .threads_used = json_get_int(machine, "UsedThreads"),
        .revision = json_get_int(machine, "BenchmarkVersion"),//Revision
    };

    snprintf(b->bvalue.extra, sizeof(b->bvalue.extra), "%s",
             json_get_string(machine, "ExtraInfo"));
    filter_invalid_chars(b->bvalue.extra);

    int nodes = json_get_int(machine, "NumNodes");

    if (nodes == 0)
        nodes = 1;

    b->machine = bench_machine_new();
    *b->machine = (bench_machine){
        .board = json_get_string_dup(machine, "Board"),
        .memory_kiB = json_get_int(machine, "MemoryInKiB"),
        .cpu_name = json_get_string_dup(machine, "CpuName"),
        .cpu_desc = get_cpu_desc(machine),
        .cpu_config = get_cpu_config(machine),
        .ogl_renderer = json_get_string_dup(machine, "OpenGlRenderer"),
        .gpu_desc = json_get_string_dup(machine, "GpuDesc"),
        .processors = json_get_int(machine, "NumCpus"),
        .cores = json_get_int(machine, "NumCores"),
        .threads = json_get_int(machine, "NumThreads"),
        .nodes = nodes,
        .mid = json_get_string_dup(machine, "MachineId"),
        .ptr_bits = json_get_int(machine, "PointerBits"),
        .is_su_data = json_get_boolean(machine, "DataFromSuperUser"),
        .memory_phys_MiB = json_get_int(machine, "PhysicalMemoryInMiB"),
        .ram_types = json_get_string_dup(machine, "MemoryTypes"),
        .machine_data_version = json_get_int(machine, "MachineDataVersion"),
        .machine_type = json_get_string_dup(machine, "MachineType"),
	.gpu_name= json_get_string_dup(machine, "GPU"),
	.storage= json_get_string_dup(machine, "Storage"),

    };

    return b;
}

static char *bench_result_more_info_less(bench_result *b)
{
    char *memory = NULL;
    if (b->machine->memory_phys_MiB) {
        memory =
            g_strdup_printf("%" PRId64 " %s %s", b->machine->memory_phys_MiB,
                            _("MiB"), b->machine->ram_types);
    } else {
        memory =
            (b->machine->memory_kiB > 0)
                ? g_strdup_printf("%" PRId64 " %s %s", b->machine->memory_kiB,
                                  _("kiB"), problem_marker())
                : g_strdup(_(unk));
    }
    char bench_str[256] = "";
    if (b->bvalue.revision >= 0)
        snprintf(bench_str, 127, "%d", b->bvalue.revision);
    char bits[24] = "";
    if (b->machine->ptr_bits)
        snprintf(bits, 23, _("%d-bit"), b->machine->ptr_bits);

    char *ret = g_strdup_printf(
        "[%s]\n"
        /* threads */ "%s=%d\n"
        /* elapsed */ "%s=%0.4f %s\n"
        "%s=%s\n"
        "%s=%s\n"
        /* legacy */ "%s%s=%s\n"
        "[%s]\n"
        /* board */ "%s=%s\n"
        /* machine_type */ "%s=%s\n"
        /* cpu   */ "%s=%s\n"
        /* cpudesc */ "%s=%s\n"
        /* cpucfg */ "%s=%s\n"
        /* threads */ "%s=%d\n"
        /* gpu desc */ "%s=%s\n"
        /* ogl rend */ "%s=%s\n"
        /* storage */ "%s=%s\n"
        /* mem */ "%s=%s\n"
        /* bits */ "%s=%s\n",
        _("Benchmark Result"), _("Threads"), b->bvalue.threads_used,
        _("Elapsed Time"), b->bvalue.elapsed_time, _("seconds"),
        *bench_str ? _("Revision") : "#Revision", bench_str,
        *b->bvalue.extra ? _("Extra Information") : "#Extra", b->bvalue.extra,
        b->legacy ? problem_marker() : "",
        b->legacy ? _("Note") : "#Note",
        b->legacy ? _("This result is from an old version of HardInfo. Results "
                      "might not be comparable to current version. Some "
                      "details are missing.")
                  : "",
        _("Machine"),
        _("Board"), (b->machine->board != NULL) ? b->machine->board : _(unk),
        _("Machine Type"), (b->machine->machine_type != NULL) ? b->machine->machine_type : _(unk),
        _("CPU Name"), b->machine->cpu_name,
        _("CPU Description"), (b->machine->cpu_desc != NULL) ? b->machine->cpu_desc : _(unk),
        _("CPU Config"), b->machine->cpu_config,
        _("Threads Available"), b->machine->threads,
        _("GPU"), (b->machine->gpu_name != NULL) ? b->machine->gpu_name : (b->machine->gpu_desc != NULL) ? b->machine->gpu_desc : _(unk),
        _("OpenGL Renderer"), (b->machine->ogl_renderer != NULL) ? b->machine->ogl_renderer : _(unk),
        _("Storage"), (b->machine->storage != NULL) ? b->machine->storage : _(unk),
        _("Memory"), memory,
        b->machine->ptr_bits ? _("Pointer Size") : "#AddySize", bits);
    free(memory);
    return ret;
}

static char *bench_result_more_info_complete(bench_result *b)
{
    char bench_str[256] = "";
    strncpy(bench_str, b->name, 127);
    if (b->bvalue.revision >= 0)
        snprintf(bench_str + strlen(bench_str), 127, " (r%d)",
                 b->bvalue.revision);
    char bits[24] = "";
    if (b->machine->ptr_bits)
        snprintf(bits, 23, _("%d-bit"), b->machine->ptr_bits);

    return g_strdup_printf(
        "[%s]\n"
        /* bench name */ "%s=%s\n"
        /* threads */ "%s=%d\n"
        /* result */ "%s=%0.2f\n"
        /* elapsed */ "%s=%0.4f %s\n"
        "%s=%s\n"
        /* legacy */ "%s%s=%s\n"
        "[%s]\n"
        /* board */ "%s=%s\n"
        /* machine_type */ "%s=%s\n"
        /* linux_kernel */ "%s=%s\n"
        /* linux_os */ "%s=%s\n"
        /* cpu   */ "%s=%s\n"
        /* cpudesc */ "%s=%s\n"
        /* cpucfg */ "%s=%s\n"
        /* threads */ "%s=%d\n"
        /* gpu desc */ "%s=%s\n"
        /* ogl rend */ "%s=%s\n"
        /* storage */ "%s=%s\n"
        /* mem */ "%s=%" PRId64 " %s\n"
        /* mem phys */ "%s=%" PRId64 " %s %s\n"
        /* bits */ "%s=%s\n"
        "%s=%d\n"
        "%s=%d\n"
        "[%s]\n"
        /* mid */ "%s=%s\n"
        /* cfg_val */ "%s=%.2f\n",
        _("Benchmark Result"), _("Benchmark"), bench_str, _("Threads"),
        b->bvalue.threads_used, _("Result"), b->bvalue.result,
        _("Elapsed Time"), b->bvalue.elapsed_time, _("seconds"),
        *b->bvalue.extra ? _("Extra Information") : "#Extra", b->bvalue.extra,
        b->legacy ? problem_marker() : "",
        b->legacy ? _("Note") : "#Note",
        b->legacy ? _("This result is from an old version of HardInfo. Results "
                      "might not be comparable to current version. Some "
                      "details are missing.")
                  : "",
        _("Machine"), _("Board"),
        (b->machine->board != NULL) ? b->machine->board : _(unk),
        _("Machine Type"), (b->machine->machine_type != NULL) ? b->machine->machine_type : _(unk),
        _("Linux Kernel"), (b->machine->linux_kernel != NULL) ? b->machine->linux_kernel : _(unk),
        _("Linux OS"), (b->machine->linux_os != NULL) ? b->machine->linux_os : _(unk),
        _("CPU Name"),
        b->machine->cpu_name, _("CPU Description"),
        (b->machine->cpu_desc != NULL) ? b->machine->cpu_desc : _(unk),
        _("CPU Config"), b->machine->cpu_config, _("Threads Available"),
        b->machine->threads, _("GPU"),
        (b->machine->gpu_name != NULL) ? b->machine->gpu_name : (b->machine->gpu_desc != NULL) ? b->machine->gpu_desc : _(unk),
        _("OpenGL Renderer"),
        (b->machine->ogl_renderer != NULL) ? b->machine->ogl_renderer : _(unk),
        _("Storage"),
        (b->machine->storage != NULL) ? b->machine->storage : _(unk),
        _("Memory"), b->machine->memory_kiB, _("kiB"), _("Physical Memory"),
        b->machine->memory_phys_MiB, _("MiB"), b->machine->ram_types,
        b->machine->ptr_bits ? _("Pointer Size") : "#AddySize", bits,
        ".machine_data_version", b->machine->machine_data_version,
        ".is_su_data", b->machine->is_su_data, _("Handles"), _("mid"),
        b->machine->mid, _("cfg_val"), cpu_config_val(b->machine->cpu_config));
}

char *bench_result_more_info(bench_result *b)
{
    // return bench_result_more_info_complete(b);
    return bench_result_more_info_less(b);
}
