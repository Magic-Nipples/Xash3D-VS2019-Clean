/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "rain.h"

#define MAX_CLIENTS 32

/// USER-DEFINED SERVER MESSAGE HANDLERS

int CHud :: MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf )
{
	ASSERT( iSize == 0 );

	// clear all hud data
	HUDLIST *pList = m_pHudList;

	while ( pList )
	{
		if ( pList->p )
			pList->p->Reset();
		pList = pList->pNext;
	}

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	return 1;
}

void CAM_ToFirstPerson(void);

void CHud :: MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	CAM_ToFirstPerson();
}

void CHud :: MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if ( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}
}


int CHud :: MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_Teamplay = READ_BYTE();

	return 1;
}


int CHud :: MsgFunc_Damage(const char *pszName, int iSize, void *pbuf )
{
	int		armor, blood;
	Vector	from;
	int		i;
	float	count;
	
	BEGIN_READ( pbuf, iSize );
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i=0 ; i<3 ; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually

	return 1;
}

int CHud :: MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iConcussionEffect = READ_BYTE();
	if (m_iConcussionEffect)
		this->m_StatusIcons.EnableIcon("dmg_concuss",255,160,0);
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return 1;
}

int CHud::MsgFunc_AddELight(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	dlight_t* dl = gEngfuncs.pEfxAPI->CL_AllocElight(READ_SHORT());

	int bELightActive = READ_BYTE();
	if (bELightActive <= 0)
	{
		dl->die = gEngfuncs.GetClientTime();
	}
	else
	{
		dl->die = gEngfuncs.GetClientTime() + 1E6;

		dl->origin[0] = READ_COORD();
		dl->origin[1] = READ_COORD();
		dl->origin[2] = READ_COORD();
		dl->radius = READ_COORD();
		dl->color.r = READ_BYTE();
		dl->color.g = READ_BYTE();
		dl->color.b = READ_BYTE();
	}
	return 1;
}

void CHud::MsgFunc_SetFog(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	gHUD.FogColor.x = READ_BYTE();
	gHUD.FogColor.y = READ_BYTE();
	gHUD.FogColor.z = READ_BYTE();
	gHUD.g_fFadeDuration = READ_BYTE();
	gHUD.g_fStartDist = READ_SHORT();

	if (gHUD.g_fFadeDuration > 0)
		gHUD.g_ftargetValue = READ_SHORT();
	else
		gHUD.g_fFinalValue = gHUD.g_iStartValue = READ_SHORT();

	gHUD.g_fskybox = READ_BYTE();
}

extern rain_properties Rain;
int CHud::MsgFunc_RainData(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	Rain.dripsPerSecond = READ_SHORT();
	Rain.distFromPlayer = READ_COORD();
	Rain.windX = READ_COORD();
	Rain.windY = READ_COORD();
	Rain.randX = READ_COORD();
	Rain.randY = READ_COORD();
	Rain.weatherMode = READ_SHORT();
	Rain.globalHeight = READ_COORD();
	return 1;
}

int CHud::MsgFunc_ShadowInfo(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	m_fShadowAngle[0] = (float)READ_COORD();
	m_fShadowAngle[1] = (float)READ_COORD();
	m_fShadowAngle[2] = (float)READ_COORD();
	m_fShadowAlpha = (float)READ_COORD();

	m_iShadowColor[0] = READ_COORD();
	m_iShadowColor[1] = READ_COORD();
	m_iShadowColor[2] = READ_COORD();

	if(m_iShadowColor[0] != 0)
		m_iShadowColor[0] /= 255;

	if (m_iShadowColor[1] != 0)
		m_iShadowColor[1] /= 255;

	if (m_iShadowColor[2] != 0)
		m_iShadowColor[2] /= 255;

	//gEngfuncs.Con_DPrintf( "CLIENT: %0.2f %0.2f %0.2f %0.2f\n", m_fShadowAngle[0], m_fShadowAngle[1], m_fShadowAngle[2], m_fShadowAlpha);
	//gEngfuncs.Con_DPrintf( "CLIENT: %0.2f %0.2f %0.2f\n", m_iShadowColor[0], m_iShadowColor[1], m_iShadowColor[2]);

	return 1;
}