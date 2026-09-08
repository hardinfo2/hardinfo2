/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2017 L. A. F. Pereira <l@tia.mat.br>
 *    This file
 *    Copyright (C) 2018 Burt P. <pburt0@gmail.com>
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
#include <sys/utsname.h>
#include "hardinfo.h"
#include "pci_util.h"
#include "util_ids.h"

gchar *pci_ids_file = NULL;
GTimer *pci_ids_timer = NULL;

const gchar *find_pci_ids_file() {
    if (pci_ids_file) {
        if (!strstr(pci_ids_file, ".min"))
            return pci_ids_file;
    }
    char *file_search_order[] = {
        g_build_filename(g_get_user_config_dir(), "hardinfo2", "pci.ids", NULL),
        g_build_filename(params.path_data, "pci.ids", NULL),
        NULL
    };
    int n;
    for(n = 0; file_search_order[n]; n++) {
        if (!pci_ids_file && !access(file_search_order[n], R_OK))
            pci_ids_file = file_search_order[n];
        else
            g_free(file_search_order[n]);
    }
    DEBUG("find_pci_ids_file() result: %s", pci_ids_file);
    return pci_ids_file;
}

char *pci_lookup_ids_vendor_str(uint32_t id) {
    gchar *ret = NULL;

    ids_query_result result;
    gchar *qpath;
    if (!find_pci_ids_file())
        return FALSE;

    qpath = g_strdup_printf("%04x", id);
    scan_ids_file(pci_ids_file, qpath, &result, -1);
    if (result.results[0]) {
        ret = g_strdup(result.results[0]);
    }
    g_free(qpath);

    return ret;
}


static pcid *lastd=NULL;
void pci_cleanup(void){
    if(lastd){
        g_free(lastd->vendor_id_str);
        g_free(lastd->device_id_str);
        g_free(lastd->sub_device_id_str);
        g_free(lastd->sub_vendor_id_str);
	g_free(lastd->class_str);
	g_free(lastd);
	lastd=NULL;
    }
}
static gboolean pci_lookup_ids(pcid *d) {
    gboolean ret = FALSE;
    gchar *qpath;
    ids_query_result result;

    if (!find_pci_ids_file()) return FALSE;

    //check if last was the same to speedup for lots of same devices (Virtual Machines)
    if( lastd && (d->vendor_id==lastd->vendor_id) && (d->device_id==lastd->device_id) && (d->sub_vendor_id==lastd->sub_vendor_id) ){
        if(lastd->vendor_id_str) d->vendor_id_str = g_strdup(lastd->vendor_id_str);
        if(lastd->device_id_str) d->device_id_str = g_strdup(lastd->device_id_str);
        if(lastd->sub_device_id_str) d->sub_device_id_str = g_strdup(lastd->sub_device_id_str);
        if(lastd->sub_vendor_id_str) d->sub_vendor_id_str = g_strdup(lastd->sub_vendor_id_str);
        if(lastd->class_str) d->class_str = g_strdup(lastd->class_str);
	return TRUE;
    }

    /* lookup vendor, device, sub device */
    qpath = g_strdup_printf("%04x/%04x/%04x %04x",
        d->vendor_id, d->device_id, d->sub_vendor_id, d->sub_device_id);
    scan_ids_file(pci_ids_file, qpath, &result, -1);
    if (result.results[0]) {
        if (d->vendor_id_str) g_free(d->vendor_id_str);
        d->vendor_id_str = g_strdup(result.results[0]);
        ret = TRUE;
    }
    if (result.results[1]) {
        if (d->device_id_str) g_free(d->device_id_str);
        d->device_id_str = g_strdup(result.results[1]);
        ret = TRUE;
    }
    if (result.results[2]) {
        if (d->sub_device_id_str) g_free(d->sub_device_id_str);
        d->sub_device_id_str = g_strdup(result.results[2]);
        ret = TRUE;
    }
    g_free(qpath);

    /* lookup sub vendor by itself */
    qpath = g_strdup_printf("%04x", d->sub_vendor_id);
    scan_ids_file(pci_ids_file, qpath, &result, -1);
    if (result.results[0]) {
        if (d->sub_vendor_id_str) g_free(d->sub_vendor_id_str);
        d->sub_vendor_id_str = g_strdup(result.results[0]);
        ret = TRUE;
    };
    g_free(qpath);

    /* lookup class */
    qpath = g_strdup_printf("C %02x/%02x", (d->class >> 8) & 0xff, (d->class & 0xff));
    scan_ids_file(pci_ids_file, qpath, &result, -1);
    if (result.results[0]) {
        if (d->class_str) g_free(d->class_str);
        d->class_str = g_strdup(result.results[0]);
        if (result.results[1]
            && !SEQ(result.results[0], result.results[1]) ) {
                /* options 1: results[0] + " :: " + results[1] */
                //d->class_str = appf(d->class_str, " :: ", "%s", result.results[1]);

                /* option 2: results[1] or results[0] */
                g_free(d->class_str);
                d->class_str = g_strdup(result.results[1]);
        }
        ret = TRUE;
    }
    g_free(qpath);

    if(lastd){
        g_free(lastd->vendor_id_str);
        g_free(lastd->device_id_str);
        g_free(lastd->sub_device_id_str);
        g_free(lastd->sub_vendor_id_str);
	g_free(lastd->class_str);
	g_free(lastd);
	lastd=NULL;
    }
    if(!lastd) lastd=g_malloc0(sizeof(pcid));
    if(lastd){
        memcpy(lastd,d,sizeof(pcid));
        //
        if(d->vendor_id_str) lastd->vendor_id_str = g_strdup(d->vendor_id_str);
        if(d->device_id_str) lastd->device_id_str = g_strdup(d->device_id_str);
        if(d->sub_device_id_str) lastd->sub_device_id_str = g_strdup(d->sub_device_id_str);
        if(d->sub_vendor_id_str) lastd->sub_vendor_id_str = g_strdup(d->sub_vendor_id_str);
        if(d->class_str) lastd->class_str = g_strdup(d->class_str);
    }
    return ret;
}

gint pcid_cmp_by_addy(gconstpointer a, gconstpointer b)
{
    const struct pcid *dev_a = a;
    const struct pcid *dev_b = b;

    if (!dev_a)
        return !!dev_b;
    if (!dev_b)
        return !!dev_a;

    return g_strcmp0(dev_a->slot_str, dev_b->slot_str);
}

void pcid_free(pcid *s) {
    if (s) {
        g_free(s->slot_str);
        g_free(s->class_str);
        g_free(s->vendor_id_str);
        g_free(s->device_id_str);
        g_free(s->sub_vendor_id_str);
        g_free(s->sub_device_id_str);
        g_free(s->driver);
        g_free(s->driver_list);
        g_free(s);
    }
}

static void pci_fill_details(pcid *s) {
    {
        gchar *pci_loc = pci_address_str(s->domain, s->bus, s->device, s->function);
        gchar *sysfs_path = g_strdup_printf("%s/%s/driver", SYSFS_PCI_ROOT, pci_loc);
        gchar *driver_path = realpath(sysfs_path, NULL);
        if (driver_path) {
            s->driver = g_strdup(g_path_get_basename(driver_path));
            g_free(driver_path);
        }
        g_free(sysfs_path);

        sysfs_path = g_strdup_printf("%s/%s/modalias", SYSFS_PCI_ROOT, pci_loc);
        gchar *modalias = NULL;
        struct utsname uts;
        if (g_file_get_contents(sysfs_path, &modalias, NULL, NULL)
            && uname(&uts) == 0) {
            g_strstrip(modalias);
            gchar *mpath = g_strdup_printf("/lib/modules/%s/modules.alias", uts.release);
            gchar *mcontents = NULL;
            GSList *mods = NULL;
            if (g_file_get_contents(mpath, &mcontents, NULL, NULL)) {
                gchar **lines = g_strsplit(mcontents, "\n", -1);
                for (int i = 0; lines[i]; i++) {
                    gchar *l = lines[i];
                    if (!g_str_has_prefix(l, "alias pci:")) continue;
                    gchar *p = l + 6, *e = p;
                    while (*e && *e != ' ' && *e != '\t') e++;
                    if (*e) {
                        gchar *pattern = g_strndup(p, e - p);
                        while (*e == ' ' || *e == '\t') e++;
                        if (*e && g_pattern_match_simple(pattern, modalias)
                            && !g_slist_find_custom(mods, e, (GCompareFunc)g_strcmp0))
                            mods = g_slist_append(mods, g_strdup(e));
                        g_free(pattern);
                    }
                }
                g_strfreev(lines);
                g_free(mcontents);
            }
            if (mods) {
                GString *mlist = g_string_new(NULL);
                for (GSList *m = mods; m; m = m->next)
                    g_string_append_printf(mlist, "%s%s", (m == mods) ? "" : ", ", (char *)m->data);
                s->driver_list = g_string_free(mlist, FALSE);
                g_slist_free_full(mods, g_free);
            }
            g_free(mpath);
        }
        if (!s->driver_list) {
            gchar *mod_sysfs = g_strdup_printf("%s/%s/driver/module", SYSFS_PCI_ROOT, pci_loc);
            gchar *mod_path = realpath(mod_sysfs, NULL);
            if (mod_path) {
                s->driver_list = g_strdup(g_path_get_basename(mod_path));
                g_free(mod_path);
            }
            g_free(mod_sysfs);
        }
        g_free(modalias);
        g_free(sysfs_path);
        g_free(pci_loc);
    }
}

char *pci_address_str(uint32_t dom, uint32_t bus, uint32_t  dev, uint32_t func) {
    return g_strdup_printf("%04x:%02x:%02x.%01x", dom, bus, dev, func);
}

/* /sys/bus/pci/devices/0000:01:00.0/ */
char *_sysfs_bus_pci(uint32_t dom, uint32_t bus, uint32_t dev, uint32_t func, const char *item) {
    char *ret = NULL, *pci_loc, *sysfs_path;
    pci_loc = pci_address_str(dom, bus, dev, func);
    sysfs_path = g_strdup_printf("%s/%s/%s", SYSFS_PCI_ROOT, pci_loc, item);
    g_file_get_contents(sysfs_path, &ret, NULL, NULL);
    g_free(pci_loc);
    g_free(sysfs_path);
    return ret;
}

gboolean _sysfs_bus_pci_read_hex(uint32_t dom, uint32_t bus, uint32_t dev, uint32_t func, const char *item, uint32_t *val) {
    char *tmp = _sysfs_bus_pci(dom, bus, dev, func, item);
    uint32_t tval;
    if (tmp && val) {
        int ec = sscanf(tmp, "%x", &tval);
        g_free(tmp);
        if (ec==1) {
            *val = tval;
            return TRUE;
        }
    }
    return FALSE;
}

/* https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-bus-pci */
static gboolean pci_get_device_sysfs(uint32_t dom, uint32_t bus, uint32_t dev, uint32_t func, pcid *s) {
    char *tmp = NULL;
    int ec = 0;
    float tf;
    s->domain = dom;
    s->bus = bus;
    s->device = dev;
    s->function = func;
    s->slot_str = s->slot_str ? s->slot_str : pci_address_str(dom, bus, dev, func);
    if (! _sysfs_bus_pci_read_hex(dom, bus, dev, func, "class", &s->class) )
        return FALSE;
    s->class >>= 8; /* TODO: find out why */
    _sysfs_bus_pci_read_hex(dom, bus, dev, func, "device", &s->device_id);
    _sysfs_bus_pci_read_hex(dom, bus, dev, func, "vendor", &s->vendor_id);
    _sysfs_bus_pci_read_hex(dom, bus, dev, func, "subsystem_device", &s->sub_device_id);
    _sysfs_bus_pci_read_hex(dom, bus, dev, func, "subsystem_vendor", &s->sub_vendor_id);
    _sysfs_bus_pci_read_hex(dom, bus, dev, func, "revision", &s->revision);

    tmp = _sysfs_bus_pci(dom, bus, dev, func, "max_link_speed");
    if (tmp) {
        ec = sscanf(tmp, "%f GT/s", &tf);
        if (ec==1) s->pcie_speed_max = tf;
        g_free(tmp);
    }
    tmp = _sysfs_bus_pci(dom, bus, dev, func, "current_link_speed");
    if (tmp) {
        ec = sscanf(tmp, "%f GT/s", &tf);
        if (ec==1) s->pcie_speed_curr = tf;
        g_free(tmp);
    }
    tmp = _sysfs_bus_pci(dom, bus, dev, func, "max_link_width");
    if (tmp) {
        s->pcie_width_max = strtoul(tmp, NULL, 0);
        g_free(tmp);
    }
    tmp = _sysfs_bus_pci(dom, bus, dev, func, "current_link_width");
    if (tmp) {
        s->pcie_width_curr = strtoul(tmp, NULL, 0);
        g_free(tmp);
    }
    return TRUE;
}


pcid *pci_get_device_str(const char *addy) {
    uint32_t dom, bus, dev, func;
    int ec;
    if (addy) {
        ec = sscanf(addy, "%x:%x:%x.%x", &dom, &bus, &dev, &func);
        if (ec == 4) {
            return pci_get_device(dom, bus, dev, func);
        }
    }
    return NULL;
}

pcid *pci_get_device(uint32_t dom, uint32_t bus, uint32_t dev, uint32_t func) {
    pcid *s = pcid_new();
    gboolean ok = FALSE;
    if (s) {
        ok = pci_get_device_sysfs(dom, bus, dev, func, s);
        if (ok) {
            ok |= pci_lookup_ids(s);
            //if (!ok) ok |= pci_get_device_lspci(dom, bus, dev, func, s);
        }
        if (!ok) {
            pcid_free(s);
            s = NULL;
        }
    }
    return s;
}

static pcid_list pci_get_device_list_sysfs(uint32_t class_min, uint32_t class_max) {
    pcid_list dl = NULL;
    pcid *nd;
    uint32_t dom, bus, dev, func, cls;
    int ec;

    if (class_max == 0) class_max = 0xffff;

    const gchar *f = NULL;
    GDir *d = g_dir_open("/sys/bus/pci/devices", 0, NULL);
    if (!d) return 0;

    while((f = g_dir_read_name(d))) {
        ec = sscanf(f, "%x:%x:%x.%x", &dom, &bus, &dev, &func);
        if (ec == 4) {
            gchar *cf = g_strdup_printf("/sys/bus/pci/devices/%s/class", f);
            gchar *cstr = NULL;
            if (g_file_get_contents(cf, &cstr, NULL, NULL) ) {
                cls = strtoul(cstr, NULL, 16) >> 8;
                if (cls >= class_min && cls <= class_max) {
                    nd = pci_get_device(dom, bus, dev, func);
                    pci_fill_details(nd);
                    dl = g_slist_append(dl, nd);
                }
            }
            g_free(cstr);
            g_free(cf);
        }
    }
    g_dir_close(d);
    return dl;
}

pcid_list pci_get_device_list(uint32_t class_min, uint32_t class_max) {
    pcid_list dl = NULL;
    dl = pci_get_device_list_sysfs(class_min, class_max);
    return dl;
}

