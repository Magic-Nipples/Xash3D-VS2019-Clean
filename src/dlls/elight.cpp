//===============================================
// Entity light
// Code by Andrew "Lucas" Highlander
// Additional help by Andrew "Confused" Hamilton
//===============================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "customentity.h"
#include "effects.h"
#include "weapons.h"
#include "decals.h"
#include "func_break.h"
#include "shake.h"

extern int gmsgAddELight;

#define SF_LIGHT_STARTON 1
#define SF_LIGHT_FLICKER 2

class CEnvELight : public CPointEntity
{
public:
	//void KeyValue( KeyValueData *pkvd );
	void Spawn(void);
	void SendData(void);
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	void EXPORT	LightFrame(void);

	virtual int	Save(CSave& save);
	virtual int	Restore(CRestore& restore);
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fLightActive;
	BOOL	m_fDeactivatedByPVS;

	BOOL	m_fParentedElight;
	int		m_iParentEntindex;
	float	m_fFlickerTime;
	int		beans;
	CBaseEntity* m_pGoalEnt;
	//int		m_iPattern;
	//int		m_strLightTarget;
};
LINK_ENTITY_TO_CLASS(env_elight, CEnvELight);

TYPEDESCRIPTION	CEnvELight::m_SaveData[] =
{
	DEFINE_FIELD(CEnvELight, m_fLightActive, FIELD_BOOLEAN),
	DEFINE_FIELD(CEnvELight, m_fParentedElight, FIELD_BOOLEAN),
	DEFINE_FIELD(CEnvELight, m_fDeactivatedByPVS, FIELD_BOOLEAN),
	DEFINE_FIELD(CEnvELight, m_pGoalEnt, FIELD_CLASSPTR),

	//DEFINE_FIELD( CEnvELight, m_iPattern, FIELD_INTEGER ),
	//DEFINE_FIELD( CEnvELight, m_strLightTarget, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE(CEnvELight, CPointEntity);

/*
void CEnvELight::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "poop"))
	{
		m_strLightTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	//else if (FStrEq(pkvd->szKeyName, "pattern"))
	//{
	//	m_iPattern = atof(pkvd->szValue);
	//	pkvd->fHandled = TRUE;
//	}
	else
		CPointEntity::KeyValue( pkvd );
}*/

void CEnvELight::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	pev->flags |= FL_ALWAYSTHINK;

	UTIL_SetSize(pev, Vector(-128, -128, -128), Vector(128, 128, 128));

	if (FStringNull(pev->targetname) || pev->spawnflags & SF_LIGHT_STARTON)
		m_fLightActive = TRUE;
	else
		m_fLightActive = FALSE;

	m_fFlickerTime = -1;
	beans = 0;

	if (m_fLightActive)
	{
		SetThink(&CEnvELight::LightFrame);
		pev->nextthink = gpGlobals->time + 0.01; //changed from 0.1
	}

	m_pGoalEnt = UTIL_FindEntityByTargetname(NULL, STRING(pev->target));
}

void CEnvELight::SendData(void)
{
	edict_t* pTarget = NULL;

	if (pev->target != NULL)
	{
		pTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));
		m_fParentedElight = TRUE;
	}

	if (FNullEnt(pTarget) && m_fParentedElight)
	{
		UTIL_Remove(this);
		return;
	}

	if ((pev->spawnflags & SF_LIGHT_FLICKER) && (m_fFlickerTime < gpGlobals->time) && (m_fLightActive))
	{
		if (RANDOM_LONG(0, 3) == 0)
			beans = 0;
		else
			beans = 1;

		m_fFlickerTime = gpGlobals->time + 0.1;
	}

	if (!m_fLightActive)
		beans = 0;

	MESSAGE_BEGIN(MSG_ALL, gmsgAddELight, NULL);

	if (FNullEnt(pTarget))
		WRITE_SHORT(entindex());
	else
		WRITE_SHORT(ENTINDEX(pTarget) + 0x1000 * pev->impulse);

	if (!m_fDeactivatedByPVS)
	{
		if (pev->spawnflags & SF_LIGHT_FLICKER)
		{
			if (beans == 1)
			{
				WRITE_BYTE(TRUE);
				SUB_UseTargets(m_pGoalEnt, USE_ON, 0);
			}
			else
			{
				WRITE_BYTE(FALSE);
				SUB_UseTargets(m_pGoalEnt, USE_OFF, 0);
			}
		}
		else
		{
			WRITE_BYTE(m_fLightActive);
		}
	}
	else
	{
		WRITE_BYTE(FALSE);
	}

	if (m_fLightActive && !m_fDeactivatedByPVS)
	{
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_COORD(pev->renderamt);
		WRITE_BYTE(pev->rendercolor.x);
		WRITE_BYTE(pev->rendercolor.y);
		WRITE_BYTE(pev->rendercolor.z);
	}
	MESSAGE_END();

	if (m_iParentEntindex == NULL && m_fParentedElight)
		m_iParentEntindex = (ENTINDEX(pTarget) + 0x1000 * pev->impulse);

	if (m_fLightActive)
	{
		pev->nextthink = pev->ltime + 0.1;
		SetThink(&CEnvELight::LightFrame);
	}
	else
	{
		SetThink(NULL);
	}
}

void CEnvELight::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (useType == USE_ON)
	{
		if (m_fLightActive)
			return;
		else
			m_fLightActive = TRUE;
	}
	else if (useType == USE_OFF)
	{
		if (!m_fLightActive)
			return;
		else
			m_fLightActive = FALSE;
	}
	else if (useType == USE_TOGGLE)
	{
		if (!m_fLightActive)
			m_fLightActive = TRUE;
		else
			m_fLightActive = FALSE;
	}

	SendData();
};
void CEnvELight::LightFrame(void)
{
	SendData();
}