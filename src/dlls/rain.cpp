/***
*
*	2023, Magic Nipples.
*
*	Use and modification of this code is allowed as long
*	as credit to the original creators is provided! Enjoy!
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "rain.h"



TYPEDESCRIPTION	CRainSettings::m_SaveData[] =
{
	DEFINE_FIELD(CRainSettings, Rain_Distance, FIELD_FLOAT),
	DEFINE_FIELD(CRainSettings, Rain_Mode, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CRainSettings, CBaseEntity);


LINK_ENTITY_TO_CLASS(rain_settings, CRainSettings);

void CRainSettings::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
}

void CRainSettings::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_flDistance"))
	{
		Rain_Distance = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iMode"))
	{
		Rain_Mode = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseEntity::KeyValue(pkvd);
	}
}



TYPEDESCRIPTION	CRainModify::m_SaveData[] =
{
	DEFINE_FIELD(CRainModify, fadeTime, FIELD_FLOAT),
	DEFINE_FIELD(CRainModify, Rain_Drips, FIELD_INTEGER),
	DEFINE_FIELD(CRainModify, Rain_randX, FIELD_FLOAT),
	DEFINE_FIELD(CRainModify, Rain_randY, FIELD_FLOAT),
	DEFINE_FIELD(CRainModify, Rain_windX, FIELD_FLOAT),
	DEFINE_FIELD(CRainModify, Rain_windY, FIELD_FLOAT),
};
IMPLEMENT_SAVERESTORE(CRainModify, CBaseEntity);


LINK_ENTITY_TO_CLASS(rain_modify, CRainModify);

void CRainModify::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;

	if (FStringNull(pev->targetname))
		pev->spawnflags |= 1;
}

void CRainModify::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iDripsPerSecond"))
	{
		Rain_Drips = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flWindX"))
	{
		Rain_windX = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flWindY"))
	{
		Rain_windY = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flRandX"))
	{
		Rain_randX = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flRandY"))
	{
		Rain_randY = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flTime"))
	{
		fadeTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseEntity::KeyValue(pkvd);
	}
}

void CRainModify::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (pev->spawnflags & 1)
		return; // constant

	if (gpGlobals->deathmatch)
	{
		ALERT(at_console, "Rain error: only static rain in multiplayer\n");
		return; // not in multiplayer
	}

	CBasePlayer* pPlayer;
	pPlayer = (CBasePlayer*)CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(1));

	if (fadeTime)
	{ // write to 'ideal' settings
		pPlayer->Rain_ideal_dripsPerSecond = Rain_Drips;
		pPlayer->Rain_ideal_randX = Rain_randX;
		pPlayer->Rain_ideal_randY = Rain_randY;
		pPlayer->Rain_ideal_windX = Rain_windX;
		pPlayer->Rain_ideal_windY = Rain_windY;

		pPlayer->Rain_endFade = gpGlobals->time + fadeTime;
		pPlayer->Rain_nextFadeUpdate = gpGlobals->time + 1;
	}
	else
	{
		pPlayer->Rain_dripsPerSecond = Rain_Drips;
		pPlayer->Rain_randX = Rain_randX;
		pPlayer->Rain_randY = Rain_randY;
		pPlayer->Rain_windX = Rain_windX;
		pPlayer->Rain_windY = Rain_windY;

		pPlayer->Rain_needsUpdate = 1;
	}
}