/***************************************************************************
 * ROM Properties Page shell extension. (GTK+ common)                      *
 * OptionsTab.cpp: Options tab for rp-config.                              *
 *                                                                         *
 * Copyright (c) 2017-2022 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "OptionsTab.hpp"
#include "RpConfigTab.hpp"

#include "RpGtk.hpp"
#include "gtk-compat.h"

#include "LanguageComboBox.hpp"

// librpbase
using namespace LibRpBase;

// PAL language codes for GameTDB.
// NOTE: 'au' is technically not a language code, but
// GameTDB handles it as a separate language.
// TODO: Combine with the KDE version.
// NOTE: GTK LanguageComboBox uses a NULL-terminated pal_lc[] array.
static const uint32_t pal_lc[] = {'au', 'de', 'en', 'es', 'fr', 'it', 'nl', 'pt', 'ru', 0};
static const int pal_lc_idx_def = 2;

#if GTK_CHECK_VERSION(3,0,0)
typedef GtkBoxClass superclass;
typedef GtkBox super;
#define GTK_TYPE_SUPER GTK_TYPE_BOX
#else /* !GTK_CHECK_VERSION(3,0,0) */
typedef GtkVBoxClass superclass;
typedef GtkVBox super;
#define GTK_TYPE_SUPER GTK_TYPE_VBOX
#endif /* GTK_CHECK_VERSION(3,0,0) */

// OptionsTab class
struct _RpOptionsTabClass {
	superclass __parent__;
};

// OptionsTab instance
struct _RpOptionsTab {
	super __parent__;
	bool inhibit;	// If true, inhibit signals.
	bool changed;	// If true, an option was changed.

	// Image bandwidth options
	GtkWidget *fraExtImgDownloads;
	GtkWidget *chkExtImgDownloadEnabled;
	GtkWidget *lblUnmeteredConnection;
	GtkWidget *cboUnmeteredConnection;
	GtkWidget *lblMeteredConnection;
	GtkWidget *cboMeteredConnection;

	// Downloads
	GtkWidget *chkUseIntIconForSmallSizes;
	GtkWidget *chkStoreFileOriginInfo;
	GtkWidget *cboGameTDBPAL;

	// Options
	GtkWidget *chkShowDangerousPermissionsOverlayIcon;
	GtkWidget *chkEnableThumbnailOnNetworkFS;
};

// Interface initialization
static void	rp_options_tab_rp_config_tab_interface_init	(RpConfigTabInterface *iface);
static gboolean	rp_options_tab_has_defaults			(RpOptionsTab	*tab);
static void	rp_options_tab_reset				(RpOptionsTab	*tab);
static void	rp_options_tab_load_defaults			(RpOptionsTab	*tab);
static void	rp_options_tab_save				(RpOptionsTab	*tab,
								 GKeyFile       *keyFile);

// "modified" signal handler for UI widgets
static void	rp_options_tab_modified_handler			(GtkWidget	*widget,
								 RpOptionsTab	*tab);
static void	rp_options_tab_lc_changed_handler		(RpLanguageComboBox *widget,
								 uint32_t	 lc,
								 RpOptionsTab	*tab);
static void	rp_options_tab_chkExtImgDownloadEnabled_toggled	(GtkCheckButton	*checkButton,
								 RpOptionsTab	*tab);

// NOTE: G_DEFINE_TYPE() doesn't work in C++ mode with gcc-6.2
// due to an implicit int to GTypeFlags conversion.
G_DEFINE_TYPE_EXTENDED(RpOptionsTab, rp_options_tab,
	GTK_TYPE_SUPER, static_cast<GTypeFlags>(0),
		G_IMPLEMENT_INTERFACE(RP_TYPE_CONFIG_TAB,
			rp_options_tab_rp_config_tab_interface_init));

static void
rp_options_tab_class_init(RpOptionsTabClass *klass)
{
	RP_UNUSED(klass);
}

static void
rp_options_tab_rp_config_tab_interface_init(RpConfigTabInterface *iface)
{
	iface->has_defaults = (__typeof__(iface->has_defaults))rp_options_tab_has_defaults;
	iface->reset = (__typeof__(iface->reset))rp_options_tab_reset;
	iface->load_defaults = (__typeof__(iface->load_defaults))rp_options_tab_load_defaults;
	iface->save = (__typeof__(iface->save))rp_options_tab_save;
}

static void
rp_options_tab_init(RpOptionsTab *tab)
{
#if GTK_CHECK_VERSION(3,0,0)
	// Make this a VBox.
	gtk_orientable_set_orientation(GTK_ORIENTABLE(tab), GTK_ORIENTATION_VERTICAL);
#endif /* GTK_CHECK_VERSION(3,0,0) */
	gtk_box_set_spacing(GTK_BOX(tab), 8);

	// Create the "Downloads" frame.
	// FIXME: GtkFrame doesn't support mnemonics?
	GtkWidget *const fraDownloads = gtk_frame_new(C_("OptionsTab", "Downloads"));
	gtk_widget_set_name(fraDownloads, "fraDownloads");
	GtkWidget *const vboxDownloads = rp_gtk_vbox_new(6);
	gtk_widget_set_name(vboxDownloads, "vboxDownloads");
#if GTK_CHECK_VERSION(2,91,0)
	gtk_widget_set_margin(vboxDownloads, 6);
	gtk_frame_set_child(GTK_FRAME(fraDownloads), vboxDownloads);
#else /* !GTK_CHECK_VERSION(2,91,0) */
	GtkWidget *const alignDownloads = gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f);
	gtk_widget_set_name(alignDownloads, "alignDownloads");
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignDownloads), 6, 6, 6, 6);
	gtk_container_add(GTK_CONTAINER(alignDownloads), vboxDownloads);
	gtk_frame_set_child(GTK_FRAME(fraDownloads), alignDownloads);
#endif /* GTK_CHECK_VERSION(2,91,0) */

	// Image bandwidth options
	tab->fraExtImgDownloads = gtk_frame_new(nullptr);
	gtk_widget_set_name(tab->fraExtImgDownloads, "fraExtImgDownloads");
	tab->chkExtImgDownloadEnabled = gtk_check_button_new_with_mnemonic(
		convert_accel_to_gtk(C_("OptionsTab", "E&xternal Image Downloads")).c_str());
	gtk_widget_set_name(tab->chkExtImgDownloadEnabled, "chkExtImgDownloadEnabled");
	gtk_frame_set_label_widget(GTK_FRAME(tab->fraExtImgDownloads), tab->chkExtImgDownloadEnabled);

	// Image bandwidth options: Labels and dropdowns
	tab->lblUnmeteredConnection = gtk_label_new(
		C_("OptionsTab", "When using an unlimited\nnetwork connection:"));
	tab->lblMeteredConnection = gtk_label_new(
		C_("OptionsTab", "When using a metered\nnetwork connection:"));
	gtk_widget_set_name(tab->lblUnmeteredConnection, "lblUnmeteredConnection");
	gtk_widget_set_name(tab->lblMeteredConnection, "lblMeteredConnection");

	const char *const s_ImgBandwidthNone      = C_("OptionsTab", "Don't download any images");
	const char *const s_ImgBandwidthNormalRes = C_("OptionsTab", "Download normal-resolution images");
	const char *const s_ImgBandwidthHighRes   = C_("OptionsTab", "Download high-resolution images");

	// GtkListStore model for the combo boxes
	GtkListStore *const lstImgBandwidth = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_list_store_insert_with_values(lstImgBandwidth, nullptr, 0, 0, s_ImgBandwidthNone, -1);
	gtk_list_store_insert_with_values(lstImgBandwidth, nullptr, 1, 0, s_ImgBandwidthNormalRes, -1);
	gtk_list_store_insert_with_values(lstImgBandwidth, nullptr, 2, 0, s_ImgBandwidthHighRes, -1);

	tab->cboUnmeteredConnection = gtk_combo_box_new_with_model(GTK_TREE_MODEL(lstImgBandwidth));
	tab->cboMeteredConnection = gtk_combo_box_new_with_model(GTK_TREE_MODEL(lstImgBandwidth));
	gtk_widget_set_name(tab->cboUnmeteredConnection, "cboUnmeteredConnection");
	gtk_widget_set_name(tab->cboMeteredConnection, "cboMeteredConnection");
	g_object_unref(lstImgBandwidth);	// TODO: Is this correct?

	// Create the cell renderers.
	// NOTE: Using GtkComboBoxText would make this somewhat easier,
	// but then we can't share the SGB/CGB GtkListStores.
	GtkCellRenderer *column = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(tab->cboUnmeteredConnection), column, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(tab->cboUnmeteredConnection),
		column, "text", 0, nullptr);

	column = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(tab->cboMeteredConnection), column, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(tab->cboMeteredConnection),
		column, "text", 0, nullptr);

#if GTK_CHECK_VERSION(3,0,0)
	GtkWidget *const tblImgBandwidth = gtk_grid_new();
	gtk_widget_set_margin(tblImgBandwidth, 6);
	gtk_grid_set_row_spacing(GTK_GRID(tblImgBandwidth), 2);
	gtk_grid_set_column_spacing(GTK_GRID(tblImgBandwidth), 8);
	gtk_grid_attach(GTK_GRID(tblImgBandwidth), tab->lblUnmeteredConnection, 0, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(tblImgBandwidth), tab->cboUnmeteredConnection, 1, 0, 1, 1);
	gtk_grid_attach(GTK_GRID(tblImgBandwidth), tab->lblMeteredConnection, 0, 1, 1, 1);
	gtk_grid_attach(GTK_GRID(tblImgBandwidth), tab->cboMeteredConnection, 1, 1, 1, 1);
	gtk_frame_set_child(GTK_FRAME(tab->fraExtImgDownloads), tblImgBandwidth);
#else /* !GTK_CHECK_VERSION(3,0,0) */
	GtkWidget *const tblImgBandwidth = gtk_table_new(2, 2, false);
	gtk_table_set_row_spacings(GTK_TABLE(tblImgBandwidth), 2);
	gtk_table_set_col_spacings(GTK_TABLE(tblImgBandwidth), 8);
	gtk_table_attach(GTK_TABLE(tblImgBandwidth), tab->lblUnmeteredConnection, 0, 1, 0, 1, GTK_EXPAND, GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(tblImgBandwidth), tab->cboUnmeteredConnection, 1, 2, 0, 1, GTK_EXPAND, GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(tblImgBandwidth), tab->lblMeteredConnection, 0, 1, 1, 2, GTK_EXPAND, GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(tblImgBandwidth), tab->cboMeteredConnection, 1, 2, 1, 2, GTK_EXPAND, GTK_EXPAND, 0, 0);

	GtkWidget *const alignImgBandwidth = gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f);
	gtk_widget_set_name(alignImgBandwidth, "alignImgBandwidth");
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignImgBandwidth), 6, 6, 6, 6);
	gtk_container_add(GTK_CONTAINER(alignImgBandwidth), tblImgBandwidth);
	gtk_frame_set_child(GTK_FRAME(tab->fraExtImgDownloads), alignImgBandwidth);
#endif /* GTK_CHECK_VERSION(3,0,0) */

	// "Downloads" checkboxes.
	tab->chkUseIntIconForSmallSizes = gtk_check_button_new_with_label(
		C_("OptionsTab", "Always use the internal icon (if present) for small sizes."));
	gtk_widget_set_name(tab->chkUseIntIconForSmallSizes, "chkUseIntIconForSmallSizes");
	tab->chkStoreFileOriginInfo = gtk_check_button_new_with_label(
		C_("OptionsTab", "Store cached file origin information using extended attributes.\n"
			"This helps to identify where cached files were downloaded from."));
	gtk_widget_set_name(tab->chkStoreFileOriginInfo, "chkStoreFileOriginInfo");

	// GameTDB PAL hbox.
	GtkWidget *const hboxGameTDBPAL = rp_gtk_hbox_new(6);
	gtk_widget_set_name(hboxGameTDBPAL, "hboxGameTDBPAL");
	GtkWidget *const lblGameTDBPAL = gtk_label_new(C_("OptionsTab", "Language for PAL titles on GameTDB:"));
	gtk_widget_set_name(lblGameTDBPAL, "lblGameTDBPAL");
	tab->cboGameTDBPAL = rp_language_combo_box_new();
	gtk_widget_set_name(tab->cboGameTDBPAL, "cboGameTDBPAL");
	rp_language_combo_box_set_force_pal(RP_LANGUAGE_COMBO_BOX(tab->cboGameTDBPAL), true);
	rp_language_combo_box_set_lcs(RP_LANGUAGE_COMBO_BOX(tab->cboGameTDBPAL), pal_lc);

	// Create the "Options" frame.
	// FIXME: GtkFrame doesn't support mnemonics?
	GtkWidget *const fraOptions = gtk_frame_new(C_("OptionsTab", "Options"));
	gtk_widget_set_name(fraOptions, "fraOptions");
	GtkWidget *const vboxOptions = rp_gtk_vbox_new(6);
	gtk_widget_set_name(vboxOptions, "vboxOptions");
#if GTK_CHECK_VERSION(2,91,0)
	gtk_widget_set_margin(vboxOptions, 6);
	gtk_frame_set_child(GTK_FRAME(fraOptions), vboxOptions);
#else /* !GTK_CHECK_VERSION(2,91,0) */
	GtkWidget *const alignOptions = gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f);
	gtk_widget_set_name(alignOptions, "alignOptions");
	gtk_alignment_set_padding(GTK_ALIGNMENT(alignOptions), 6, 6, 6, 6);
	gtk_container_add(GTK_CONTAINER(alignOptions), vboxOptions);
	gtk_frame_set_child(GTK_FRAME(fraOptions), alignOptions);
#endif /* GTK_CHECK_VERSION(2,91,0) */

	// "Options" checkboxes.
	tab->chkShowDangerousPermissionsOverlayIcon = gtk_check_button_new_with_label(
		C_("OptionsTab", "Show a security overlay icon for ROM images with\n\"dangerous\" permissions."));
	gtk_widget_set_name(tab->chkShowDangerousPermissionsOverlayIcon, "chkShowDangerousPermissionsOverlayIcon");
	tab->chkEnableThumbnailOnNetworkFS = gtk_check_button_new_with_label(
		C_("OptionsTab", "Enable thumbnailing and metadata extraction on network\n"
			"file systems. This may slow down file browsing."));
	gtk_widget_set_name(tab->chkEnableThumbnailOnNetworkFS, "chkEnableThumbnailOnNetworkFS");

	// Connect the signal handlers for the checkboxes.
	// NOTE: Signal handlers are triggered if the value is
	// programmatically edited, unlike Qt, so we'll need to
	// inhibit handling when loading settings.
	g_signal_connect(tab->chkExtImgDownloadEnabled, "toggled",
		G_CALLBACK(rp_options_tab_modified_handler), tab);
	g_signal_connect(tab->chkExtImgDownloadEnabled, "toggled",
		G_CALLBACK(rp_options_tab_chkExtImgDownloadEnabled_toggled), tab);
	g_signal_connect(tab->cboUnmeteredConnection, "changed",
		G_CALLBACK(rp_options_tab_modified_handler), tab);
	g_signal_connect(tab->cboMeteredConnection, "changed",
		G_CALLBACK(rp_options_tab_modified_handler), tab);
	g_signal_connect(tab->chkUseIntIconForSmallSizes, "toggled",
		G_CALLBACK(rp_options_tab_modified_handler), tab);
	g_signal_connect(tab->chkStoreFileOriginInfo, "toggled",
		G_CALLBACK(rp_options_tab_modified_handler), tab);
	g_signal_connect(tab->cboGameTDBPAL, "lc-changed",
		G_CALLBACK(rp_options_tab_lc_changed_handler), tab);

	g_signal_connect(tab->chkShowDangerousPermissionsOverlayIcon, "toggled",
		G_CALLBACK(rp_options_tab_modified_handler), tab);
	g_signal_connect(tab->chkEnableThumbnailOnNetworkFS, "toggled",
		G_CALLBACK(rp_options_tab_modified_handler), tab);

#if GTK_CHECK_VERSION(4,0,0)
	gtk_box_append(GTK_BOX(tab), fraDownloads);
	gtk_box_append(GTK_BOX(vboxDownloads), tab->fraExtImgDownloads);
	gtk_box_append(GTK_BOX(vboxDownloads), tab->chkUseIntIconForSmallSizes);
	gtk_box_append(GTK_BOX(vboxDownloads), tab->chkStoreFileOriginInfo);

	gtk_box_append(GTK_BOX(vboxDownloads), hboxGameTDBPAL);
	gtk_box_append(GTK_BOX(hboxGameTDBPAL), lblGameTDBPAL);
	gtk_box_append(GTK_BOX(hboxGameTDBPAL), tab->cboGameTDBPAL);

	gtk_box_append(GTK_BOX(tab), fraOptions);
	gtk_box_append(GTK_BOX(vboxOptions), tab->chkShowDangerousPermissionsOverlayIcon);
	gtk_box_append(GTK_BOX(vboxOptions), tab->chkEnableThumbnailOnNetworkFS);
#else /* !GTK_CHECK_VERSION(4,0,0) */
	gtk_box_pack_start(GTK_BOX(tab), fraDownloads, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxDownloads), tab->fraExtImgDownloads, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxDownloads), tab->chkUseIntIconForSmallSizes, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxDownloads), tab->chkStoreFileOriginInfo, false, false, 0);

	gtk_box_pack_start(GTK_BOX(vboxDownloads), hboxGameTDBPAL, false, false, 0);
	gtk_box_pack_start(GTK_BOX(hboxGameTDBPAL), lblGameTDBPAL, false, false, 0);
	gtk_box_pack_start(GTK_BOX(hboxGameTDBPAL), tab->cboGameTDBPAL, false, false, 0);

	gtk_box_pack_start(GTK_BOX(tab), fraOptions, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxOptions), tab->chkShowDangerousPermissionsOverlayIcon, false, false, 0);
	gtk_box_pack_start(GTK_BOX(vboxOptions), tab->chkEnableThumbnailOnNetworkFS, false, false, 0);

	gtk_widget_show_all(fraDownloads);
	gtk_widget_show_all(fraOptions);
#endif /* GTK_CHECK_VERSION(4,0,0) */

	// Load the current configuration.
	rp_options_tab_reset(tab);
}

GtkWidget*
rp_options_tab_new(void)
{
	return static_cast<GtkWidget*>(g_object_new(RP_TYPE_OPTIONS_TAB, nullptr));
}

/** RpConfigTab interface functions **/

static gboolean
rp_options_tab_has_defaults(RpOptionsTab *tab)
{
	g_return_val_if_fail(RP_IS_OPTIONS_TAB(tab), FALSE);
	return TRUE;
}

static void
rp_options_tab_reset(RpOptionsTab *tab)
{
	g_return_if_fail(RP_IS_OPTIONS_TAB(tab));

	// NOTE: This may re-check the configuration timestamp.
	const Config *const config = Config::instance();

	tab->inhibit = true;

	// Downloads
	gtk_check_button_set_active(
		GTK_CHECK_BUTTON(tab->chkExtImgDownloadEnabled),
		config->extImgDownloadEnabled());
	gtk_check_button_set_active(
		GTK_CHECK_BUTTON(tab->chkUseIntIconForSmallSizes),
		config->useIntIconForSmallSizes());
	gtk_check_button_set_active(
		GTK_CHECK_BUTTON(tab->chkStoreFileOriginInfo),
		config->storeFileOriginInfo());

	// Image bandwidth options
	gtk_combo_box_set_active(
		GTK_COMBO_BOX(tab->cboUnmeteredConnection),
		static_cast<int>(config->imgBandwidthUnmetered()));
	gtk_combo_box_set_active(
		GTK_COMBO_BOX(tab->cboMeteredConnection),
		static_cast<int>(config->imgBandwidthMetered()));
	// Update sensitivity
	rp_options_tab_chkExtImgDownloadEnabled_toggled(GTK_CHECK_BUTTON(tab->chkExtImgDownloadEnabled), tab);

	// Options
	gtk_check_button_set_active(
		GTK_CHECK_BUTTON(tab->chkShowDangerousPermissionsOverlayIcon),
		config->showDangerousPermissionsOverlayIcon());
	gtk_check_button_set_active(
		GTK_CHECK_BUTTON(tab->chkEnableThumbnailOnNetworkFS),
		config->enableThumbnailOnNetworkFS());

	// PAL language code
	rp_language_combo_box_set_selected_lc(RP_LANGUAGE_COMBO_BOX(tab->cboGameTDBPAL), config->palLanguageForGameTDB());

	tab->changed = false;
	tab->inhibit = false;
}

static void
rp_options_tab_load_defaults(RpOptionsTab *tab)
{
	g_return_if_fail(RP_IS_OPTIONS_TAB(tab));
	tab->inhibit = true;

	// TODO: Get the defaults from Config.
	// For now, hard-coding everything here.

	// Downloads
	static const bool extImgDownloadEnabled_default = true;
	static const bool useIntIconForSmallSizes_default = true;
	static const bool storeFileOriginInfo_default = true;
	static const int palLanguageForGameTDB_default = pal_lc_idx_def;	// cboGameTDBPAL index ('en')

	// Image bandwidth options
	static const Config::ImgBandwidth imgBandwidthUnmetered_default = Config::ImgBandwidth::HighRes;
	static const Config::ImgBandwidth imgBandwidthMetered_default = Config::ImgBandwidth::NormalRes;

	// Options
	static const bool showDangerousPermissionsOverlayIcon_default = true;
	static const bool enableThumbnailOnNetworkFS_default = false;
	bool isDefChanged = false;

#define COMPARE_CHK(widget, defval) \
	(gtk_check_button_get_active(GTK_CHECK_BUTTON(widget)) != (defval))
#define COMPARE_CBO(widget, defval) \
	(gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) != (defval))

	// Downloads
	if (COMPARE_CHK(tab->chkExtImgDownloadEnabled, extImgDownloadEnabled_default)) {
		gtk_check_button_set_active(
			GTK_CHECK_BUTTON(tab->chkExtImgDownloadEnabled),
			extImgDownloadEnabled_default);
		isDefChanged = true;
		// Update sensitivity
		rp_options_tab_chkExtImgDownloadEnabled_toggled(GTK_CHECK_BUTTON(tab->chkExtImgDownloadEnabled), tab);
	}
	if (COMPARE_CHK(tab->chkUseIntIconForSmallSizes, useIntIconForSmallSizes_default)) {
		gtk_check_button_set_active(
			GTK_CHECK_BUTTON(tab->chkUseIntIconForSmallSizes),
			extImgDownloadEnabled_default);
		isDefChanged = true;
	}
	if (COMPARE_CHK(tab->chkUseIntIconForSmallSizes, useIntIconForSmallSizes_default)) {
		gtk_check_button_set_active(
			GTK_CHECK_BUTTON(tab->chkUseIntIconForSmallSizes),
			useIntIconForSmallSizes_default);
		isDefChanged = true;
	}
	if (COMPARE_CHK(tab->chkStoreFileOriginInfo, storeFileOriginInfo_default)) {
		gtk_check_button_set_active(
			GTK_CHECK_BUTTON(tab->chkStoreFileOriginInfo),
			storeFileOriginInfo_default);
		isDefChanged = true;
	}
	if (COMPARE_CBO(tab->cboGameTDBPAL, palLanguageForGameTDB_default)) {
		gtk_combo_box_set_active(
			GTK_COMBO_BOX(tab->cboGameTDBPAL),
			palLanguageForGameTDB_default);
		isDefChanged = true;
	}

	// Image bandwidth options
	if (COMPARE_CBO(tab->cboUnmeteredConnection, static_cast<int>(imgBandwidthUnmetered_default))) {
		gtk_combo_box_set_active(
			GTK_COMBO_BOX(tab->cboUnmeteredConnection),
			static_cast<int>(imgBandwidthUnmetered_default));
		isDefChanged = true;
	}
	if (COMPARE_CBO(tab->cboMeteredConnection, static_cast<int>(imgBandwidthMetered_default))) {
		gtk_combo_box_set_active(
			GTK_COMBO_BOX(tab->cboMeteredConnection),
			static_cast<int>(imgBandwidthMetered_default));
		isDefChanged = true;
	}

	// Options
	if (COMPARE_CHK(tab->chkShowDangerousPermissionsOverlayIcon, showDangerousPermissionsOverlayIcon_default)) {
		gtk_check_button_set_active(
			GTK_CHECK_BUTTON(tab->chkShowDangerousPermissionsOverlayIcon),
			showDangerousPermissionsOverlayIcon_default);
		isDefChanged = true;
	}
	if (COMPARE_CHK(tab->chkEnableThumbnailOnNetworkFS, enableThumbnailOnNetworkFS_default)) {
		gtk_check_button_set_active(
			GTK_CHECK_BUTTON(tab->chkEnableThumbnailOnNetworkFS),
			enableThumbnailOnNetworkFS_default);
		isDefChanged = true;
	}

	if (isDefChanged) {
		tab->changed = true;
		g_signal_emit_by_name(tab, "modified", NULL);
	}
	tab->inhibit = false;
}

static void
rp_options_tab_save(RpOptionsTab *tab, GKeyFile *keyFile)
{
	g_return_if_fail(RP_IS_OPTIONS_TAB(tab));
	g_return_if_fail(keyFile != nullptr);

	if (!tab->changed) {
		// Configuration was not changed.
		return;
	}

	// Save the configuration.

	// Downloads
	g_key_file_set_boolean(keyFile, "Downloads", "ExtImageDownload",
		gtk_check_button_get_active(GTK_CHECK_BUTTON(tab->chkExtImgDownloadEnabled)));
	g_key_file_set_boolean(keyFile, "Downloads", "UseIntIconForSmallSizes",
		gtk_check_button_get_active(GTK_CHECK_BUTTON(tab->chkUseIntIconForSmallSizes)));
	g_key_file_set_boolean(keyFile, "Downloads", "StoreFileOriginInfo",
		gtk_check_button_get_active(GTK_CHECK_BUTTON(tab->chkStoreFileOriginInfo)));
	g_key_file_set_string(keyFile, "Downloads", "PalLanguageForGameTDB",
		SystemRegion::lcToString(rp_language_combo_box_get_selected_lc(
			RP_LANGUAGE_COMBO_BOX(tab->cboGameTDBPAL))).c_str());

	// Image bandwidth options
	// TODO: Consolidate this.
	const char *sUnmetered, *sMetered;
	switch (static_cast<Config::ImgBandwidth>(gtk_combo_box_get_active(GTK_COMBO_BOX(tab->cboUnmeteredConnection)))) {
		case Config::ImgBandwidth::None:
			sUnmetered = "None";
			break;
		case Config::ImgBandwidth::NormalRes:
			sUnmetered = "NormalRes";
			break;
		case Config::ImgBandwidth::HighRes:
		default:
			sUnmetered = "HighRes";
			break;
	}
	switch (static_cast<Config::ImgBandwidth>(gtk_combo_box_get_active(GTK_COMBO_BOX(tab->cboMeteredConnection)))) {
		case Config::ImgBandwidth::None:
			sMetered = "None";
			break;
		case Config::ImgBandwidth::NormalRes:
		default:
			sMetered = "NormalRes";
			break;
		case Config::ImgBandwidth::HighRes:
			sMetered = "HighRes";
			break;
	}
	g_key_file_set_string(keyFile, "Downloads", "ImgBandwidthUnmetered", sUnmetered);
	g_key_file_set_string(keyFile, "Downloads", "ImgBandwidthMetered", sMetered);
	// Remove the old option
	g_key_file_remove_key(keyFile, "Downloads", "DownloadHighResScans", nullptr);

	// Options
	g_key_file_set_boolean(keyFile, "Options", "ShowDangerousPermissionsOverlayIcon",
		gtk_check_button_get_active(GTK_CHECK_BUTTON(tab->chkShowDangerousPermissionsOverlayIcon)));
	g_key_file_set_boolean(keyFile, "Options", "EnableThumbnailOnNetworkFS",
		gtk_check_button_get_active(GTK_CHECK_BUTTON(tab->chkEnableThumbnailOnNetworkFS)));

	// Configuration saved.
	tab->changed = false;
}

/** Signal handlers **/

/**
 * "modified" signal handler for UI widgets
 * @param widget Widget sending the signal
 * @param tab OptionsTab
 */
static void
rp_options_tab_modified_handler(GtkWidget *widget, RpOptionsTab *tab)
{
	RP_UNUSED(widget);
	if (tab->inhibit)
		return;

	// Forward the "modified" signal.
	tab->changed = true;
	g_signal_emit_by_name(tab, "modified", NULL);
}

/**
 * "lc-changed" signal handler for RpLanguageComboBox
 * @param widget RpLanguageComboBox
 * @param lc New language code
 * @param tab OptionsTab
 */
static void
rp_options_tab_lc_changed_handler(RpLanguageComboBox *widget,
				  uint32_t	 lc,
				  RpOptionsTab	*tab)
{
	RP_UNUSED(widget);
	RP_UNUSED(lc);
	if (tab->inhibit)
		return;

	// Forward the "modified" signal.
	tab->changed = true;
	g_signal_emit_by_name(tab, "modified", NULL);
}

/**
 * chkExtImgDownloadEnabled was toggled.
 *
 * This handles enabling fraExtImgDownloads widgets, since we can't just
 * disable the entire frame; otherwise, the checkbox also gets disabled.
 *
 * @param checkButton chkExtImgDownloadEnabled
 * @param tab OptionsTab
 */
static void
rp_options_tab_chkExtImgDownloadEnabled_toggled(GtkCheckButton *checkButton, RpOptionsTab *tab)
{
	const bool enable = gtk_check_button_get_active(checkButton);
	gtk_widget_set_sensitive(tab->lblUnmeteredConnection, enable);
	gtk_widget_set_sensitive(tab->cboUnmeteredConnection, enable);
	gtk_widget_set_sensitive(tab->lblMeteredConnection, enable);
	gtk_widget_set_sensitive(tab->cboMeteredConnection, enable);
}
