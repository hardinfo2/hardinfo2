/*
 * HardInfo2
 * Copyright(C) 2003-2007 L. A. F. Pereira.
 * Copyright(C) 2024-2026 HardInfo2 Project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 or later.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <menu.h>
#include <config.h>

#include <iconcache.h>

#include <callbacks.h>
#include <hardinfo.h>


static void _register_action(Shell *shell, const gchar *name, GtkWidget *widget) {
    g_hash_table_insert(shell->action_widget_map, g_strdup(name), widget);
}

void menu_init(Shell * shell)
{
    GtkWidget *menu_box = shell->vbox;
    gint size = 16;

    /* Scale icon sizes */
    if(params.scale >= 1.5) size = 20;
    if(params.scale >= 2)   size = 24;

    /* Create the toolbar that acts as menubar */
    shell->toolbar_widget = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(shell->toolbar_widget), GTK_TOOLBAR_BOTH_HORIZ);

    /* Create action_widget_map early so signal handlers can use it */
    shell->action_widget_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    /* Create submenus */
    GtkWidget *info_submenu = gtk_menu_new();
    GtkWidget *view_submenu = gtk_menu_new();
#if GTK_CHECK_VERSION(3, 20, 0)
    GtkWidget *theme_submenu = gtk_menu_new();
#endif
    GtkWidget *help_submenu = gtk_menu_new();

    /* ===========================================
     * INFORMATION MENU ITEMS
     * ========================================= */

    /* Report menu item (also in toolbar) */
    shell->menu_report = gtk_image_menu_item_new_with_mnemonic(_("Generate _Report"));
#if GTK_CHECK_VERSION(2, 16, 0)
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(shell->menu_report), TRUE);
#endif
    GtkWidget *img = icon_cache_get_image_at_size("report.svg", size, size);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(shell->menu_report), img);
    g_signal_connect(G_OBJECT(shell->menu_report), "activate", G_CALLBACK(cb_generate_report), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(info_submenu), shell->menu_report);

    /* Separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(info_submenu), gtk_separator_menu_item_new());

    /* SyncManager menu item (also in toolbar) */
    shell->menu_sync = gtk_image_menu_item_new_with_mnemonic(_("Synchronize"));
#if GTK_CHECK_VERSION(2, 16, 0)
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(shell->menu_sync), TRUE);
#endif
    img = icon_cache_get_image_at_size("sync.svg", size, size);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(shell->menu_sync), img);
    g_signal_connect(G_OBJECT(shell->menu_sync), "activate", G_CALLBACK(cb_sync_manager), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(info_submenu), shell->menu_sync);

    /* SyncOnStartup toggle (menu only) */
    GtkWidget *sync_startup = gtk_check_menu_item_new_with_mnemonic(_("Synchronize on startup"));
    g_signal_connect(G_OBJECT(sync_startup), "toggled", G_CALLBACK(cb_sync_on_startup), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(info_submenu), sync_startup);

    /* Separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(info_submenu), gtk_separator_menu_item_new());

    /* Quit menu item (also in toolbar) */
    shell->menu_quit = gtk_image_menu_item_new_with_mnemonic(_("_Quit"));
#if GTK_CHECK_VERSION(2, 16, 0)
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(shell->menu_quit), TRUE);
#endif
    img = icon_cache_get_image_at_size("close.svg", size, size);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(shell->menu_quit), img);
    g_signal_connect(G_OBJECT(shell->menu_quit), "activate", G_CALLBACK(cb_quit), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(info_submenu), shell->menu_quit);

    /* ===========================================
     * VIEW MENU ITEMS
     * ========================================= */

    /* SidePane toggle */
    shell->menu_side_pane = gtk_check_menu_item_new_with_mnemonic(_("_Side Panel"));
    g_signal_connect(G_OBJECT(shell->menu_side_pane), "toggled", G_CALLBACK(cb_side_pane), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_submenu), shell->menu_side_pane);
    _register_action(shell, "SidePaneAction", shell->menu_side_pane);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(shell->menu_side_pane), TRUE);

    /* Toolbar toggle */
    shell->menu_toolbar = gtk_check_menu_item_new_with_mnemonic(_("_Toolbar"));
    g_signal_connect(G_OBJECT(shell->menu_toolbar), "toggled", G_CALLBACK(cb_toolbar), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_submenu), shell->menu_toolbar);
    _register_action(shell, "ToolbarAction", shell->menu_toolbar);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(shell->menu_toolbar), TRUE);

#if GTK_CHECK_VERSION(3, 20, 0)
    /* Theme submenu */
    GtkWidget *theme_menu_item = gtk_menu_item_new_with_mnemonic(_("_Theme"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(theme_menu_item), theme_submenu);
    //g_object_set_data(G_OBJECT(shell->toolbar_widget), "theme_menu", theme_menu_item);

    /* Create radio group for themes */
    GtkWidget *theme_disable = gtk_radio_menu_item_new_with_mnemonic(NULL, _("Disable Theme"));
    shell->theme_radio_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(theme_disable));
    g_signal_connect(G_OBJECT(theme_disable), "toggled", G_CALLBACK(cb_disable_theme), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(theme_submenu), theme_disable);
    _register_action(shell, "DisableThemeAction", theme_disable);

    GtkWidget *theme1 = gtk_radio_menu_item_new_with_mnemonic(shell->theme_radio_group, _("Theme Motherboard"));
    g_signal_connect(G_OBJECT(theme1), "toggled", G_CALLBACK(cb_theme1), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(theme_submenu), theme1);
    _register_action(shell, "Theme1Action", theme1);

    GtkWidget *theme2 = gtk_radio_menu_item_new_with_mnemonic(shell->theme_radio_group, _("Theme Graffiti"));
    g_signal_connect(G_OBJECT(theme2), "toggled", G_CALLBACK(cb_theme2), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(theme_submenu), theme2);
    _register_action(shell, "Theme2Action", theme2);

    GtkWidget *theme3 = gtk_radio_menu_item_new_with_mnemonic(shell->theme_radio_group, _("Theme Anime PC"));
    g_signal_connect(G_OBJECT(theme3), "toggled", G_CALLBACK(cb_theme3), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(theme_submenu), theme3);
    _register_action(shell, "Theme3Action", theme3);

    GtkWidget *theme4 = gtk_radio_menu_item_new_with_mnemonic(shell->theme_radio_group, _("Theme Tux Star"));
    g_signal_connect(G_OBJECT(theme4), "toggled", G_CALLBACK(cb_theme4), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(theme_submenu), theme4);
    _register_action(shell, "Theme4Action", theme4);

    GtkWidget *theme5 = gtk_radio_menu_item_new_with_mnemonic(shell->theme_radio_group, _("Theme PixArt"));
    g_signal_connect(G_OBJECT(theme5), "toggled", G_CALLBACK(cb_theme5), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(theme_submenu), theme5);
    _register_action(shell, "Theme5Action", theme5);

    GtkWidget *theme6 = gtk_radio_menu_item_new_with_mnemonic(shell->theme_radio_group, _("Theme Silicon"));
    g_signal_connect(G_OBJECT(theme6), "toggled", G_CALLBACK(cb_theme6), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(theme_submenu), theme6);
    _register_action(shell, "Theme6Action", theme6);
#endif /* GTK_CHECK_VERSION(3, 20, 0) */

    /* Separator */
    gtk_menu_shell_append(GTK_MENU_SHELL(view_submenu), gtk_separator_menu_item_new());

    /* Refresh menu item (also in toolbar as button) */
    GtkWidget *menu_refresh = gtk_image_menu_item_new_with_mnemonic(_("_Refresh"));
#if GTK_CHECK_VERSION(2, 16, 0)
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(menu_refresh), TRUE);
#endif
    img = icon_cache_get_image_at_size("refresh.svg", size, size);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_refresh), img);
    g_signal_connect(G_OBJECT(menu_refresh), "activate", G_CALLBACK(cb_refresh), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_submenu), menu_refresh);

    /* ===========================================
     * HELP MENU ITEMS
     * ========================================= */

    /* Home Web Page */
    GtkWidget *web_item = gtk_image_menu_item_new_with_mnemonic(_("HardInfo2 _Web Site"));
#if GTK_CHECK_VERSION(2, 16, 0)
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(web_item), TRUE);
#endif
    img = icon_cache_get_image_at_size("home.svg", size, size);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(web_item), img);
    g_signal_connect(G_OBJECT(web_item), "activate", G_CALLBACK(cb_open_web_page), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(help_submenu), web_item);

    /* Help Page */
    GtkWidget *help_page = gtk_image_menu_item_new_with_mnemonic(_("_Help - User Guide"));
#if GTK_CHECK_VERSION(2, 16, 0)
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(help_page), TRUE);
#endif
    img = icon_cache_get_image_at_size("help.svg", size, size);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(help_page), img);
    g_signal_connect(G_OBJECT(help_page), "activate", G_CALLBACK(cb_open_help_page), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(help_submenu), help_page);

    /* Update HardInfo2 */
    GtkWidget *update_page = gtk_image_menu_item_new_with_mnemonic(_("_Update HardInfo2"));
#if GTK_CHECK_VERSION(2, 16, 0)
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(update_page), TRUE);
#endif
    img = icon_cache_get_image_at_size("updates.svg", size, size);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(update_page), img);
    g_signal_connect(G_OBJECT(update_page), "activate", G_CALLBACK(cb_open_updates_page), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(help_submenu), update_page);

    /* Report Bug */
    GtkWidget *bug_report = gtk_image_menu_item_new_with_mnemonic(_("_Report bug"));
#if GTK_CHECK_VERSION(2, 16, 0)
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(bug_report), TRUE);
#endif
    img = icon_cache_get_image_at_size("report-bug.svg", size, size);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(bug_report), img);
    g_signal_connect(G_OBJECT(bug_report), "activate", G_CALLBACK(cb_report_bug), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(help_submenu), bug_report);

    /* About */
    GtkWidget *about_item = gtk_image_menu_item_new_with_mnemonic(_("_About HardInfo2"));
#if GTK_CHECK_VERSION(2, 16, 0)
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(about_item), TRUE);
#endif
    img = icon_cache_get_image_at_size("hardinfo2.svg", size, size);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(about_item), img);
    g_signal_connect(G_OBJECT(about_item), "activate", G_CALLBACK(cb_about), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(help_submenu), about_item);

    /* ===========================================
     * CREATE MENU BAR with Dropdown Menus
     * ========================================= */

    shell->menubar = gtk_menu_bar_new();

    /* Information Menu (dropdown) */
    GtkWidget *info_menu_btn = gtk_menu_item_new_with_mnemonic(_("_Information"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(info_menu_btn), info_submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(shell->menubar), info_menu_btn);

    /* View Menu (dropdown) */
    GtkWidget *view_menu_btn = gtk_menu_item_new_with_mnemonic(_("_View"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_menu_btn), view_submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(shell->menubar), view_menu_btn);

#if GTK_CHECK_VERSION(3, 20, 0)
    /* Theme submenu inside View Menu */
    gtk_menu_shell_insert(GTK_MENU_SHELL(view_submenu), theme_menu_item, 2);
#endif

    /* Help Menu (dropdown) */
    GtkWidget *help_menu_btn = gtk_menu_item_new_with_mnemonic(_("_Help"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_menu_btn), help_submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(shell->menubar), help_menu_btn);

    /* Pack menubar into vbox first */
    gtk_box_pack_start(GTK_BOX(menu_box), shell->menubar, FALSE, FALSE, 0);
    gtk_widget_show_all(shell->menubar);

    /* ===========================================
     * CREATE TOOLBAR buttons (using GtkBox for GTK3 compatibility)
     * ========================================= */

    /* Create toolbar as a horizontal box with buttons/separators */
    GtkWidget *toolbar = shell->toolbar_widget;
    gtk_widget_show(toolbar);

    size*=1.5;

    /* Add refresh button to toolbar */
    GtkWidget *btn_refresh = gtk_button_new_with_label(_("Refresh"));
    img = icon_cache_get_image_at_size("refresh.svg", size, size);
#if GTK_CHECK_VERSION(3, 6, 0)
    gtk_button_set_always_show_image (GTK_BUTTON (btn_refresh), TRUE);
#endif
    gtk_button_set_image (GTK_BUTTON (btn_refresh), img);
    GtkToolItem *tbtn_refresh = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(tbtn_refresh), btn_refresh);
    g_signal_connect(G_OBJECT(btn_refresh), "clicked", G_CALLBACK(cb_refresh), NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_refresh), "flat");
#endif
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(tbtn_refresh), -1);
    gtk_widget_show_all(GTK_WIDGET(tbtn_refresh));

    /* Separator */
    GtkToolItem *sep1 = GTK_TOOL_ITEM(gtk_separator_tool_item_new());
    gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(sep1), FALSE);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(sep1), -1);
    gtk_widget_show_all(GTK_WIDGET(sep1));

    /* Report button */
    GtkWidget *btn_report = gtk_button_new_with_label(_("Report"));
    img = icon_cache_get_image_at_size("report.svg", size, size);
#if GTK_CHECK_VERSION(3, 6, 0)
    gtk_button_set_always_show_image (GTK_BUTTON (btn_report), TRUE);
#endif
    gtk_button_set_image (GTK_BUTTON (btn_report), img);
    GtkToolItem *tbtn_report = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(tbtn_report), btn_report);
    g_signal_connect(G_OBJECT(btn_report), "clicked", G_CALLBACK(cb_generate_report), NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_report), "flat");
#endif
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(tbtn_report), -1);
    gtk_widget_show_all(GTK_WIDGET(tbtn_report));

    /* Separator */
    GtkToolItem *sep2 = GTK_TOOL_ITEM(gtk_separator_tool_item_new());
    gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(sep2), FALSE);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(sep2), -1);
    gtk_widget_show_all(GTK_WIDGET(sep2));

    /* Sync button */
    GtkWidget *btn_sync = gtk_button_new_with_label(_("Synchronize"));
    img = icon_cache_get_image_at_size("sync.svg", size, size);
#if GTK_CHECK_VERSION(3, 6, 0)
    gtk_button_set_always_show_image (GTK_BUTTON (btn_sync), TRUE);
#endif
    gtk_button_set_image (GTK_BUTTON (btn_sync), img);
    GtkToolItem *tbtn_sync = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(tbtn_sync), btn_sync);
    g_signal_connect(G_OBJECT(btn_sync), "clicked", G_CALLBACK(cb_sync_manager), NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_sync), "flat");
#endif
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(tbtn_sync), -1);
    gtk_widget_show_all(GTK_WIDGET(tbtn_sync));

    /* Separator */
    GtkToolItem *sep3 = GTK_TOOL_ITEM(gtk_separator_tool_item_new());
    gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(sep3), FALSE);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(sep3), -1);
    gtk_widget_show_all(GTK_WIDGET(sep3));

    /* Update button (info only, disabled) */
    GtkWidget *btn_update = gtk_toggle_button_new_with_label(_("Update"));
    img = icon_cache_get_image_at_size("updates.svg", size, size);
#if GTK_CHECK_VERSION(3, 6, 0)
    gtk_button_set_always_show_image (GTK_BUTTON (btn_update), TRUE);
#endif
    gtk_button_set_image (GTK_BUTTON (btn_update), img);
    GtkToolItem *tbtn_update = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(tbtn_update), btn_update);
    gtk_widget_set_sensitive(btn_update, FALSE);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(tbtn_update), -1);
    /* Register so shell_action_set_property can show/hide it at startup */
    _register_action(shell, "UpdateAction", GTK_WIDGET(tbtn_update));
    gtk_widget_show_all(GTK_WIDGET(tbtn_update));

    /* Separator */
    GtkToolItem *sep4 = GTK_TOOL_ITEM(gtk_separator_tool_item_new());
    gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(sep4), FALSE);
    gtk_tool_item_set_expand(sep4, TRUE);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(sep4), -1);
    gtk_widget_show_all(GTK_WIDGET(sep4));

    shell->search_entry = gtk_entry_new();
    GtkWidget *search_entry = shell->search_entry;
    gtk_widget_set_size_request(search_entry, 150, -1);
    GtkToolItem *entry_item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(entry_item), search_entry);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), entry_item, -1);
    gtk_widget_show_all(GTK_WIDGET(entry_item));

    shell->search_button = gtk_button_new_with_label(_("Search"));
    GtkWidget *search_button = shell->search_button;
    GtkToolItem *button_item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(button_item), search_button);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), button_item, -1);
    gtk_widget_show_all(GTK_WIDGET(button_item));

    /* Push toolbar to the end with expansion */
#if GTK_CHECK_VERSION(3,0,0)
    GtkWidget *hbox_wrap = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
    GtkWidget *hbox_wrap = gtk_hbox_new(TRUE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(hbox_wrap), toolbar, TRUE, TRUE, 0);

    //GtkWidget *align = gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f);
    //gtk_widget_set_size_request(align, 1500, -1);
    //gtk_box_pack_end(GTK_BOX(hbox_wrap), align, FALSE, TRUE, 0);

    /* Pack toolbar into vbox after menubar */
    gtk_box_pack_start(GTK_BOX(menu_box), hbox_wrap, FALSE, FALSE, 0);
    gtk_widget_show_all(hbox_wrap);

    /* ===========================================
     * Register all action widgets for lookup API
     * ========================================= */

    _register_action(shell, "ReportAction", shell->menu_report);
    _register_action(shell, "SyncManagerAction", shell->menu_sync);
    _register_action(shell, "QuitAction", shell->menu_quit);
    _register_action(shell, "RefreshAction", shell->menu_refresh);
    _register_action(shell, "SidePaneAction", shell->menu_side_pane);
    _register_action(shell, "ToolbarAction", shell->menu_toolbar);

#if GTK_CHECK_VERSION(3, 20, 0)
    /* Theme actions */
    {
        GSList *group = shell->theme_radio_group;
        gint idx = 0;
        for(GSList *iter = group; iter != NULL; iter = g_slist_next(iter), idx++) {
            const gchar *name;
            switch(idx) {
                case 0: name = "DisableThemeAction"; break;
                case 1: name = "Theme1Action"; break;
                case 2: name = "Theme2Action"; break;
                case 3: name = "Theme3Action"; break;
                case 4: name = "Theme4Action"; break;
                case 5: name = "Theme5Action"; break;
                case 6: name = "Theme6Action"; break;
                default: continue;
            }
            _register_action(shell, name, GTK_WIDGET(iter->data));
        }
    }
#endif

    GtkIconSize icon_size = GTK_ICON_SIZE_MENU;
    if(params.scale >= 1.5) icon_size = GTK_ICON_SIZE_LARGE_TOOLBAR;
    if(params.scale >= 2)   icon_size = (GtkIconSize)6;

    gtk_toolbar_set_icon_size(GTK_TOOLBAR(shell->toolbar_widget), icon_size);
}
