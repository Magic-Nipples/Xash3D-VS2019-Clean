/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "extdll.h"
#include "basemenu.h"
#include "utils.h"
#include "menu_btnsbmp_table.h"

#define ART_BANNER			"gfx/shell/head_audio"

#define ID_BACKGROUND		0
#define ID_BANNER			1
#define ID_DONE			2
#define ID_SUITVOLUME		3
#define ID_SOUNDVOLUME		4
#define ID_MUSICVOLUME		5
#define ID_INTERP			6
#define ID_NODSP			7
#define ID_MSGHINT			8

typedef struct
{
	float		soundVolume;
	float		musicVolume;
	float		suitVolume;
} uiAudioValues_t;

static uiAudioValues_t	uiAudioInitial;

typedef struct
{
	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuPicButton_s	done;

	menuSlider_s	soundVolume;
	menuSlider_s	musicVolume;
	menuSlider_s	suitVolume;
	//menuCheckBox_s	lerping;
	menuSpinControl_s	lerping;
	menuCheckBox_s	noDSP;
} uiAudio_t;

static uiAudio_t		uiAudio;

/*
=================
UI_Audio_GetConfig
=================
*/
static void UI_Audio_GetConfig( void )
{
	uiAudio.soundVolume.curValue = CVAR_GET_FLOAT( "volume" );
	uiAudio.musicVolume.curValue = CVAR_GET_FLOAT( "MP3Volume" );
	uiAudio.suitVolume.curValue = CVAR_GET_FLOAT( "suitvolume" );
	uiAudio.lerping.curValue = CVAR_GET_FLOAT("s_lerping");

	if( CVAR_GET_FLOAT( "dsp_off" ))
		uiAudio.noDSP.enabled = 1;

	// save initial values
	uiAudioInitial.soundVolume = uiAudio.soundVolume.curValue;
	uiAudioInitial.musicVolume = uiAudio.musicVolume.curValue;
	uiAudioInitial.suitVolume = uiAudio.suitVolume.curValue;
	
}

/*
=================
UI_Audio_SetConfig
=================
*/
static void UI_Audio_SetConfig( void )
{
	CVAR_SET_FLOAT("volume", uiAudio.soundVolume.curValue);
	CVAR_SET_FLOAT("MP3Volume", uiAudio.musicVolume.curValue);
	CVAR_SET_FLOAT("suitvolume", uiAudio.suitVolume.curValue);
	CVAR_SET_FLOAT("s_lerping", uiAudio.lerping.curValue);
	CVAR_SET_FLOAT("dsp_off", uiAudio.noDSP.enabled);
}

/*
=================
UI_Audio_UpdateConfig
=================
*/
static void UI_Audio_UpdateConfig(void)
{
	static char	interpText[8];

	CVAR_SET_FLOAT("volume", uiAudio.soundVolume.curValue);
	CVAR_SET_FLOAT("MP3Volume", uiAudio.musicVolume.curValue);
	CVAR_SET_FLOAT("suitvolume", uiAudio.suitVolume.curValue);
	CVAR_SET_FLOAT("dsp_off", uiAudio.noDSP.enabled);

	if (uiAudio.lerping.curValue <= 0)
		sprintf(interpText, "Off");
	else if (uiAudio.lerping.curValue == 1)
		sprintf(interpText, "Linear");
	else if (uiAudio.lerping.curValue == 2)
		sprintf(interpText, "Cubic");
	else
		sprintf(interpText, "ERROR");

	uiAudio.lerping.generic.name = interpText;

	CVAR_SET_FLOAT("s_lerping", uiAudio.lerping.curValue);
}

/*
=================
UI_Audio_Callback
=================
*/
static void UI_Audio_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	switch( item->id )
	{
	//case ID_INTERP:
	case ID_NODSP:
		if( event == QM_PRESSED )
			((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_PRESSED;
		else ((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_FOCUS;
		break;
	}

	if( event == QM_CHANGED )
	{
		UI_Audio_UpdateConfig();
		return;
	}

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
		UI_PopMenu();
		break;
	}
}

static void UI_Audio_Ownerdraw(void* self)
{
	if (CVAR_GET_FLOAT("cl_background") != 1)
		UI_DrawPic(((menuBitmap_s*)self)->generic.x, ((menuBitmap_s*)self)->generic.y, ((menuBitmap_s*)self)->generic.width, ((menuBitmap_s*)self)->generic.height, uiColorWhite, ((menuBitmap_s*)self)->pic);

	DrawBoldString(320 * uiStatic.scaleX, 440 * uiStatic.scaleY,
		"Sound interpolation:");
}

/*
=================
UI_Audio_Init
=================
*/
static void UI_Audio_Init( void )
{
	memset( &uiAudio, 0, sizeof( uiAudio_t ));

	uiAudio.menu.vidInitFunc = UI_Audio_Init;
	
	uiAudio.background.generic.id	= ID_BACKGROUND;
	uiAudio.background.generic.type = QMTYPE_BITMAP;
	uiAudio.background.generic.flags = QMF_INACTIVE;
	uiAudio.background.generic.x = 0;
	uiAudio.background.generic.y = 0;
	uiAudio.background.generic.width = 1024;
	uiAudio.background.generic.height = 768;
	uiAudio.background.pic = ART_BACKGROUND;
	uiAudio.background.generic.ownerdraw = UI_Audio_Ownerdraw;

	uiAudio.banner.generic.id = ID_BANNER;
	uiAudio.banner.generic.type = QMTYPE_BITMAP;
	uiAudio.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiAudio.banner.generic.x = UI_BANNER_POSX;
	uiAudio.banner.generic.y = UI_BANNER_POSY;
	uiAudio.banner.generic.width = UI_BANNER_WIDTH;
	uiAudio.banner.generic.height	= UI_BANNER_HEIGHT;
	uiAudio.banner.pic = ART_BANNER;

	uiAudio.done.generic.id = ID_DONE;
	uiAudio.done.generic.type = QMTYPE_BM_BUTTON;
	uiAudio.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiAudio.done.generic.x = UI_SELECTION_POSX;
	uiAudio.done.generic.y = 230;
	uiAudio.done.generic.name = "Done";
	uiAudio.done.generic.statusText = "Go back to the Configuration Menu";
	uiAudio.done.generic.callback = UI_Audio_Callback;

	UI_UtilSetupPicButton( &uiAudio.done, PC_DONE );

	uiAudio.soundVolume.generic.id = ID_SOUNDVOLUME;
	uiAudio.soundVolume.generic.type = QMTYPE_SLIDER;
	uiAudio.soundVolume.generic.flags = QMF_PULSEIFFOCUS|QMF_DROPSHADOW;
	uiAudio.soundVolume.generic.name = "Game sound volume";
	uiAudio.soundVolume.generic.x = 320;
	uiAudio.soundVolume.generic.y = 280;
	uiAudio.soundVolume.generic.callback = UI_Audio_Callback;
	uiAudio.soundVolume.generic.statusText = "Set master volume level";
	uiAudio.soundVolume.minValue	= 0.0;
	uiAudio.soundVolume.maxValue	= 1.0;
	uiAudio.soundVolume.range = 0.05f;

	uiAudio.musicVolume.generic.id = ID_MUSICVOLUME;
	uiAudio.musicVolume.generic.type = QMTYPE_SLIDER;
	uiAudio.musicVolume.generic.flags = QMF_PULSEIFFOCUS|QMF_DROPSHADOW;
	uiAudio.musicVolume.generic.name = "Game music volume";
	uiAudio.musicVolume.generic.x = 320;
	uiAudio.musicVolume.generic.y = 340;
	uiAudio.musicVolume.generic.callback = UI_Audio_Callback;
	uiAudio.musicVolume.generic.statusText = "Set background music volume level";
	uiAudio.musicVolume.minValue = 0.0;
	uiAudio.musicVolume.maxValue = 1.0;
	uiAudio.musicVolume.range = 0.05f;

	uiAudio.suitVolume.generic.id = ID_SUITVOLUME;
	uiAudio.suitVolume.generic.type = QMTYPE_SLIDER;
	uiAudio.suitVolume.generic.flags = QMF_PULSEIFFOCUS|QMF_DROPSHADOW;
	uiAudio.suitVolume.generic.name = "Suit volume";
	uiAudio.suitVolume.generic.x = 320;
	uiAudio.suitVolume.generic.y = 400;
	uiAudio.suitVolume.generic.callback = UI_Audio_Callback;
	uiAudio.suitVolume.generic.statusText = "Singleplayer suit volume";
	uiAudio.suitVolume.minValue = 0.0;
	uiAudio.suitVolume.maxValue = 1.0;
	uiAudio.suitVolume.range = 0.05f;

	uiAudio.lerping.generic.id = ID_INTERP;
	uiAudio.lerping.generic.type = QMTYPE_SPINCONTROL;
	uiAudio.lerping.generic.flags = QMF_CENTER_JUSTIFY | QMF_HIGHLIGHTIFFOCUS | QMF_DROPSHADOW;
	uiAudio.lerping.generic.x = 348;
	uiAudio.lerping.generic.y = 470;
	uiAudio.lerping.generic.width = 168;
	uiAudio.lerping.generic.height = 26;
	uiAudio.lerping.generic.callback = UI_Audio_Callback;
	uiAudio.lerping.generic.statusText = "change interpolation on sound output";
	uiAudio.lerping.minValue = 0;
	uiAudio.lerping.maxValue = 2;
	uiAudio.lerping.range = 1;

	uiAudio.noDSP.generic.id = ID_NODSP;
	uiAudio.noDSP.generic.type = QMTYPE_CHECKBOX;
	uiAudio.noDSP.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_ACT_ONRELEASE|QMF_MOUSEONLY|QMF_DROPSHADOW;
	uiAudio.noDSP.generic.name = "Disable DSP effects";
	uiAudio.noDSP.generic.x = 312;
	uiAudio.noDSP.generic.y = 520;
	uiAudio.noDSP.generic.callback = UI_Audio_Callback;
	uiAudio.noDSP.generic.statusText = "this disables sound processing (like echo, flanger etc)";

	UI_Audio_GetConfig();

	UI_AddItem( &uiAudio.menu, (void *)&uiAudio.background );
	UI_AddItem( &uiAudio.menu, (void *)&uiAudio.banner );
	UI_AddItem( &uiAudio.menu, (void *)&uiAudio.done );
	UI_AddItem( &uiAudio.menu, (void *)&uiAudio.soundVolume );
	UI_AddItem( &uiAudio.menu, (void *)&uiAudio.musicVolume );
	UI_AddItem( &uiAudio.menu, (void *)&uiAudio.suitVolume );
	UI_AddItem( &uiAudio.menu, (void *)&uiAudio.lerping );
	UI_AddItem( &uiAudio.menu, (void *)&uiAudio.noDSP );
}

/*
=================
UI_Audio_Precache
=================
*/
void UI_Audio_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_Audio_Menu
=================
*/
void UI_Audio_Menu( void )
{
	UI_Audio_Precache();
	UI_Audio_Init();

	UI_Audio_UpdateConfig();
	UI_PushMenu( &uiAudio.menu );
}