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

#include <string.h>

#include "hardinfo.h"
#include "computer.h"

DisplayInfo *computer_get_display(void) {
    DisplayInfo *di = g_new0(DisplayInfo, 1);
    di->wl = get_walyand_info();
    di->xi = xinfo_get_info();

    di->width = di->height = 0;
    if (di->xi->xrr->screen_count > 0) {
        di->width = di->xi->xrr->screens[0].px_width;
        di->height = di->xi->xrr->screens[0].px_height;
    }
    di->vendor = di->xi->vendor;
    di->session_type = di->wl->xdg_session_type;

    if (strcmp(di->session_type, "x11") == 0 ) {
        if (di->xi->nox) {
            di->display_server = g_strdup(_("(Unknown)"));
            /* assumed x11 previously, because it wasn't set */
            free(di->wl->xdg_session_type);
            di->session_type = (di->wl->xdg_session_type = NULL);
        } else if (di->xi->vendor && di->xi->version)
            di->display_server = g_strdup_printf("%s %s", di->xi->vendor, di->xi->version );
        else if (di->xi->vendor && di->xi->release_number)
            di->display_server = g_strdup_printf("[X11] %s %s", di->xi->vendor, di->xi->release_number );
        else
            di->display_server = g_strdup("X11");
    } else
    if (strcmp(di->session_type, "wayland") == 0 ) {
        di->display_server = g_strdup("Wayland");
    } else
    if (strcmp(di->session_type, "mir") == 0 ) {
        di->display_server = g_strdup("Mir");
    } else {
        di->display_server = g_strdup(_("(Unknown)"));
    }

    return di;
}

void computer_free_display(DisplayInfo *di) {
    /* fyi: DisplayInfo is in computer.h */
    if (di) {
        free(di->display_server);
        xinfo_free(di->xi);
        wl_free(di->wl);
        free(di);
    }
}
