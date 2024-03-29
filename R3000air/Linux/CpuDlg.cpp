/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
 
 #include "Linux.h"
 
GtkWidget *CpuDlg;
GtkWidget *check_eerec, *check_vu0rec, *check_vu1rec;

void OnCpu_Ok(GtkButton *button, gpointer user_data)
{
	u32 newopts = 0;

	if is_checked(CpuDlg, "GtkCheckButton_EERec") newopts |= PCSX2_EEREC;
	if is_checked(CpuDlg, "GtkCheckButton_VU0rec") newopts |= PCSX2_VU0REC;
	if is_checked(CpuDlg, "GtkCheckButton_VU1rec") newopts |= PCSX2_VU1REC;
	if is_checked(CpuDlg, "GtkCheckButton_microVU0rec") newopts |= PCSX2_MICROVU0;
	if is_checked(CpuDlg, "GtkCheckButton_microVU1rec") newopts |= PCSX2_MICROVU1;
	if is_checked(CpuDlg, "GtkCheckButton_MTGS") newopts |= PCSX2_GSMULTITHREAD;
	
	if is_checked(CpuDlg, "GtkRadioButton_LimitNormal")
		newopts |= PCSX2_FRAMELIMIT_NORMAL;
	else if is_checked(CpuDlg, "GtkRadioButton_LimitLimit")
		newopts |= PCSX2_FRAMELIMIT_LIMIT;
	else if is_checked(CpuDlg, "GtkRadioButton_LimitFS")
		newopts |= PCSX2_FRAMELIMIT_SKIP;
	
	Config.CustomFps = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "CustomFPSLimit")));
	Config.CustomFrameSkip = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FrameThreshold")));
	Config.CustomConsecutiveFrames = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FramesBeforeSkipping")));
	Config.CustomConsecutiveSkip = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FramesToSkip")));

	if (Config.Options != newopts)
	{
		SysRestorableReset();

		if ((Config.Options&PCSX2_GSMULTITHREAD) ^(newopts&PCSX2_GSMULTITHREAD))
		{
			// Need the MTGS setting to take effect, so close out the plugins:
			PluginsResetGS();

			if (CHECK_MULTIGS)
				Console::Notice("MTGS mode disabled.\n\tEnjoy the fruits of single-threaded simpicity.");
			else
				Console::Notice("MTGS mode enabled.\n\tWelcome to multi-threaded awesomeness.");
		}

		Config.Options = newopts;
	}
	else
		UpdateVSyncRate();

	SaveConfig();

	gtk_widget_destroy(CpuDlg);
	if (MainWindow) gtk_widget_set_sensitive(MainWindow, TRUE);
	gtk_main_quit();
}

void DisableMicroVU()
{
	if is_checked(CpuDlg, "GtkCheckButton_VU0rec") 
		gtk_widget_set_sensitive(lookup_widget(CpuDlg, "GtkCheckButton_microVU0rec"), true);
	else
		gtk_widget_set_sensitive(lookup_widget(CpuDlg, "GtkCheckButton_microVU0rec"), false);
	
	if is_checked(CpuDlg, "GtkCheckButton_VU1rec") 
		gtk_widget_set_sensitive(lookup_widget(CpuDlg, "GtkCheckButton_microVU1rec"), true);
	else
		gtk_widget_set_sensitive(lookup_widget(CpuDlg, "GtkCheckButton_microVU1rec"), false);
}

void OnConf_Cpu(GtkMenuItem *menuitem, gpointer user_data)
{
	char str[512];

	CpuDlg = create_CpuDlg();
	gtk_window_set_title(GTK_WINDOW(CpuDlg), _("Configuration"));

	set_checked(CpuDlg, "GtkCheckButton_EERec", !!CHECK_EEREC);
	set_checked(CpuDlg, "GtkCheckButton_VU0rec", !!CHECK_VU0REC);
	set_checked(CpuDlg, "GtkCheckButton_VU1rec", !!CHECK_VU1REC);
	set_checked(CpuDlg, "GtkCheckButton_microVU0rec", !!CHECK_MICROVU0);
	set_checked(CpuDlg, "GtkCheckButton_microVU1rec", !!CHECK_MICROVU1);
	set_checked(CpuDlg, "GtkCheckButton_MTGS", !!CHECK_MULTIGS);
	set_checked(CpuDlg, "GtkRadioButton_LimitNormal", CHECK_FRAMELIMIT == PCSX2_FRAMELIMIT_NORMAL);
	set_checked(CpuDlg, "GtkRadioButton_LimitLimit", CHECK_FRAMELIMIT == PCSX2_FRAMELIMIT_LIMIT);
	set_checked(CpuDlg, "GtkRadioButton_LimitFS", CHECK_FRAMELIMIT == PCSX2_FRAMELIMIT_SKIP);
	DisableMicroVU();
	
	sprintf(str, "Cpu Vendor:     %s", cpuinfo.x86ID);
	gtk_label_set_text(GTK_LABEL(lookup_widget(CpuDlg, "GtkLabel_CpuVendor")), str);
	sprintf(str, "Familly:   %s", cpuinfo.x86Fam);
	gtk_label_set_text(GTK_LABEL(lookup_widget(CpuDlg, "GtkLabel_Family")), str);
	sprintf(str, "Cpu Speed:   %d MHZ", cpuinfo.cpuspeed);
	gtk_label_set_text(GTK_LABEL(lookup_widget(CpuDlg, "GtkLabel_CpuSpeed")), str);

	strcpy(str, "Features:    ");
	if (cpucaps.hasMultimediaExtensions) strcat(str, "MMX");
	if (cpucaps.hasStreamingSIMDExtensions) strcat(str, ",SSE");
	if (cpucaps.hasStreamingSIMD2Extensions) strcat(str, ",SSE2");
	if (cpucaps.hasStreamingSIMD3Extensions) strcat(str, ",SSE3");
	if (cpucaps.hasAMD64BitArchitecture) strcat(str, ",x86-64");
	gtk_label_set_text(GTK_LABEL(lookup_widget(CpuDlg, "GtkLabel_Features")), str);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "CustomFPSLimit")), (gdouble)Config.CustomFps);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FrameThreshold")), (gdouble)Config.CustomFrameSkip);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FramesBeforeSkipping")), (gdouble)Config.CustomConsecutiveFrames);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(lookup_widget(CpuDlg, "FramesToSkip")), (gdouble)Config.CustomConsecutiveSkip);

	gtk_widget_show_all(CpuDlg);
	if (MainWindow) gtk_widget_set_sensitive(MainWindow, FALSE);
	gtk_main();
}

void OnCpuCheckToggled(GtkToggleButton *togglebutton, gpointer user_data)
{
	DisableMicroVU();
}
