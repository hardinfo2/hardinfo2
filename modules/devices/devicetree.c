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
/*
 * Device Tree support by Burt P. <pburt0@gmail.com>
 * Sources:
 *   http://elinux.org/Device_Tree_Usage
 *   http://elinux.org/Device_Tree_Mysteries
 */
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include "hardinfo.h"
#include "devices.h"
#include "cpu_util.h"
#include "dt_util.h"
#include "appf.h"

gchar *dtree_info = NULL;
const char *dtree_mem_str = NULL; /* used by memory devices when nothing else is available */

/*static gchar *get_node(dtr *dt, char *np) {
    gchar *nodes = NULL, *props = NULL, *ret = NULL;
    gchar *tmp = NULL, *pstr = NULL, *lstr = NULL;
    gchar *dir_path;
    const gchar *fn;
    GDir *dir;
    dtr_obj *node, *child;

    props = g_strdup_printf("[%s]\n", _("Properties") );
    nodes = g_strdup_printf("[%s]\n", _("Children") );
    node = dtr_obj_read(dt, np);
    dir_path = dtr_obj_full_path(node);

    dir = g_dir_open(dir_path, 0 , NULL);
    if (dir) {
        while((fn = g_dir_read_name(dir)) != NULL) {
            child = dtr_get_prop_obj(dt, node, fn);
            pstr = hardinfo_clean_value(dtr_str(child), 1);
            lstr = hardinfo_clean_label(fn, 0);
            if (dtr_obj_type(child) == DT_NODE) {
                tmp = g_strdup_printf("%s%s=%s\n",
                    nodes, lstr, pstr);
                g_free(nodes);
                nodes = tmp;
            } else {
                tmp = g_strdup_printf("%s%s=%s\n",
                    props, lstr, pstr);
                g_free(props);
                props = tmp;
            }
            dtr_obj_free(child);
            g_free(pstr);
            g_free(lstr);
        }
    }
    g_dir_close(dir);
    g_free(dir_path);

    lstr = dtr_obj_alias(node);
    pstr = dtr_obj_symbol(node);
    ret = g_strdup_printf("[%s]\n"
                    "%s=%s\n"
                    "%s=%s\n"
                    "%s=%s\n"
                    "%s%s",
                    _("Node"),
                    _("Node Path"), dtr_obj_path(node),
                    _("Alias"), (lstr != NULL) ? lstr : _("(None)"),
                    _("Symbol"), (pstr != NULL) ? pstr : _("(None)"),
                    props, nodes);

    dtr_obj_free(node);
    g_free(props);
    g_free(nodes);

    return ret;
 }*/

/* different from  dtr_get_string() in that it re-uses the existing dt */
static char *get_dt_string(dtr *dt, char *path, gboolean decode) {
    char *ret;

    if (decode) {
        dtr_obj *obj = dtr_get_prop_obj(dt, NULL, path);

        ret = dtr_str(obj);

        dtr_obj_free(obj);
    } else {
        ret = dtr_get_prop_str(dt, NULL, path);
    }

    return ret;
}

static gchar *get_summary(dtr *dt) {
    char *model = NULL, *compat = NULL, *serial_number=NULL;
    char *ret = NULL;

    model = get_dt_string(dt, "/model", 0);
    compat = get_dt_string(dt, "/compatible", 1);
    serial_number = get_dt_string(dt, "/serial-number", 1);
    UNKIFNULL(model);
    EMPIFNULL(compat);
    EMPIFNULL(serial_number);

    ret = g_strdup_printf(
                "[%s]\n"
                "%s=%s\n"
                "%s=%s\n"
                "%s=%s\n",
                _("Board"),
                _("Model"), model,
                _("Serial Number"), serial_number,
                _("Compatible"), compat);

    g_free(serial_number);
    g_free(model);
    g_free(compat);

    return ret;
}

static void mi_add(const char *key, const char *value, int report_details) {
    gchar *ckey, *rkey;

    ckey = hardinfo_clean_label(key, 0);
    rkey = g_strdup_printf("%s:%s", "DTREE", ckey);

    dtree_info = h_strdup_cprintf("$%s%s$%s=\n", dtree_info,
        (report_details) ? "!" : "", rkey, ckey);
    moreinfo_add_with_prefix("DEV", rkey, g_strdup(value));

    g_free(ckey);
    g_free(rkey);
}

/*static void add_keys(dtr *dt, char *np) {
    gchar *dir_path, *dt_path;
    gchar *ftmp, *ntmp;
    gchar *n_info;
    const gchar *fn;
    GDir *dir;
    dtr_obj *obj;

    dir_path = g_strdup_printf("%s/%s", dtr_base_path(dt), np);
    dir = g_dir_open(dir_path, 0 , NULL);
    if(!dir){ // add self
        obj = dtr_obj_read(dt, np);
        dt_path = dtr_obj_path(obj);
        n_info = get_node(dt, dt_path);
        mi_add(dt_path, n_info, 0);
    }else { //dir
        while((fn = g_dir_read_name(dir)) != NULL) {
            ftmp = g_strdup_printf("%s/%s", dir_path, fn);
            if ( g_file_test(ftmp, G_FILE_TEST_IS_DIR) ) {
                if (strcmp(np, "/") == 0)
                    ntmp = g_strdup_printf("/%s", fn);
                else
                    ntmp = g_strdup_printf("%s/%s", np, fn);
                if(strlen(ntmp)>0) add_keys(dt, ntmp);
                g_free(ntmp);
            }
            g_free(ftmp);
        }
        g_dir_close(dir);
    }
    g_free(dir_path);
}*/

/*static char *msg_section(dtr *dt, int dump) {
    gchar *aslbl = NULL;
    gchar *messages = dtr_messages(dt);
    gchar *ret = g_strdup_printf("[%s]", _("Messages"));
    gchar **lines = g_strsplit(messages, "\n", 0);
    int i = 0;
    while(lines[i] != NULL) {
        aslbl = hardinfo_clean_label(lines[i], 0);
        ret = appfnl(ret, "%s=", aslbl);
        g_free(aslbl);
        i++;
    }
    g_strfreev(lines);
    if (dump)
        printf("%s", messages);
    g_free(messages);
    return ret;
}*/


/* kvl: 0 = key is label, 1 = key is v */
char* dtr_map_info_section(dtr *s, dtr_map *map, char *title, int kvl) {
    gchar *tmp, *ret;
    const gchar *sym;
    ret = g_strdup_printf("[%s]\n", _(title));
    dtr_map *it = map;
    while(it != NULL) {
        if (kvl) {
	    dtr_map *ali = s->symbols;
	    sym=NULL;
	    while(ali != NULL && sym==NULL) {
	        if (strcmp(ali->path, it->path) == 0){
		    sym=ali->label;
		    ali = ali->next;
	        }
	    }
            //sym = dtr_symbol_lookup_by_path(s, it->path);
            if (sym != NULL)
                tmp = g_strdup_printf("%s0x%x (%s)=%s\n", ret, it->v, sym, it->path);
            else
                tmp = g_strdup_printf("%s0x%x=%s\n", ret, it->v, it->path);
        } else
            tmp = g_strdup_printf("%s%s=%s\n", ret, it->label, it->path);
        g_free(ret);
        ret = tmp;
        it = it->next;
    }

    return ret;
}


void __scan_dtree()
{
    dtr *dt = dtr_new(NULL);
    gchar *summary = get_summary(dt);
    gchar *mem = dtr_map_info_section(dt,dt->phandles,_("Memory Map"),1);
    gchar *irq = dtr_map_info_section(dt,dt->irqs,_("IRQ Map"),1);
    gchar *alias = dtr_map_info_section(dt,dt->aliases,_("Alias Map"),0);
    gchar *symbol = dtr_map_info_section(dt,dt->symbols,_("Symbol Map"),0);
    //gchar *messages = NULL;

    dtree_info = g_strdup("[Device Tree]\n");
    mi_add("Summary", summary, 1);
    mi_add("Memory Map", mem, 0);
    mi_add("IRQ Map", irq, 0);
    mi_add("Alias Map", alias, 0);
    mi_add("Symbol Map", symbol, 0);

    //if(dtr_was_found(dt)) add_keys(dt, "/");
    //messages = msg_section(dt, 0);
    //mi_add("Messages", messages, 0);

    g_free(summary);
    g_free(mem);
    g_free(irq);
    g_free(alias);
    g_free(symbol);
    //g_free(messages);
    dtr_free(dt);
}
