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
 *
 * Some code from xfce4-mount-plugin, version 0.4.3
 *  Copyright (C) 2005 Jean-Baptiste jb_dul@yahoo.com
 *  Distributed under the terms of GNU GPL 2+ - 
 * https://gitlab.xfce.org/panel-plugins/xfce4-mount-plugin/-/blob/master/panel-plugin/mount-plugin.c
.*
 */

#include <string.h>
#include <sys/vfs.h>
#include "hardinfo.h"
#include "computer.h"

gchar *fs_list = NULL;

static gint filesystem_sort(gchar *linea, gchar *lineb)
{
    return g_strcmp0(linea,lineb);
    /*    gchar *sta=strstr(linea," /");
    gchar *stb=strstr(lineb," /");
    return g_strcmp0(sta?sta+1:NULL,stb?stb+1:NULL);*/
}

void scan_filesystems(void)
{
    gchar *buf;
    struct statfs sfs;
    int count = 0;
    GList *fs=NULL,*p;
    gchar **fslines;

    g_free(fs_list);
    fs_list = g_strdup("");
    moreinfo_del_with_prefix("COMP:FS");

    if(!g_file_get_contents("/etc/mtab", &buf, NULL, NULL)) return;
    buf=strreplace(buf,"\r","");
    fslines=g_strsplit(buf,"\n",0);
    g_free(buf);

    gint i=0; while(fslines && fslines[i] && strlen(fslines[i])) fs=g_list_append(fs, g_strdup(fslines[i++]));
    g_strfreev(fslines);

    fs=g_list_sort(fs,(GCompareFunc)filesystem_sort);
    p=fs;
    while (p && p->data) {
        gfloat size, used, avail;
        gchar **tmp=NULL;

        tmp = g_strsplit((gchar *)p->data, " ", 0);
        if(tmp && tmp[1]) if (!statfs(tmp[1], &sfs)) {
                gfloat use_ratio;

                size = (float) sfs.f_bsize * (float) sfs.f_blocks;
                avail = (float) sfs.f_bsize * (float) sfs.f_bavail;
                used = size - avail;

                if (size == 0.0f) {
		    p=p->next;
                    continue;
                }

                if (avail == 0.0f) {
                    use_ratio = 100.0f;
                } else {
                    use_ratio = 100.0f * (used / size);
                }

                gchar *strsize = size_human_readable(size),
                      *stravail = size_human_readable(avail),
                      *strused = size_human_readable(used);

                gchar *strhash;

                gboolean rw = strstr(tmp[3], "rw") != NULL;

                strreplacechr(tmp[0], "#", '_');
                strhash = g_strdup_printf("[%s]\n"
                        "%s=%s\n"
                        "%s=%s\n"
                        "%s=%s\n"
                        "%s=%s\n"
                        "%s=%s\n"
                        "%s=%s\n",
                        tmp[0], /* path */
                        _("Filesystem"), tmp[2],
                        _("Mounted As"), rw ? _("Read-Write") : _("Read-Only"),
                        _("Mount Point"), tmp[1],
                        _("Size"), strsize,
                        _("Used"), strused,
                        _("Available"), stravail);
                gchar *key = g_strdup_printf("FS%d", ++count);
                moreinfo_add_with_prefix("COMP", key, strhash);
                g_free(key);

                fs_list = h_strdup_cprintf("$FS%03d$%s%s=%.2f %% (%s of %s)|%s\n",
                                          fs_list,
                                          count, tmp[0], rw ? "" : "🔒",
                                          use_ratio, stravail, strsize, tmp[1]);
                g_free(strsize);
                g_free(stravail);
                g_free(strused);
        }
        g_strfreev(tmp);tmp=NULL;
	p=p->next;
    }
    g_list_free(fs);
}
