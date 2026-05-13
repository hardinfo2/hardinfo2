/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2012 L. A. F. Pereira <l@tia.mat.br>
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

#include <sys/types.h>
#include <grp.h>
#include "hardinfo.h"
#include "computer.h"

gint comparGroups (gpointer a, gpointer b) {return strcmp( (char*)a, (char*)b );}

gchar *groups = NULL;
void scan_groups_do(void)
{
    struct group *group_;
    GList *list=NULL, *a;

    setgrent();
    group_ = getgrent();
    if (!group_)
        return;

    g_free(groups);
    groups = g_strdup("");

    while (group_) {
        gchar *members=NULL, **p;
        for(p=group_->gr_mem; *p != NULL; p++){
	    if(members)
	        members=h_strdup_cprintf(", %s", members, *p);
	    else
	        members=g_strdup(*p);
	}
	if(!members) members=g_strdup(_("None"));
        gchar *key = g_strdup_printf("GROUP%s", group_->gr_name);
        gchar *val = g_strdup_printf("[%s]\n"
		                     "%s=%s\n",
				     _("Group Information"),
				     _("Members"), members);

        list=g_list_prepend(list,g_strdup_printf("%s,%s,%d,%s", key, group_->gr_name, group_->gr_gid, val));
        group_ = getgrent();
    }
    
    endgrent();

    //sort
    list=g_list_sort(list,(GCompareFunc)comparGroups);

    while (list) {
        char **datas = g_strsplit(list->data,",",4);
	if(datas[0]){
            groups = h_strdup_cprintf("$%s$%s=%s\n", groups, datas[0], datas[1], datas[2]);
	    moreinfo_add_with_prefix("COMP", datas[0], g_strdup(datas[3]));
	}
        g_strfreev(datas);

        //next and free
        a=list;
        list=list->next;
        free(a->data);
        g_list_free_1(a);
    }
}
