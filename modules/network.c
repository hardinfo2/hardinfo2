/*
 *    HardInfo - Displays System Information
 *    Copyright (C) 2003-2008 L. A. F. Pereira <l@tia.mat.br>
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <config.h>
#include <time.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <hardinfo.h>
#include <iconcache.h>
#include <shell.h>

#include <vendor.h>

#include "network.h"

/* Callbacks */
gchar *callback_network();
gchar *callback_route();
gchar *callback_dns();
gchar *callback_connections();
gchar *callback_shares();
gchar *callback_arp();
gchar *callback_statistics();

/* Scan callbacks */
void scan_network(gboolean reload);
void scan_route(gboolean reload);
void scan_dns(gboolean reload);
void scan_connections(gboolean reload);
void scan_shares(gboolean reload);
void scan_arp(gboolean reload);
void scan_statistics(gboolean reload);

static ModuleEntry entries[] = {
    {N_("Interfaces"), "network-interface.svg", callback_network, scan_network, MODULE_FLAG_NONE},
    {N_("IP Connections"), "network-connections.svg", callback_connections, scan_connections, MODULE_FLAG_NONE},
    {N_("Routing Table"), "route.svg", callback_route, scan_route, MODULE_FLAG_NONE},
    {N_("ARP Table"), "network-arp.svg", callback_arp, scan_arp, MODULE_FLAG_NONE},
    {N_("DNS Servers"), "internet.svg", callback_dns, scan_dns, MODULE_FLAG_NONE},
    {N_("Statistics"), "network-statistics.svg", callback_statistics, scan_statistics, MODULE_FLAG_NONE},
    {N_("Shared Directories"), "shares.svg", callback_shares, scan_shares, MODULE_FLAG_NONE},
    {NULL},
};

gchar** strsplit_multi(const gchar *string, const gchar *delimiter, gint max_tokens)
{
    const char *s;
    const gchar *remainder;
    gchar **string_list;
    //GPtrArray *string_list;
    int c=0;

    g_return_val_if_fail (string != NULL, NULL);
    g_return_val_if_fail (delimiter != NULL, NULL);
    g_return_val_if_fail (delimiter[0] != '\0', NULL);

    if (max_tokens < 1) {
        max_tokens = G_MAXINT;
	//string_list = g_ptr_array_new ();
    } //else {
      //string_list = g_ptr_array_new_full (max_tokens + 1, NULL);
    //}
    string_list=g_malloc((max_tokens+1)*sizeof(char*));

    remainder = string;
    while(strstr(remainder, delimiter)==remainder) {remainder++;}//Skip next - multi
    s = strstr(remainder, delimiter);
    if (s) {
        gsize delimiter_len = strlen (delimiter);
        while (--max_tokens && s)
        {
            gsize len;
	    len = s - remainder;
	    string_list[c++]=g_strndup (remainder, len); //g_ptr_array_add (string_list, g_strndup (remainder, len));
	    remainder = s + delimiter_len;
	    while(strstr(remainder, delimiter)==remainder) {remainder++;}//Skip next - multi
	    s = strstr (remainder, delimiter);
        }
    }
    if (*string) string_list[c++]=g_strdup (remainder); //g_ptr_array_add (string_list, g_strdup (remainder));
    string_list[c++]=NULL; //g_ptr_array_add (string_list, NULL);
    return string_list;//(char **) g_ptr_array_free (string_list, FALSE);
}


void scan_shares(gboolean reload)
{
    SCAN_START();
    scan_samba();
    scan_nfs_shared_directories();
    SCAN_END();
}

static gchar *__statistics = NULL;
void scan_statistics(gboolean reload)
{
    gboolean spawned;
    gchar *out=NULL,*err=NULL,*p,*netstat_path,*command_line=NULL,*names[6];
    int line = 0,ip=0;

    SCAN_START();

    g_free(__statistics);
    __statistics = g_strdup("");
    int i=0;while(i<6) names[i++]=NULL;

    if ((netstat_path = find_program("ip"))) {ip=1; command_line = g_strdup_printf("%s -s -s link show", netstat_path);}
    else if ((netstat_path = find_program("netstat"))) command_line = g_strdup_printf("%s -s", netstat_path);
    if(command_line){
    spawned = g_spawn_command_line_sync(command_line, &out, &err, NULL, NULL);

    if (spawned && out) {
        if(ip){
	    out=strreplace(out,"RX:","");
	    out=strreplace(out,"TX:","");
	    out=strreplace(out,"RX errors","");
	    out=strreplace(out,"TX errors","");
	}
        p=out;
        while (p && *p) {
	    if (!isspace(*p)) {
	        if(ip) if(strchr(p,' ')) p=strchr(p,' ');
		gchar *np=strchr(p,':');
		if(np) *np=0;
		gchar *tmp = g_ascii_strup(p, -1);
		__statistics = h_strdup_cprintf("[%s]\n", __statistics, tmp);
		g_free(tmp);
		if(np) *np=':';
		line=0;
	    } else {
		while (*p && isspace(*p)) p++;
		gchar *np=p;
		while (*np && (*np!='\n')) np++;
		if(*np && (*np=='\n')) {*np=0;} else np=NULL;
                if(ip){
		    if(strstr(p,"/")) {
		      // __statistics = h_strdup_cprintf(">#%d=%s\n", __statistics, line++, p);
		    } else {
		        if(strstr(p,":")){
			  gchar **sv=strsplit_multi(strstr(p,":")+1," ",6);
		          int i=0;while(i<6 && sv[i]) {
			      g_free(names[i]); names[i]=NULL;
			      if(sv[i]) names[i]=g_strdup(g_strstrip(sv[i]));
			      //if(names[i] && strlen(names[i])==0) {g_free(names[i]);names[i]=NULL;}
			      i++;
			  }
			  g_strfreev(sv);
			} else {
			  gchar **sv=strsplit_multi(p," ",6);
			  int i=0;while(i<6 && sv[i]) {if(names[i]) __statistics = h_strdup_cprintf(">#%d=%s\n", __statistics, line++, g_strconcat(names[i],": ",sv[i],NULL)); i++;}
			  g_strfreev(sv);
			}
		    }
		} else {
		    /* the bolded-space/dot used here is a hardinfo shell hack */
		    if (params.markup_ok)
		        __statistics = h_strdup_cprintf("<b> </b>#%d=%s\n", __statistics, line++, p);
		    else
		        __statistics = h_strdup_cprintf(">#%d=%s\n", __statistics, line++, p);
		}
		if(np) *np='\n';
	    }
	    p=strchr(p,'\n');
	    if(p && *p) p++;
	}
    }
    g_free(out);
    g_free(err);
    g_free(command_line);
    g_free(netstat_path);
    int i=0;while(i<6) g_free(names[i++]);
    }
    SCAN_END();
}

static gchar *__nameservers = NULL;
void scan_dns(gboolean reload)
{
    FILE *resolv;
    gchar buffer[256];
    gboolean spawned=FALSE;
    gchar *out=NULL,*err=NULL,*p,*resolvectl_path,*command_line=NULL,*ip;

    SCAN_START();

    g_free(__nameservers);
    __nameservers = g_strdup("");

    if ((resolvectl_path = find_program("resolvectl"))) command_line = g_strdup_printf("%s status", resolvectl_path);
    if(command_line){
      spawned = g_spawn_command_line_sync(command_line, &out, &err, NULL, NULL);
      if (spawned && out) {
        p=out;
	while (p && *p) {
	    gchar *np=strchr(p,'\n');
	    if(np) *np=0;
	    if(strstr(p,"DNS Servers:")){
		struct sockaddr_in sa;
		sa.sin_family = AF_INET;
		char hbuf[NI_MAXHOST];
	        ip=strstr(p,":")+2;
		while(ip && *ip){
                    gchar *np1=strchr(ip,' ');
		    if(np1) *np1=0;
		    inet_pton(AF_INET,ip,&sa.sin_addr.s_addr);
		    if (getnameinfo((struct sockaddr *)&sa, sizeof(sa), hbuf, sizeof(hbuf), NULL, 0, NI_NAMEREQD)) {
		        __nameservers = h_strdup_cprintf("%s=\n", __nameservers, ip);
		    } else {
		        __nameservers = h_strdup_cprintf("%s=%s\n", __nameservers, ip, hbuf);
		    }
		    if(np1) *np1=' ';
		    ip=strstr(ip," ");
		    if(ip && *ip) ip++;
		}
	    }
	    if(np) *np='\n';
	    p=strchr(p,'\n');
	    if(p && *p) p++;
	}
      }
      g_free(out);
      g_free(err);
      g_free(command_line);
      g_free(resolvectl_path);
    }

    if ((!spawned || (strlen(__nameservers)==0)) && (resolv = fopen("/etc/resolv.conf", "r"))) {
        while (fgets(buffer, 256, resolv)) {
	    if (g_str_has_prefix(buffer, "nameserver")) {
	        struct sockaddr_in sa;
		char hbuf[NI_MAXHOST];
		ip = g_strstrip(buffer + sizeof("nameserver"));
		sa.sin_family = AF_INET;
		inet_pton(AF_INET,ip,&sa.sin_addr.s_addr);

		if (getnameinfo((struct sockaddr *)&sa, sizeof(sa), hbuf, sizeof(hbuf), NULL, 0, NI_NAMEREQD)) {
		    __nameservers = h_strdup_cprintf("%s=\n", __nameservers, ip);
		} else {
		    __nameservers = h_strdup_cprintf("%s=%s\n", __nameservers, ip, hbuf);
		}

	    }
	}
	fclose(resolv);
    }

    SCAN_END();
}

void scan_network(gboolean reload)
{
    SCAN_START();
    scan_net_interfaces();
    SCAN_END();
}

static gchar *__routing_table = NULL;
void scan_route(gboolean reload)
{
    gboolean spawned;
    gchar *out=NULL,*err=NULL,*p,*route_path,*command_line=NULL;
    int ip=0;

    SCAN_START();

    g_free(__routing_table);
    __routing_table = g_strdup("");

    if ((route_path = find_program("ip"))) {ip=1;command_line = g_strdup_printf("%s route", route_path);}
    else if ((route_path = find_program("route"))) command_line = g_strdup_printf("%s -n", route_path);
    if(command_line){
    spawned = g_spawn_command_line_sync(command_line, &out, &err, NULL, NULL);

    if(spawned && out) {
	p=out;
	if(!ip){
	    /* eat first two lines */
            if(p) p=strstr(p,"\n");
	    if(p) p++;
	    if(p) p=strstr(p,"\n");
	    if(p) p++;
	}

        while (p && *p) {
	    gchar *np=strchr(p,'\n');
	    if(np) *np=0;
	    gchar **v=strsplit_multi(p," ",10);

	    if(v) {
	        if(!ip){
	            __routing_table = h_strdup_cprintf("%s / %s=%s|%s|%s\n",
                                             __routing_table,
					     v[0],//dest
					     v[1],//gateway
					     v[7],//interface
					     v[3],//flags
					     v[2]);//mask
	        } else {
		  if(!strstr(p,"via")){
		      if(strstr(p,"src")){
			  gchar **sv=g_strsplit(v[0],"/",2);
			  __routing_table = h_strdup_cprintf("%s / %s=%s|%s|/%s\n",
					     __routing_table,
					     sv[0],//dest
					     v[8],//gateway
					     v[2],//interface
					     "",//flags
					     sv[1]);//mask
			  g_strfreev(sv);
		       } else {
			  gchar **sv=g_strsplit(v[0],"/",2);
			  __routing_table = h_strdup_cprintf("%s=%s|%s|/%s\n",
					     __routing_table,
					     sv[0],//dest
					     v[2],//interface
					     "",//flags
					     sv[1]);//mask
			  g_strfreev(sv);
		       }
		    } else {
		        __routing_table = h_strdup_cprintf("%s / %s=%s|%s|/%s\n",
                                             __routing_table,
					     v[0],//dest
					     v[2],//gateway
					     v[4],//interface
					     "",//flags
					     "0");//mask
		    }
	        }
	    }
	    if(np) *np='\n';
	    g_strfreev(v);

	    p=strchr(p,'\n');
	    if(p && *p) p++;
        }
    }
    g_free(out);
    g_free(err);
    g_free(command_line);
    g_free(route_path);
    }
    SCAN_END();
}

static gchar *__arp_table = NULL;
void scan_arp(gboolean reload)
{
    FILE *arp;
    gchar buffer[256];

    SCAN_START();

    g_free(__arp_table);
    __arp_table = g_strdup("");

    if ((arp = fopen("/proc/net/arp", "r"))) {
      /* eat first line */
      char *c=fgets(buffer, 256, arp);

      if(c) while (fgets(buffer, 256, arp)) {
        buffer[15] = '\0';
        buffer[58] = '\0';

        __arp_table = h_strdup_cprintf("%s=%s|%s\n",
                                       __arp_table,
                                       g_strstrip(buffer),
                                       g_strstrip(buffer + 72),
                                       g_strstrip(buffer + 41));
      }

      fclose(arp);
    }

    SCAN_END();
}


static gchar *__connections = NULL;
void scan_connections(gboolean reload)
{
    gboolean spawned;
    gchar *out=NULL,*err=NULL,*p,*netstat_path,*command_line=NULL;

    SCAN_START();

    g_free(__connections);
    __connections = g_strdup("");

    if ((netstat_path = find_program("ss"))) command_line = g_strdup_printf("%s -antu", netstat_path);
    else if ((netstat_path = find_program("netstat"))) command_line = g_strdup_printf("%s -antu", netstat_path);
    if(command_line){
    spawned = g_spawn_command_line_sync(command_line, &out, &err, NULL, NULL);

    if(spawned && out) {
        out=strreplace(out,"%","-");
        out=strreplace(out,"[","(");
        out=strreplace(out,"]",")");
        p=out;
        while (p && *p) {
	    gchar *np=strchr(p,'\n');
	    if(np) *np=0;
	    gchar **v=strsplit_multi(p," ",6);

	    if (v && (g_str_has_prefix(p, "tcp") || g_str_has_prefix(p, "udp"))) {
	        if(strstr(command_line,"netstat")){
	            __connections = h_strdup_cprintf("%s=%s|%s|%s\n",
                                             __connections,
                                             v[3],	/* local address */
                                             v[0],	/* protocol */
					     v[4],	/* foreign address */
                                             v[5]);	/* state */
	        } else {
	            __connections = h_strdup_cprintf("%s=%s|%s|%s\n",
                                             __connections,
                                             v[4],	/* local address */
                                             v[0],	/* protocol */
					     v[5],	/* foreign address */
                                             v[1]);	/* state */
	        }
	    }
	    if(np) *np='\n';
	    g_strfreev(v);

	    p=strchr(p,'\n');
	    if(p && *p) p++;
        }
    }

    g_free(out);
    g_free(err);
    g_free(command_line);
    g_free(netstat_path);
    }
    SCAN_END();
}

gchar *callback_arp()
{
    return g_strdup_printf("[%s]\n"
                           "%s\n"
                           "[$ShellParam$]\n"
                           "ReloadInterval=3000\n"
                           "ColumnTitle$TextValue=%s\n" /* IP Address */
                           "ColumnTitle$Value=%s\n" /* Interface */
                           "ColumnTitle$Extra1=%s\n" /* MAC Address */
                           "ShowColumnHeaders=true\n",
                           _("ARP Table"), __arp_table,
                           _("IP Address"), _("Interface"), _("MAC Address") );
}

gchar *callback_shares()
{
    return g_strdup_printf("[%s]\n"
                "%s\n"
                "[%s]\n"
                "%s",
                _("SAMBA"), smb_shares_list,
                _("NFS"), nfs_shares_list);
}

gchar *callback_dns()
{
    return g_strdup_printf("[%s]\n"
                           "%s\n"
                           "[$ShellParam$]\n"
                           "ColumnTitle$TextValue=%s\n" /* IP Address */
                           "ColumnTitle$Value=%s\n" /* Name */
                           "ShowColumnHeaders=true\n",
                           _("Name Servers"), __nameservers,
                           _("IP Address"), _("Name") );
}

gchar *callback_connections()
{
    return g_strdup_printf("[%s]\n"
                           "%s\n"
                           "[$ShellParam$]\n"
                           "ReloadInterval=3000\n"
                           "ColumnTitle$TextValue=%s\n" /* Local Address */
                           "ColumnTitle$Value=%s\n" /* Protocol */
                           "ColumnTitle$Extra1=%s\n" /* Foreign Address */
                           "ColumnTitle$Extra2=%s\n" /* State */
                           "ShowColumnHeaders=true\n",
                           _("Connections"), __connections,
                           _("Local Address"), _("Protocol"), _("Foreign Address"), _("State") );
}

gchar *callback_network()
{
    return g_strdup_printf("%s\n"
               "[$ShellParam$]\n"
               "ReloadInterval=3000\n"
               "ViewType=1\n"
               "ColumnTitle$TextValue=%s\n" /* Interface */
               "ColumnTitle$Value=%s\n" /* IP Address */
               "ColumnTitle$Extra1=%s\n" /* Sent */
               "ColumnTitle$Extra2=%s\n" /* Received */
               "ShowColumnHeaders=true\n"
               "%s",
               network_interfaces,
               _("Interface"), _("IP Address"), _("Sent"), _("Received"),
               network_icons);
}

gchar *callback_route()
{
    return g_strdup_printf("[%s]\n"
                           "%s\n"
                           "[$ShellParam$]\n"
                           "ViewType=0\n"
                           "ReloadInterval=3000\n"
                           "ColumnTitle$TextValue=%s\n" /* Destination / Gateway */
                           "ColumnTitle$Value=%s\n" /* Interface */
                           "ColumnTitle$Extra1=%s\n" /* Flags */
                           "ColumnTitle$Extra2=%s\n" /* Mask */
                           "ShowColumnHeaders=true\n",
                           _("IP routing table"), __routing_table,
                           _("Destination/Gateway"), _("Interface"), _("Flags"), _("Mask") );
}

gchar *callback_statistics()
{
    return g_strdup_printf("%s\n"
                           "[$ShellParam$]\n"
                           "ReloadInterval=3000\n",
                            __statistics);
}

gchar *hi_more_info(gchar * entry)
{
    gchar *info = moreinfo_lookup_with_prefix("NET", entry);

    if (info)
	return g_strdup(info);

    return g_strdup_printf("[%s]", entry);
}

ModuleEntry *hi_module_get_entries(void)
{
    return entries;
}

gchar *hi_module_get_name(void)
{
    return _("Network");
}

guchar hi_module_get_weight(void)
{
    return 160;
}

void hi_module_init(void)
{
}

void hi_module_deinit(void)
{
    moreinfo_del_with_prefix("NET");

    g_free(smb_shares_list);
    g_free(nfs_shares_list);
    g_free(network_interfaces);
    g_free(network_icons);

    g_free(__statistics);
    g_free(__nameservers);
    g_free(__arp_table);
    g_free(__routing_table);
    g_free(__connections);
}

const ModuleAbout *hi_module_get_about(void)
{
    static const ModuleAbout ma = {
        .author = "L. A. F. Pereira",
        .description =
            N_("Gathers information about this computer's network connection"),
        .version = VERSION,
        .license = "GNU GPL version 2 or later.",
    };

    return &ma;
}
