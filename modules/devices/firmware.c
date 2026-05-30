/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2019 L. A. F. Pereira <l@tia.mat.br>
 *    Copyright (C) 2019 Burt P. <pburt0@gmail.com>
 *    Copyright (C) 2020 Ondrej Čerman
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
#include <inttypes.h>
#include <gio/gio.h>
#include "util_sysobj.h" /* for SEQ() and appf() */

#define FWUPT_INTERFACE  "org.freedesktop.fwupd"
gboolean fail_no_fwupd = TRUE;

char *decode_flags(guint64 flags) {
    /* https://github.com/hughsie/fwupd/blob/master/libfwupd/fwupd-enums.{h,c} */
    static const struct { guint64 b; char *flag, *def; } flag_defs[] = {
        { (1u << 0),  "internal", N_("Device cannot be removed easily") },
        { (1u << 1),  "updatable", N_("Device is updatable in this or any other mode") },
        { (1u << 2),  "only-offline", N_("Update can only be done from offline mode") },
        { (1u << 3),  "require-ac", N_("Requires AC power") },
        { (1u << 4),  "locked", N_("Is locked and can be unlocked") },
        { (1u << 5),  "supported", N_("Is found in current metadata") },
        { (1u << 6),  "needs-bootloader", N_("Requires a bootloader mode to be manually enabled by the user") },
        { (1u << 7),  "registered", N_("Has been registered with other plugins") },
        { (1u << 8),  "needs-reboot", N_("Requires a reboot to apply firmware or to reload hardware") },
        { (1u << 17), "needs-shutdown", N_("Requires system shutdown to apply firmware") },
        { (1u << 9),  "reported", N_("Has been reported to a metadata server") },
        { (1u << 10), "notified", N_("User has been notified") },
        { (1u << 11), "use-runtime-version", N_("Always use the runtime version rather than the bootloader") },
        { (1u << 12), "install-parent-first", N_("Install composite firmware on the parent before the child") },
        { (1u << 13), "is-bootloader", N_("Is currently in bootloader mode") },
        { (1u << 14), "wait-for-replug", N_("The hardware is waiting to be replugged") },
        { (1u << 15), "ignore-validation", N_("Ignore validation safety checks when flashing this device") },
        { (1u << 18), "another-write-required", N_("Requires the update to be retried with a new plugin") },
        { (1u << 19), "no-auto-instance-ids", N_("Do not add instance IDs from the device baseclass") },
        { (1u << 20), "needs-activation", N_("Device update needs to be separately activated") },
        { (1u << 21), "ensure-semver", N_("Ensure the version is a valid semantic version, e.g. numbers separated with dots") },
        { (1u << 16), "trusted", N_("Extra metadata can be exposed about this device") },
        {  0, NULL }
    };

    gchar *flag_list = g_strdup("");

    int i;
    for (i = 0; flag_defs[i].flag; i++) {
        if (flags & flag_defs[i].b)
            flag_list = appfnl(flag_list, "[%s] %s", flag_defs[i].flag, flag_defs[i].def);
    }
    return flag_list;
}

const char *find_translation(const char *str) {
    /* TODO: https://github.com/hughsie/fwupd/blob/master/src/README.md */
    static const char *translatable[] = {
        N_("DeviceId"), N_("Guid"), N_("Summary"), N_("Plugin"), N_("Flags"),
        N_("Vendor"), N_("VendorId"), N_("Version"), N_("VersionBootloader"),
        N_("Icon"), N_("InstallDuration"), N_("Created"), N_("Checksum"),
	N_("Protocol"),
        NULL
    };
    int i;
    if (!str) return NULL;
    for (i = 0; translatable[i]; i++) {
        if (SEQ(str, translatable[i]))
            return _(translatable[i]);
    }
    DEBUG("firmware.c - Translation missing %s",str);
    return g_strdup(str);
};

/* map lvfs icon names to hardinfo icon names */
const char *find_icon(const char *lvfs_name) {
    /* icon names found by looking for fu_device_add_icon ()
     * in the fwupd source. */
    static const
    struct { char *lvfs, *hi; } imap[] = {
        { "application-certificate", "security" },
        { "applications-internet", "internet" },
        { "audio-card", "audio" },
        { "computer", "computer" },
        { "drive-harddisk", "hdd" },
        { "input-gaming", "joystick" },
	//{ "input-tablet", NULL },
        { "network-modem", "wireless" },
        { "preferences-desktop-keyboard", "keyboard" },
        //{ "thunderbolt", NULL },
        //{ "touchpad-disabled", NULL },
        /* default */
        { NULL, "memory" } /* a device with firmware maybe */
    };
    unsigned int i = 0;
    for(; imap[i].lvfs; i++) {
        if (SEQ(imap[i].lvfs, lvfs_name) && imap[i].hi)
            return imap[i].hi;
    }
    return imap[i].hi;
}

#ifndef G_GNUC_FALLTHROUGH
#define G_GNUC_FALLTHROUGH ;
#endif
#if GLIB_CHECK_VERSION(2,35,0)
#define g2_variant_check_format_string g_variant_check_format_string
#else
gboolean g2_variant_check_format_string (GVariant    *value,
                               const gchar *format_string,
                               gboolean     copy_only)
{
  const gchar *original_format = format_string;
  const gchar *type_string;
  /* Interesting factoid: assuming a format string is valid, it can be
   * converted to a type string by removing all '@' '&' and '^'
   * characters.
   *
   * Instead of doing that, we can just skip those characters when
   * comparing it to the type string of @value.
   *
   * For the copy-only case we can just drop the '&' from the list of
   * characters to skip over.  A '&' will never appear in a type string
   * so we know that it won't be possible to return %TRUE if it is in a
   * format string.
   */
  type_string = g_variant_get_type_string (value);
  while (*type_string || *format_string)
    {
      gchar format = *format_string++;
      switch (format)
        {
        case '&':
          if G_UNLIKELY (copy_only)
            {
              /* for the love of all that is good, please don't mark this string for translation... */
              g_critical ("g_variant_check_format_string() is being called by a function with a GVariant varargs "
                          "interface to validate the passed format string for type safety.  The passed format "
                          "(%s) contains a '&' character which would result in a pointer being returned to the "
                          "data inside of a GVariant instance that may no longer exist by the time the function "
                          "returns.  Modify your code to use a format string without '&'.", original_format);
              return FALSE;
            }
          G_GNUC_FALLTHROUGH;
        case '^':
        case '@':
          /* ignore these 2 (or 3) */
          continue;
        case '?':
          /* attempt to consume one of 'bynqiuxthdsog' */
          {
            char s = *type_string++;
            if (s == '\0' || strchr ("bynqiuxthdsog", s) == NULL)
              return FALSE;
          }
          continue;
        case 'r':
          /* ensure it's a tuple */
          if (*type_string != '(')
            return FALSE;
          G_GNUC_FALLTHROUGH;
        case '*':
          /* consume a full type string for the '*' or 'r' */
          if (!g_variant_type_string_scan (type_string, NULL, &type_string))
            return FALSE;
          continue;
        default:
          /* attempt to consume exactly one character equal to the format */
          if (format != *type_string++)
            return FALSE;
        }
    }
  return TRUE;
}
#endif

gint compareFW (gpointer a, gpointer b) {return strcmp( (char*)a, (char*)b );}


gchar *fwupdmgr_get_devices_info() {
    gchar *info = NULL;
    gchar *this_group = NULL, *icon_list=NULL;
    gboolean has_vendor_field = FALSE;
    gboolean updatable = FALSE;
    const Vendor *gv = NULL;
    GList *list=NULL, *a;

    GDBusConnection *conn;
    GDBusProxy *proxy;
    GVariant *devices, *value;
    GVariantIter *deviter, *dictiter, *iter;
    const gchar *key, *tmpstr;
    int t=0;

    moreinfo_del_with_prefix("DEV:FW");

    conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
    if (!conn) {g_free(info); return g_strdup("");}

    proxy = g_dbus_proxy_new_sync(conn, G_DBUS_PROXY_FLAGS_NONE, NULL,
                                  FWUPT_INTERFACE, "/", FWUPT_INTERFACE,
                                  NULL, NULL);
    if (!proxy) {
        g_object_unref(conn);
	g_free(info);
        return g_strdup("");
    }

    fail_no_fwupd = FALSE;
    devices = g_dbus_proxy_call_sync(proxy, "GetDevices", NULL,
                                     G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);

    if (devices && g2_variant_check_format_string(devices, "(aa{sv})", FALSE) ) {
        g_variant_get(devices, "(aa{sv})", &deviter);
        while(g_variant_iter_loop(deviter, "a{sv}", &dictiter)){

            has_vendor_field = FALSE;
            updatable = FALSE;
            gv = NULL;
	    if(info) {g_free(info);info=NULL;}
	    info=g_strdup("");

            while (g_variant_iter_loop(dictiter, "{&sv}", &key, &value)) {
                if (SEQ(key, "Name")) {
                    tmpstr = g_variant_get_string(value, NULL);
                    this_group = hardinfo_clean_grpname(tmpstr, 0);
                    gv = vendor_match(tmpstr, NULL);
                } else if (SEQ(key, "Vendor")) {
                    has_vendor_field = TRUE;
                    tmpstr = g_variant_get_string(value, NULL);

                    const Vendor* v = vendor_match(tmpstr, NULL);
                    if (v) {
		        info=h_strconcat(info, _("Vendor"), "=", v->name, "\n", NULL);
                    } else {
		        info=h_strconcat(info, _("Vendor"), "=", tmpstr, "\n", NULL);
                    }
                } else if (SEQ(key, "Icon")) {
                    g_variant_get(value, "as", &iter);
                    while (g_variant_iter_loop(iter, "s", &tmpstr)) {
		        info=h_strconcat(info, _("Icon"), "=", tmpstr, "\n", NULL);
                    }
                } else if (SEQ(key, "Guid")) {
                    g_variant_get(value, "as", &iter);
                    gint guid_index = 1;
                    while (g_variant_iter_loop(iter, "s", &tmpstr)) {
                        gchar *guid_key = g_strdup_printf("%s%d", _("Guid"), guid_index++);
                        info = h_strconcat(info, guid_key, "=", tmpstr, "\n", NULL);
                        g_free(guid_key);
                    }
                    g_variant_iter_free(iter);
                } else if (SEQ(key, "Created")) {
                    guint64 created = g_variant_get_uint64(value);
                    GDateTime *dt = g_date_time_new_from_unix_local(created);
		    gchar *s;
                    if (dt) {
		        info=h_strconcat(info, _("Created"), "=", (s=g_date_time_format(dt, "%x")), "\n", NULL);
			g_free(s);
                        g_date_time_unref(dt);
                    }
                } else if (SEQ(key, "Flags")) {
                    guint64 flags = g_variant_get_uint64(value);
                    updatable = (gboolean)(flags & (1u << 1));
		    info=h_strconcat(info, _("Flags"), "=", decode_flags(flags), "\n", NULL);
                } else {
                    if (g_variant_is_of_type(value, G_VARIANT_TYPE_STRING)) {
		        info=h_strconcat(info, find_translation(key), "=", g_variant_dup_string(value, NULL), "\n", NULL);
                    }
                }
            }

            if (gv && !has_vendor_field) {
	        info=h_strconcat(info, _("Vendor"), gv->name, NULL);
            }

            // hide devices that are not updatable
            if (updatable) {
	        gchar *fwkey = g_strdup_printf("FW%d",t++);
	        list=g_list_prepend(list, g_strdup_printf("%s,%s,%s", this_group, fwkey, info));
		g_free(fwkey);
            }
	    g_free(info); info=NULL;
	    g_free(this_group); this_group=NULL;
        }
        g_variant_iter_free(deviter);
        g_variant_unref(devices);
    }

    g_object_unref(proxy);
    g_object_unref(conn);


    gchar *firmware=NULL, *ret=NULL, *s;
    if (list) {
        firmware=g_strdup_printf("[%s]\n",_("Firmware"));
        //sort
        list=g_list_sort(list,(GCompareFunc)compareFW);
        while (list) {
	    char **datas = g_strsplit(list->data,",",3);
	    if (datas[0]) {
	        datas[2]=strreplace(datas[2],"\\","");
	        firmware = h_strdup_cprintf("$!%s$=%s\n", firmware, datas[1], datas[0]);
	        moreinfo_add_with_prefix("DEV", datas[1], g_strconcat("[", _("Firmware"), "]\n", _("Firmware"), "=", datas[0], "\n", datas[2], NULL));
		if( (s=strstr(datas[2],_("Icon"))) ){
		    s+=5;
		    strend(s,'\n');
		    icon_list=h_strdup_cprintf("Icon$%s$=%s.svg\n", icon_list, datas[1], find_icon(s));
		}
	    }
	    g_strfreev(datas);

	    //next and free
	    a=list;
	    list=list->next;
	    free(a->data);
	    g_list_free_1(a);
	}

	ret = g_strconcat(firmware, "[$ShellParam$]\n", "ViewType=1\n", icon_list, NULL);
	g_free(firmware);
    } else {
        ret = g_strdup_printf("[%s]\n%s=%s\n" "[$ShellParam$]\nViewType=0\n",
                _("Firmware List"),
                _("Result"), _("(Not available)") );
    }
    return ret;
}

gchar *firmware_get_info(void) {
    return fwupdmgr_get_devices_info();
}

gchar *firmware_hinote(void) {
    if (fail_no_fwupd) {
        return _("Requires the <i><b>fwupd</b></i> daemon.");
    }
    return NULL;
}
