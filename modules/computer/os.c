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

#include <gdk/gdkx.h>
#include <string.h>
#include <ctype.h>
#include <sys/utsname.h>
#include "hardinfo.h"
#include "computer.h"
#include "util_sysobj.h" /* for appfsp() */

//find distro for debian/ubuntu bases
typedef struct {
    const char *name;
    const char *id;
    const char *aptname;
    const char *filename;
    const char *versiontext;
} AptFlavor;

static const AptFlavor apt_flavors[] = {
    { "Puppy Linux",    "puppy",           "/etc/DISTRO_SPECS",            "/etc/DISTRO_SPECS",          "DISTRO_VERSION="      },
    { "Winux",          "winux",           "/usr/bin/winux-driver-manager","/etc/os-release",            "PRETTY_NAME=\"Winux " },
    { "Vendefoul Wolf", "Vendefoul",       "/opt/vendefoulwolf.png",       "/etc/os-release",            "VERSION_ID="          },
    { "Parrot Security","parrot",          "parrot-updater",               "/etc/os-release",            "VERSION_ID="          },
    { "Bodhi Linux",    "bodhi",           "bodhi-appcenter",              "/etc/bodhi/info",            "RELEASE="             },
    { "MX Linux",       "mxlinux",         "mx-welcome",                   "/etc/mx-version",            "MX-"                  },
    { "Raspbian",       "raspbian",        "raspbian-archive-keyring",     "/etc/os-release",            "VERSION_ID="          },
    { "Armbian",        "armbian",         "armbian-config",               "/etc/armbian-image-release", "VERSION="             },
    { "Raspberry Pi",   "raspberry-pi",    "rpi-update",                   "/etc/os-release",            "VERSION_ID="          },
    { "RevyOS",         "revyos",          "revyos-keyring",               "/etc/revyos-release",        "BUILD_ID="            },
    { "PureOS",         "pureos",          "pureos-settings",              "/etc/os-release",            "VERSION_ID="          },
    { "Xubuntu",        "xubuntu",         "xubuntu-desktop",              "/etc/os-release",            "VERSION_ID="          },
    { "Kubuntu",        "kubuntu",         "kubuntu-desktop",              "/etc/os-release",            "VERSION_ID="          },
    { "Lubuntu",        "lubuntu",         "lubuntu-desktop",              "/etc/os-release",            "VERSION_ID="          },
    { "Edubuntu",       "edubuntu",        "edubuntu-desktop",             "/etc/os-release",            "VERSION_ID="          },
    { "Ubuntu Server",  "ubuntu",          "ubuntu-server",                "/etc/os-release",            "VERSION_ID="          },
    { "Ubuntu MATE",    "ubuntu-mate",     "ubuntu-mate-desktop",          "/etc/os-release",            "VERSION_ID="          },
    { "Ubuntu Budgie",  "ubuntu-budgie",   "ubuntu-budgie-desktop",        "/etc/os-release",            "VERSION_ID="          },
    { "Ubuntu Kylin",   "ubuntu-kylin",    "ubuntukylin-desktop",          "/etc/os-release",            "VERSION_ID="          },
    { "Ubuntu Studio",  "ubuntu-studio",   "ubuntustudio-desktop",         "/etc/os-release",            "VERSION_ID="          },
    { "Ubuntu Desktop", "ubuntu",          "ubuntu-desktop",               "/etc/os-release",            "VERSION_ID="          },
    { "Ubuntu GNOME",   "ubuntu-gnome",    "ubuntu-gnome-desktop",         "/etc/os-release",            "VERSION_ID="          },//dead
    { NULL }
};
void apt_flavors_scan(gchar **pretty_name, gchar **codename, gchar **id, gchar **orig_id, gchar **orig_name) {
    gboolean spawned;
    gchar *out, *err, *p, *next_nl,*st,*version;
    gchar **split, *contents=NULL, **line;
    gint exit_status;
    const AptFlavor *f = NULL;
    gchar *cmdline;
    int i = 0, found=0;

    while(!found && apt_flavors[i].name){
       if((apt_flavors[i].aptname[0]=='/') && g_file_get_contents(apt_flavors[i].aptname, &contents, NULL, NULL)) {
	   found=1;
           f = &apt_flavors[i];
	   g_free(contents);
       } else if(apt_flavors[i].aptname[0]!='/') {
	   cmdline = g_strconcat("sh -c 'LC_ALL=C apt-cache policy ", apt_flavors[i].aptname,"'",NULL);
           spawned = hardinfo_spawn_command_line_sync(cmdline, &out, &err, &exit_status, NULL);
	   g_free(cmdline);
           if (spawned) {
                p = out;
                while((next_nl = strchr(p, '\n'))) {
                    strend(p, '\n');
                    int mc = 0;
		    char pkg[32] = "";
		    if (*p != ' ' && *p != '\t')
		       mc = sscanf(p, "%31s", pkg);
		    if (mc == 1) {
		       strend(pkg, ':');
		       int i=0;
		       while(apt_flavors[i].name && !SEQ(apt_flavors[i].aptname, pkg)) i++;
		       if(apt_flavors[i].name) f = &apt_flavors[i]; else f=NULL;
		    } else if(g_strstr_len(p, -1, "Installed:") && !g_strstr_len(p, -1, "(none)") ) {
		        found=1;
		        break;
		    }
                    p = next_nl + 1;
	        }
	        g_free(out);
	        g_free(err);
	    }
        }
        if(!found) i++;
    }

    if(found){
        //find version
        version=NULL; split=NULL; contents=NULL;
        if (f && f->filename && (strlen(f->filename)>1) && g_file_get_contents(f->filename, &contents, NULL, NULL)){
	    split = g_strsplit(contents, "\n", 0);
	    if (split) for (line = split; *line; line++) {
		if (!f->versiontext ||
		    (!strlen(f->versiontext) || !strncmp(*line, f->versiontext, strlen(f->versiontext))) ) {
		  if(strlen(f->versiontext)==0)
		      version=g_strdup(contents);
		  else
		      version = g_strdup(*line + strlen(f->versiontext));
		  strend(version,' ');
		  strend(version,'_');
		  //clean up version and allow zero length
		  version=strreplace(version,"\"","");
		  version=strreplace(version,"\n","");
		  if(strlen(version)<1) {g_free(version); version=NULL;}
		}
	      }
	}
	if(version){
	    st=*pretty_name; *pretty_name=g_strdup_printf("%s %s - %s", f->name, version, st); g_free(st);
	} else {
	    st=*pretty_name; *pretty_name=g_strdup_printf("%s - %s", f->name, st); g_free(st);
	}
	//g_free(*codename);*codename=NULL;
	if(contents) g_free(contents);
	if(split) g_strfreev(split);
	g_free(*id);*id=g_strdup(f->id);
    }
    //use from os-release if not found and not native debian
    if(!found && *orig_id && !g_str_equal(*orig_id,"debian")){
	*id=*orig_id;
	if(*pretty_name && *orig_name){
            st=*pretty_name; *pretty_name=g_strdup_printf("%s - %s", *orig_name, st); g_free(st);
	}
        if(*orig_name) g_free(*orig_name);
    }
}


static gchar *get_libc_version(void)
{
    static const struct {
        const char *test_cmd;
        const char *match_str;
        const char *lib_name;
        gboolean try_ver_str;
        gboolean use_stderr;
    } libs[] = {
        { "ldd --version", "GLIBC", N_("GNU C Library"), TRUE, FALSE},
        { "ldd --version", "GNU libc", N_("GNU C Library"), TRUE, FALSE},
        { "ldd --version", "musl libc", N_("musl C Library"), TRUE, TRUE},
        { "ldconfig -V", "GLIBC", N_("GNU C Library"), TRUE, FALSE},
        { "ldconfig -V", "GNU libc", N_("GNU C Library"), TRUE, FALSE},
        { "ldconfig -v", "uClibc", N_("uClibc or uClibc-ng"), FALSE, FALSE},
        { "diet", "diet version", N_("diet libc"), TRUE, TRUE},
        { NULL }
    };
    int i=0;
    gboolean spawned;
    gchar *out, *err, *p, *ret=NULL,*ver_str;

    while (!ret && libs[i].test_cmd) {
        out=(err=NULL);
        spawned = hardinfo_spawn_command_line_sync(libs[i].test_cmd, &out, &err, NULL, NULL);
        if (spawned) {

	    if (libs[i].use_stderr) {
                if (strstr(err,"musl")) {p=strchr(err,'\n');*p=' ';} //combine musl arch and version
		p = strend(err, '\n');
	    } else {
	        p = strend(out, '\n');
	    }

	    if (p && strstr(p, libs[i].match_str)) {
	        if (libs[i].try_ver_str) {
		    /* skip the first word, likely "ldconfig" or name of utility */
		    if(strstr(p,"musl")) ver_str=p; else {ver_str=strchr(p, ' ');if(ver_str) ver_str++;}

		    if (ver_str) {
		        ret=g_strdup_printf("%s / %s", _(libs[i].lib_name), ver_str);
		    }
		}
		if(!ret) ret=g_strdup(_(libs[i].lib_name));
	    }
	}
	g_free(err);
	g_free(out);
	i++;
    }

    if(!ret) ret=g_strdup(_("Unknown"));
    return ret;
}

static gchar *detect_kde_version(void)
{
  gchar *cmd,*ret=NULL;
    const gchar *tmp = g_getenv("KDE_SESSION_VERSION");
    gchar *out=NULL;
    gboolean spawned;

    if (tmp && tmp[0] == '4') {
        cmd = "kwin --version";
    } else {
        cmd = "kcontrol --version";
    }

    spawned = hardinfo_spawn_command_line_sync(cmd, &out, NULL, NULL, NULL);
    if (!spawned) return NULL;

    tmp = strstr(out, "KDE: ");
    if(tmp) ret=g_strdup(tmp + strlen("KDE: "));

    g_free(out);

    return ret;
}


static gchar *detect_gnome_version(void)
{
    gchar *tmp;
    gchar *out,*ret=NULL;
    gboolean spawned;

    spawned = hardinfo_spawn_command_line_sync("gnome-shell --version", &out, NULL, NULL, NULL);
    if (spawned) {
        tmp = strstr(out, _("GNOME Shell "));

        if (tmp) {
            tmp += strlen(_("GNOME Shell "));
            ret=g_strdup_printf("GNOME %s", strend(tmp, '\n'));
	    g_free(out);
	    return ret;
        }
        g_free(out);
    }

    spawned = hardinfo_spawn_command_line_sync("gnome-about --gnome-version", &out, NULL, NULL, NULL);
    if (spawned) {
        tmp = strstr(out, _("Version: "));

        if (tmp) {
            tmp += strlen(_("Version: "));
            ret=g_strdup_printf("GNOME %s", strend(tmp, '\n'));
            g_free(out);
	    return ret;
        }
        g_free(out);
    }

    return NULL;
}


static gchar *detect_mate_version(void)
{
    gchar *tmp;
    gchar *out,*ret=NULL;
    gboolean spawned;

    spawned = hardinfo_spawn_command_line_sync("mate-about --version", &out, NULL, NULL, NULL);
    if (spawned) {
        tmp = strstr(out, _("MATE Desktop Environment "));

        if (tmp) {
            tmp += strlen(_("MATE Desktop Environment "));
	    ret=g_strdup_printf("MATE %s", strend(tmp, '\n'));
	    g_free(out);
            return ret;
        }
        g_free(out);
    }

    return NULL;
}

static gchar *detect_window_manager(void)
{
  const gchar *curdesktop;
  const gchar* windowman;
  GdkScreen *screen = gdk_screen_get_default();

#if GTK_CHECK_VERSION(3,0,0)
    if (GDK_IS_X11_SCREEN(screen)) {
#else
    if (screen && GDK_IS_SCREEN(screen)) {
#endif
      windowman = gdk_x11_screen_get_window_manager_name(screen);
    } else return g_strdup("Not X11");

    if (g_str_equal(windowman, "Xfwm4"))
        return g_strdup("XFCE 4");

    curdesktop = g_getenv("XDG_CURRENT_DESKTOP");
    if (curdesktop) {
        const gchar *desksession = g_getenv("DESKTOP_SESSION");

        if (desksession && !g_str_equal(curdesktop, desksession))
            return g_strdup(desksession);
    }

    return g_strdup_printf(_("Unknown (Window Manager: %s)"), windowman);
}

static gchar *
desktop_with_session_type(const gchar *desktop_env)
{
    const char *tmp;

    tmp = g_getenv("XDG_SESSION_TYPE");
    if (tmp) {
        if (!g_str_equal(tmp, "unspecified"))
            return g_strdup_printf(_(/*!/{desktop environment} on {session type}*/ "%s on %s"), desktop_env, tmp);
    }

    return g_strdup(desktop_env);
}

static gchar *detect_xdg_environment(const gchar *env_var)
{
    const gchar *tmp;

    tmp = g_getenv(env_var);
    if (!tmp)
        return NULL;

    if (g_str_equal(tmp, "GNOME") || g_str_equal(tmp, "gnome")) {
        gchar *maybe_gnome = detect_gnome_version();

        if (maybe_gnome)
            return maybe_gnome;
    }
    if (g_str_equal(tmp, "KDE") || g_str_equal(tmp, "kde")) {
        gchar *maybe_kde = detect_kde_version();

        if (maybe_kde)
            return maybe_kde;
    }
    if (g_str_equal(tmp, "MATE") || g_str_equal(tmp, "mate")) {
        gchar *maybe_mate = detect_mate_version();

        if (maybe_mate)
            return maybe_mate;
    }

    return g_strdup(tmp);
}

static gchar *detect_desktop_environment(void)
{
    const gchar *tmp;
    gchar *windowman;

    windowman = detect_xdg_environment("XDG_CURRENT_DESKTOP");
    if (windowman) return windowman;
    windowman = detect_xdg_environment("XDG_SESSION_DESKTOP");
    if (windowman) return windowman;

    tmp = g_getenv("KDE_FULL_SESSION");
    if (tmp) {
        gchar *maybe_kde = detect_kde_version();

        if (maybe_kde)
            return maybe_kde;
    }
    tmp = g_getenv("GNOME_DESKTOP_SESSION_ID");
    if (tmp) {
        gchar *maybe_gnome = detect_gnome_version();

        if (maybe_gnome)
            return maybe_gnome;
    }

    windowman = detect_window_manager();
    if (windowman)
        return windowman;

    if (!g_getenv("DISPLAY"))
        return g_strdup(_("Terminal"));

    return g_strdup(_("Unknown"));
}

gchar *computer_get_dmesg_status(void)
{
    gchar *out = NULL, *err = NULL;
    int ex = 1, result = 0;
    hardinfo_spawn_command_line_sync("dmesg", &out, &err, &ex, NULL);
    g_free(out);
    g_free(err);
    result += (getuid() == 0) ? 2 : 0;
    result += ex ? 1 : 0;
    switch(result) {
        case 0: /* readable, user */
            return g_strdup(_("User access allowed"));
        case 1: /* unreadable, user */
            return g_strdup(_("User access forbidden"));
        case 2: /* readable, root */
            return g_strdup(_("Access allowed (running as superuser)"));
        case 3: /* unreadable, root */
            return g_strdup(_("Access forbidden? (running as superuser)"));
    }
    return g_strdup(_("(Unknown)"));
}

gchar *computer_get_aslr(void)
{
    switch (h_sysfs_read_int("/proc/sys/kernel", "randomize_va_space")) {
    case 0:
        return g_strdup(_("Disabled"));
    case 1:
        return g_strdup(_("Partially enabled (mmap base+stack+VDSO base)"));
    case 2:
        return g_strdup(_("Fully enabled (mmap base+stack+VDSO base+heap)"));
    default:
        return g_strdup(_("Unknown"));
    }
}

gchar *computer_get_entropy_avail(void)
{
    gchar tab_entropy_fstr[][32] = {
      N_(/*!/bits of entropy for rng (0)*/              "(None or not available)"),
      N_(/*!/bits of entropy for rng (low/poor value)*/  "%d bits (low)"),
      N_(/*!/bits of entropy for rng (medium value)*/    "%d bits (medium)"),
      N_(/*!/bits of entropy for rng (high/good value)*/ "%d bits (healthy)")
    };
    gint bits = h_sysfs_read_int("/proc/sys/kernel/random", "entropy_avail");
    if (bits > 3000) return g_strdup_printf(_(tab_entropy_fstr[3]), bits);
    if (bits > 200)  return g_strdup_printf(_(tab_entropy_fstr[2]), bits);
    if (bits > 1)    return g_strdup_printf(_(tab_entropy_fstr[1]), bits);
    return g_strdup_printf(_(tab_entropy_fstr[0]), bits);
}

gchar *computer_get_language(void)
{
    gchar *tab_lang_env[] =
        { "LANGUAGE", "LANG", "LC_ALL", "LC_MESSAGES", NULL };
    gchar *lc = NULL, *env = NULL, *ret = NULL;
    gint i = 0;

    lc = setlocale(LC_ALL, NULL);

    while (tab_lang_env[i] != NULL) {
        env = g_strdup( g_getenv(tab_lang_env[i]) );
        if (env != NULL)  break;
        i++;
    }

    if (env != NULL)
        if (lc != NULL)
            ret = g_strdup_printf("%s (%s)", lc, env);
        else
            ret = g_strdup_printf("%s", env);
    else
        if (lc != NULL)
            ret = g_strdup_printf("%s", lc);

    if (ret == NULL)
        ret = g_strdup( _("(Unknown)") );

    return ret;
}

static Distro parse_os_release(void)
{
    gchar *pretty_name = NULL;
    gchar *id = NULL;
    gchar *version = NULL;
    gchar *codename = NULL;
    gchar **split, *contents, *content2, **line;
    gchar *orig_id=NULL, *orig_name=NULL;

    //some overrides the /etc/os-release, so we use usr/lib first and fixes distro via Apt, OLD DISTRO fallback to /etc/os.
    if (!g_file_get_contents("/usr/lib/os-release", &contents, NULL, NULL))
        if (!g_file_get_contents("/etc/os-release", &contents, NULL, NULL))
            return (Distro) {};

    //force /etc/os-release for some distros
    if (!g_file_get_contents("/etc/os-release", &content2, NULL, NULL)) return (Distro) {};
    if(strstr(content2,"CachyOS")) {
        g_free(contents);
	contents=content2;
    }

    split = g_strsplit(contents, "\n", 0);
    g_free(contents);
    if (!split)
        return (Distro) {};

    for (line = split; *line; line++) {
        if (!strncmp(*line, "ID=", sizeof("ID=") - 1) && (id == NULL)) {
            id = g_strdup(*line + strlen("ID="));
        } else if (!strncmp(*line, "VERSION_ID=", sizeof("VERSION_ID=") - 1) && (version == NULL)) {
            version = g_strdup(*line + strlen("VERSION_ID="));
        } else if (!strncmp(*line, "CODENAME=", sizeof("CODENAME=") - 1) && (codename == NULL)) {
            codename = g_strdup(*line + strlen("CODENAME="));
        } else if (!strncmp(*line, "VERSION_CODENAME=", sizeof("VERSION_CODENAME=") - 1) && (codename == NULL)) {
            codename = g_strdup(*line + strlen("VERSION_CODENAME="));
        } else if (!strncmp(*line, "PRETTY_NAME=", sizeof("PRETTY_NAME=") - 1) && (pretty_name == NULL)) {
            pretty_name = g_strdup(*line + strlen("PRETTY_NAME="));
        }
    }

    g_strfreev(split);

    //remove ",/n,allow empty
    if(pretty_name){
        pretty_name=strreplace(pretty_name,"\"","");
	//
        pretty_name=strreplace(pretty_name,"\n","");
        if(strlen(pretty_name)<1) {g_free(pretty_name);pretty_name=NULL;}
    }
    if(codename){
        codename=strreplace(codename,"\"","");
	//
        codename=strreplace(codename,"\n","");
        if(strlen(codename)<1) {g_free(codename);codename=NULL;}
    }
    if(id){
        id=strreplace(id,"\"","");
	//
        id=strreplace(id,"\n","");
	//
        id=strreplace(id," ","");
        if(strlen(id)<1) {g_free(id);id=NULL;}
    }
    if(version){
        version=strreplace(version,"\"","");
	//
        version=strreplace(version,"\n","");
        if(strlen(version)<1) {g_free(version);version=NULL;}
    }

    //remove codename from pretty name
    if(pretty_name && codename){
        gchar *t;
	//upper first letter
	t=g_strdup_printf(" (%s)",codename);
	t[2]=toupper(t[2]);
        pretty_name=strreplace(pretty_name,t,"");
	g_free(t);
	//normal
	t=g_strdup_printf(" (%s)",codename);
        pretty_name=strreplace(pretty_name,t,"");
	g_free(t);
	//without brackets upper first letter
	t=g_strdup_printf(" %s",codename);
	t[1]=toupper(t[1]);
        pretty_name=strreplace(pretty_name,t,"");
	g_free(t);
	//without brackets normal
	t=g_strdup_printf(" %s",codename);
        pretty_name=strreplace(pretty_name,t,"");
	g_free(t);
	g_strstrip(pretty_name);
    }

    //Based on Alpine Linux add to distro string
    if(pretty_name && !g_str_equal(id, "alpine")  && g_file_get_contents("/etc/alpine-release", &contents , NULL, NULL) ) {
        gchar *t,*p=contents;
        while(*p && ((*p>'9') || (*p<'0'))) p++;
	strend(p,' ');
        t=pretty_name; pretty_name=g_strdup_printf("%s - Alpine %s", t,p); g_free(t);
        g_free(contents);
    } else
    //Based on Fedora Linux add to distro string
    if(pretty_name && !g_str_equal(id, "fedora")  && g_file_get_contents("/etc/fedora-release", &contents , NULL, NULL) ) {
        gchar *t,*p=contents;
        while(*p && ((*p>'9') || (*p<'0'))) p++;
	strend(p,' ');
	if(g_str_equal(id,"altlinux")) {/*ALT Linux is not directly based on redhat/fedora anymore - was based on redhat*/}
        else {
	  t=pretty_name; pretty_name=g_strdup_printf("%s - Fedora %s", t,p); g_free(t);
	}
        g_free(contents);
    } else
    //Based on RedHat Linux add to distro string
    if(pretty_name && !g_str_equal(id, "rhel") && !g_str_equal(id, "fedora") && g_file_get_contents("/etc/redhat-release", &contents , NULL, NULL) ) {
        gchar *t,*p=contents;
        while(*p && ((*p>'9') || (*p<'0'))) p++;
	strend(p,' ');
	//FIXME: Add table RHEL->Fedora
	//RHEL4=>FC3
	//RHEL5=>FC6
	//RHEL6=>FC12+
	//RHEL7=>FC19+
	//RHEL8=>FC28
	//RHEL9=>FC34
	//RHEL10=>FC40
	if(g_str_equal(id,"altlinux")) {/*ALT Linux is not directly based on redhat/fedora anymore - was based on redhat*/}
        else if(g_str_equal(id,"openmandriva")) {/*Mandriva is not based on redhat/fedora anymore - was based on RH5.1*/}
	else if(atoi(p)>=19){//hmm, distro should have had fedora-release
            t=pretty_name; pretty_name=g_strdup_printf("%s - Fedora %s", t,p); g_free(t);
	} else {
            t=pretty_name; pretty_name=g_strdup_printf("%s - RHEL %s", t,p); g_free(t);
	}
        g_free(contents);
    }
    //Based on Arch Linux add to distro string
    if(pretty_name && !g_str_equal(id, "arch") && g_file_get_contents("/etc/arch-release", &contents , NULL, NULL) ) {
       gchar *t;
       t=pretty_name; pretty_name=g_strdup_printf("%s - Arch Linux", t); g_free(t);
       g_free(contents);
    }
    //Based on Slackware add to distro string
    if(pretty_name && !g_str_equal(id, "slackware") && g_file_get_contents("/etc/slackware-version", &contents , NULL, NULL) ) {
       gchar *t;
       t=pretty_name; pretty_name=g_strdup_printf("%s - Slackware", t); g_free(t);
       g_free(contents);
    }
    //Based on debian add to distro string
    if(pretty_name && id && (g_file_get_contents("/etc/debian_version", &contents , NULL, NULL) || g_str_equal(id,"debian")) ) {
       orig_id=id;
       id=g_strdup("debian");
       orig_name=pretty_name;
       //debian version check
       if (!contents || isdigit(contents[0]) || contents[0] != 'D') {
	   if(contents)
              pretty_name=g_strdup_printf("Debian GNU/Linux %s", contents);
           else
              pretty_name=g_strdup_printf("Debian GNU/Linux");
       } else {
           pretty_name=g_strdup(contents);
       }
       g_free(contents);
    }
    //use APT to find distro
    if (pretty_name && (g_str_equal(id, "debian") ||g_str_equal(id, "ubuntu")) ) {
      apt_flavors_scan(&pretty_name,&codename,&id,&orig_id,&orig_name);
    }

    if (pretty_name){
        g_strstrip(pretty_name);
        return (Distro) { .distro = pretty_name, .codename = codename, .id = id };
    }

    g_free(pretty_name);
    g_free(id);
    g_free(version);
    g_free(codename);
    return (Distro) {};
}


static Distro detect_distro(void)
{
    static const struct {
        const gchar *file;
        const gchar *id;
        const gchar *override;
        const gchar *versiontext;
    } distro_db[] = {
#define DB_PREFIX "/etc/"
//This table is fallback to check files instead of os-release file and apt (OLD DISTROS)
        { DB_PREFIX "arch-release",        "arch",    "Arch Linux" ,      NULL },
        { DB_PREFIX "fatdog-version",      "fatdog",  NULL,               "" },
	{ DB_PREFIX "debian_version",      "debian",  "Debian GNU/Linux", "" },
        { DB_PREFIX "slackware-version",   "slk" },
        { DB_PREFIX "mandrake-release",    "mdk" },
        { DB_PREFIX "mandriva-release",    "mdv" },
        { DB_PREFIX "fedora-release",      "fedora" },
        { DB_PREFIX "coas",                "coas" },
        { DB_PREFIX "environment.corel",   "corel" },
        { DB_PREFIX "gentoo-release",      "gnt" },
        { DB_PREFIX "conectiva-release",   "cnc" },
        { DB_PREFIX "versão-conectiva",    "cnc" },
        { DB_PREFIX "turbolinux-release",  "tl" },
        { DB_PREFIX "yellowdog-release",   "yd" },
        { DB_PREFIX "sabayon-release",     "sbn" },
        { DB_PREFIX "enlisy-release",      "enlsy" },
        { DB_PREFIX "SuSE-release",        "suse" },
        { DB_PREFIX "sun-release",         "sun" },
        { DB_PREFIX "zenwalk-version",     "zen" },
        { DB_PREFIX "DISTRO_SPECS",        "puppy",    "Puppy Linux",     "DISTRO_VERSION=" },
        { DB_PREFIX "distro-release",      "fl" },
        { DB_PREFIX "vine-release",        "vine" },
        { DB_PREFIX "PartedMagic-version", "pmag" },
        { DB_PREFIX "NIXOS",               "nixos",    "NixOS Linux",     NULL },
        { DB_PREFIX "redhat-release",      "redhat" },
#undef DB_PREFIX
        { NULL, NULL }
    };
    Distro distro;
    gchar *contents,*t;
    int i;
    gchar *version=NULL, **split, **line;

    distro = parse_os_release();
    if (distro.distro) return distro;

    //if not found via os-release then find distro via files
    for (i = 0; distro_db[i].file; i++) {
        if (!g_file_get_contents(distro_db[i].file, &contents, NULL, NULL))
            continue;

	distro.distro=g_strdup(contents);
	distro.id = g_strdup(distro_db[i].id);

	//override distro name
        if (distro_db[i].override) {
	    g_free(distro.distro);
	    distro.distro = g_strdup(distro_db[i].override);
        }

	//Add version
        if (distro.distro && distro_db[i].versiontext) {
	    //find version
            split = g_strsplit(contents, "\n", 0);
            if (split) for (line = split; *line; line++) {
                if (!distro_db[i].versiontext ||
		  (!strlen(distro_db[i].versiontext) || !strncmp(*line, distro_db[i].versiontext, strlen(distro_db[i].versiontext)))) {
		    if(strlen(distro_db[i].versiontext)==0)
		        version=g_strdup(contents);
		    else
		        version = g_strdup(*line + strlen(distro_db[i].versiontext));
	            strend(version,' ');
	            strend(version,'_');
	            //clean up version and allow zero length
	            version=strreplace(version,"\"","");
	            version=strreplace(version,"\n","");
	            if(strlen(version)<1) {g_free(version);version=NULL;}
	        }
	    }
            //add version
	    if(version){
	       t=distro.distro;
               distro.distro = g_strdup_printf("%s %s", t, contents),
	       g_free(t);
	    }
	}
        g_free(contents);
        return distro;
    }

    return (Distro) { .distro = g_strdup(_("Unknown")) };
}

OperatingSystem *computer_get_os(void)
{
    struct utsname utsbuf;
    OperatingSystem *os;
    gchar *p=NULL;

    os = g_new0(OperatingSystem, 1);

    Distro distro = detect_distro();
    os->distro = g_strstrip(distro.distro);
    os->distroid = distro.id;
    os->distrocode = distro.codename;

    /* Kernel and hostname info */
    uname(&utsbuf);
    os->kernel_version = g_strdup(utsbuf.version);
    os->kernel = g_strdup_printf("%s %s (%s)", utsbuf.sysname,
				 utsbuf.release, utsbuf.machine);
    os->kcmdline = h_sysfs_read_string("/proc", "cmdline");
    os->hostname = g_strdup(utsbuf.nodename);
    os->language = computer_get_language();
    os->homedir = g_strdup(g_get_home_dir());
    os->username = g_strdup_printf("%s (%s)",
				   g_get_user_name(), g_get_real_name());
    os->libc = get_libc_version();
    scan_languages(os);

    os->desktop = detect_desktop_environment();
    if (os->desktop){
        p=os->desktop;
        os->desktop=desktop_with_session_type(p);
	g_free(p);
    }

    os->entropy_avail = computer_get_entropy_avail();

    return os;
}

const gchar *
computer_get_selinux(void)
{
    int r;
    gboolean spawned = hardinfo_spawn_command_line_sync("selinuxenabled", NULL, NULL, &r, NULL);

    if (!spawned)
        return _("Not installed");

    if (r == 0)
        return _("Enabled");

    return _("Disabled");
}

gchar *
computer_get_lsm(void)
{
    gchar *contents;

    if (!g_file_get_contents("/sys/kernel/security/lsm", &contents, NULL, NULL))
        return g_strdup(_("Unknown"));

    return contents;
}
