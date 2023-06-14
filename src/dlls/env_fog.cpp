/***
*
*	Copyright (c) 2023, Magic Nipples.
*
*	Use and modification of this code is allowed as long
*	as credit is provided! Enjoy!
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

//=========================================================
// fog server side entity based on LRC's
//=========================================================
#define SF_FOG_ACTIVE 1
#define SF_FOG_SKY 2

extern int gmsgSetFog;

class CEnvFog : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache(void);

	void EXPORT ResumeThink(void);
	void EXPORT StartThink(void);

	void TurnOn(void);
	void TurnOff(void);

	void SendData(Vector col, int fFadeTime, int StartDist, int iEndDist);
	void KeyValue(KeyValueData* pkvd);
	virtual int		Save(CSave& save);
	virtual int		Restore(CRestore& restore);
	static	TYPEDESCRIPTION m_SaveData[];
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	int m_iStartDist;
	int m_iEndDist;
	int m_iLevelChange;
	float m_iFadeIn;
	float m_iFadeOut;
};

TYPEDESCRIPTION	CEnvFog::m_SaveData[] =
{
	DEFINE_FIELD(CEnvFog, m_iStartDist, FIELD_INTEGER),
	DEFINE_FIELD(CEnvFog, m_iEndDist, FIELD_INTEGER),
	DEFINE_FIELD(CEnvFog, m_iFadeIn, FIELD_INTEGER),
	DEFINE_FIELD(CEnvFog, m_iFadeOut, FIELD_INTEGER),
	DEFINE_FIELD(CEnvFog, m_iLevelChange, FIELD_INTEGER),
};
IMPLEMENT_SAVERESTORE(CEnvFog, CBaseEntity);

LINK_ENTITY_TO_CLASS(env_fog, CEnvFog);


void CEnvFog::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "startdist"))
	{
		m_iStartDist = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "enddist"))
	{
		m_iEndDist = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadein"))
	{
		m_iFadeIn = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadeout"))
	{
		m_iFadeOut = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CEnvFog::Spawn(void)
{
	pev->effects |= EF_NODRAW;

	if (pev->targetname == 0)
		pev->spawnflags |= SF_FOG_ACTIVE;

	if (pev->spawnflags & SF_FOG_ACTIVE)
	{
		SetThink(&CEnvFog::StartThink);
		pev->nextthink = gpGlobals->time + 0.1;
	}

	// things get messed up if we try to draw fog with a startdist
	// or an enddist of 0, so we don't allow it.
	if (m_iStartDist == 0) m_iStartDist = 1;
	if (m_iEndDist == 0) m_iEndDist = 1;
}

void CEnvFog::Precache(void)
{
	if (pev->spawnflags & SF_FOG_ACTIVE)
	{
		SetThink(&CEnvFog::ResumeThink);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CEnvFog::ResumeThink(void)
{
	//ALERT(at_console, "Fog resume %f\n", gpGlobals->time);

	SendData(pev->rendercolor, 0, m_iStartDist, m_iEndDist);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CEnvFog::StartThink(void)
{
	TurnOn();
}

void CEnvFog::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	//ALERT(at_console, "Fog use\n");

	if (useType == USE_ON)
		TurnOn();

	else if (useType == USE_OFF)
		TurnOff();

	else if (ShouldToggle(useType, true))
	{
		if (pev->spawnflags & SF_FOG_ACTIVE)
			TurnOff();
		else
			TurnOn();
	}
}

void CEnvFog::TurnOn(void)
{
	//ALERT(at_console, "Fog turn on %f\n", gpGlobals->time);

	pev->spawnflags |= SF_FOG_ACTIVE;

	if (m_iFadeIn)
		SendData(pev->rendercolor, m_iFadeIn, m_iStartDist, m_iEndDist);
	else
		SendData(pev->rendercolor, 0, m_iStartDist, m_iEndDist);

	pev->nextthink = 0;
	SetThink(NULL);
}

void CEnvFog::TurnOff(void)
{
	//ALERT(at_console, "Fog turnoff\n");

	pev->spawnflags &= ~SF_FOG_ACTIVE;

	if (m_iFadeOut)
		SendData(pev->rendercolor, m_iFadeOut, m_iStartDist, 30000);
	else
		SendData(pev->rendercolor, 0, m_iStartDist, 30000);

	pev->nextthink = 0;
	SetThink(NULL);
}

void CEnvFog::SendData(Vector col, int iFadeTime, int iStartDist, int iEndDist)
{
	//ALERT(at_console, "Fog send (%d %d %d), %d - %d\n", col.x, col.y, col.z, iStartDist, iEndDist);
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(i);

		if (pPlayer)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgSetFog, NULL, pPlayer->pev);
			WRITE_BYTE(col.x);
			WRITE_BYTE(col.y);
			WRITE_BYTE(col.z);
			WRITE_BYTE(iFadeTime);
			WRITE_SHORT(iStartDist);
			WRITE_SHORT(iEndDist);

			if (pev->spawnflags & SF_FOG_SKY)
				WRITE_BYTE(1);
			else
				WRITE_BYTE(0);

			MESSAGE_END();
		}
	}
}