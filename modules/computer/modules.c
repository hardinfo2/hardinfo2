/*
 *    HardInfo2 - System information and benchmark
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

#include <string.h>
#include <sys/utsname.h>
#include <json-glib/json-glib.h>

#include "syncmanager.h"
#include "computer.h"
#include "hardinfo.h"

GHashTable *_module_hash_table = NULL;
static JsonParser *parser=NULL;
static JsonObject *icons=NULL;

void kernel_module_icon_init(void)
{
    static SyncEntry sync_entry = {
        .name = N_("Update kernel module icon table"),
        .file_name = "kernel-module-icons.json",
	.optional = TRUE,
    };
    sync_manager_add_entry(&sync_entry);
}

static const gchar* get_module_icon(const char *modname, const char *path)
{
    if(!icons) return NULL;
    char *modname_temp = g_strdup(modname);
    char *p;
    for (p = modname_temp; *p; p++) {
        if (*p == '_') *p = '-';
    }
    //printf("LOOKUP %s\n",modname_temp);
    if(!json_object_has_member(icons, modname_temp)) return NULL;

    const gchar *icon=json_object_get_string_member(icons, modname_temp);
    g_free(modname_temp);
    //if(icon) printf("Found %s\n",(gchar *)icon); else printf("\n");
    return icon;
}

gint compar (gpointer a, gpointer b) {return strcmp( (char*)a, (char*)b );}

void scan_modules_do(void) {
    FILE *lsmod;
    gchar buffer[1024];
    gchar *lsmod_path;
    gchar *module_icons;
    GList *list=NULL,*a;
    const gchar *icon;
    gchar *icon_json=NULL;
    JsonNode *root=NULL;

    if(parser) g_object_unref(parser);
    if(icon_json) g_free(icon_json);

    icon_json = g_build_filename(g_get_user_config_dir(),"hardinfo2", "kernel-module-icons.json",NULL);
    if (!g_file_test(icon_json, G_FILE_TEST_EXISTS)){
        g_free(icon_json);
        icon_json = g_build_filename(params.path_data, "kernel-module-icons.json",NULL);
    }
    if (g_file_test(icon_json, G_FILE_TEST_EXISTS)){
      parser = json_parser_new();
      if (json_parser_load_from_file(parser, icon_json, NULL)){
	root = json_parser_get_root(parser);
	if (json_node_get_node_type(root) == JSON_NODE_OBJECT){
	  icons = json_node_get_object(root);
	}
      }
    }

    if (!_module_hash_table) { _module_hash_table = g_hash_table_new(g_str_hash, g_str_equal); }

    g_free(module_list);

    module_list = NULL;
    module_icons = NULL;
    moreinfo_del_with_prefix("COMP:MOD");

    lsmod_path = find_program("lsmod");
    if (!lsmod_path) return;
    lsmod = popen(lsmod_path, "r");
    if (!lsmod) {
        g_free(lsmod_path);
        return;
    }

    char *c=fgets(buffer, 1024, lsmod); /* Discards the first line */
    if(!c) return;
    //Sort modules
    while (fgets(buffer, 1024, lsmod)) {
        list=g_list_prepend(list,g_strdup(buffer));
    }
    list=g_list_sort(list,(GCompareFunc)compar);

    while (list) {
        gchar *buf, *strmodule, *hashkey;
        gchar *author = NULL, *description = NULL, *license = NULL, *deps = NULL, *vermagic = NULL,
              *filename = NULL, *srcversion = NULL, *version = NULL, *retpoline = NULL,
              *intree = NULL, modname[64];
        FILE *modi;
        glong memory;

        //if(!count--) {shell_status_pulse();count=100;}

        sscanf(list->data, "%s %ld", modname, &memory);

        hashkey = g_strdup_printf("MOD%s", modname);
        buf = g_strdup_printf("/sbin/modinfo %s 2>/dev/null", modname);

        modi = popen(buf, "r");
        while (fgets(buffer, 1024, modi) && strlen(buffer)) {
            gchar **tmp = g_strsplit(buffer, ":", 2);

	    if (!author && strstr(tmp[0], "author")) author = g_markup_escape_text(g_strstrip(tmp[1]), strlen(tmp[1]));
	    else if (!description && strstr(tmp[0], "description")) description = g_markup_escape_text(g_strstrip(tmp[1]), strlen(tmp[1]));
	    else if (!license && strstr(tmp[0], "license")) license = g_markup_escape_text(g_strstrip(tmp[1]), strlen(tmp[1]));
	    else if (!deps && strstr(tmp[0], "depends")) deps = g_markup_escape_text(g_strstrip(tmp[1]), strlen(tmp[1]));
	    else if (!vermagic && strstr(tmp[0], "vermagic")) vermagic = g_markup_escape_text(g_strstrip(tmp[1]), strlen(tmp[1]));
	    else if (!filename && strstr(tmp[0], "filename")) filename = g_markup_escape_text(g_strstrip(tmp[1]), strlen(tmp[1]));
	    else if (!srcversion && strstr(tmp[0], "srcversion")) srcversion = g_markup_escape_text(g_strstrip(tmp[1]), strlen(tmp[1]));
	    else if (!version && strstr(tmp[0], "version")) version = g_markup_escape_text(g_strstrip(tmp[1]), strlen(tmp[1]));
	    else if (!retpoline && strstr(tmp[0], "retpoline")) retpoline = g_markup_escape_text(g_strstrip(tmp[1]), strlen(tmp[1]));
	    else if (!intree && strstr(tmp[0], "intree")) intree = g_markup_escape_text(g_strstrip(tmp[1]), strlen(tmp[1]));

            g_strfreev(tmp);
        }
        pclose(modi);
        g_free(buf);

        if (description && g_str_equal(description, "&lt;none&gt;")) {
            g_free(description);
            description = g_strdup("");

            g_hash_table_insert(_module_hash_table, g_strdup(modname),
                                g_strdup_printf("Kernel module (%s)", modname));
        } else {
            g_hash_table_insert(_module_hash_table, g_strdup(modname), g_strdup(description));
        }

        /* append this module to the list of modules */
        module_list = h_strdup_cprintf("$%s$%s=%s\n", module_list, hashkey, modname,
                                       description ? description : "");
        icon = get_module_icon(modname, filename);
	module_icons = h_strdup_cprintf("Icon$%s$%s=%s.svg\n", module_icons, hashkey, modname, icon ? icon: "module");

        if(!filename) filename = g_strdup(_("(Not available)"));
        if(!description) description = g_strdup(_("(Not available)"));
        if(!vermagic) vermagic = g_strdup(_("(Not available)"));
        if(!author) author = g_strdup(_("(Not available)"));
        if(!license) license = g_strdup(_("(Not available)"));
        if(!version) version = g_strdup(_("(Not available)"));

        gboolean ry = FALSE, ity = FALSE;
        if (retpoline && *retpoline == 'Y') ry = TRUE;
        if (intree && *intree == 'Y') ity = TRUE;

        g_free(retpoline);
        g_free(intree);

        retpoline = g_strdup(ry ? _("Yes") : _("No"));
        intree = g_strdup(ity ? _("Yes") : _("No"));

        /* create the module information string */
        strmodule = g_strdup_printf("[%s]\n"
                                    "%s=%s\n"
                                    "%s=%.2f %s\n"
                                    "[%s]\n"
                                    "%s=%s\n"
                                    "%s=%s\n"
                                    "%s=%s\n"
                                    "%s=%s\n"
                                    "%s=%s\n"
                                    "%s=%s\n"
                                    "[%s]\n"
                                    "%s=%s\n"
                                    "%s=%s\n",
                                    _("Module Information"), _("Path"), filename, _("Used Memory"),
                                    memory / 1024.0, _("KiB"), _("Description"), _("Name"), modname,
                                    _("Description"), description, _("Version Magic"), vermagic,
                                    _("Version"), version, _("In Linus' Tree"), intree,
                                    _("Retpoline Enabled"), retpoline, _("Copyright"), _("Author"),
                                    author, _("License"), license);

        /* if there are dependencies, append them to that string */
        if (deps && strlen(deps)) {
            gchar **tmp = g_strsplit(deps, ",", 0);
            gchar *p=g_strjoinv("=\n", tmp);
            strmodule = h_strconcat(strmodule, "\n[", _("Dependencies"), "]\n", p, "=\n", NULL);
	    g_free(p);
            g_strfreev(tmp);
        }

        moreinfo_add_with_prefix("COMP", hashkey, strmodule);
        g_free(hashkey);

        g_free(author);
        g_free(description);
        g_free(license);
	g_free(deps);
        g_free(vermagic);
        g_free(filename);
        g_free(srcversion);
        g_free(version);
        g_free(retpoline);
        g_free(intree);

        //next and free
        a=list;
        list=list->next;
        free(a->data);
        g_list_free_1(a);
    }

    pclose(lsmod);

    g_free(lsmod_path);

    if (module_list != NULL && module_icons != NULL) {
        module_list = h_strdup_cprintf("[$ShellParam$]\n%s", module_list, module_icons);
    }
    g_free(module_icons);
}
