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
/*

===== water.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "doors.h"

class CBaseWater : public CBaseDoor
{
public:
	void Spawn(void);
	virtual void KeyValue(KeyValueData* pkvd);
	virtual void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);


	virtual int	ObjectCaps(void)
	{
		if (pev->spawnflags & SF_ITEM_USE_ONLY)
			return (CBaseDoor::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE;
		else
			return (CBaseDoor::ObjectCaps() & ~FCAP_ACROSS_TRANSITION);
	};

	virtual void SetToggleState(int state);
};

LINK_ENTITY_TO_CLASS(func_sourcewater, CBaseWater);

void CBaseWater::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "XSpeed"))
	{
		pev->iuser1 = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "YSpeed"))
	{
		pev->iuser2 = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "memer"))
	{
		pev->iuser3 = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "FogDensity"))
	{
		pev->iuser4 = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDoor::KeyValue(pkvd);
}

void CBaseWater::Spawn()
{
	Precache();
	SetMovedir(pev);

	//pev->iuser3 = 1;

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_PUSH;

	UTIL_SetOrigin(pev, pev->origin);

	SET_MODEL(ENT(pev), STRING(pev->model));

	if (pev->speed == 0)
		pev->speed = 100;

	m_vecPosition1 = pev->origin;

	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2 = m_vecPosition1 + (pev->movedir * (fabs(pev->movedir.x * (pev->size.x - 2)) + fabs(pev->movedir.y * (pev->size.y - 2)) + fabs(pev->movedir.z * (pev->size.z - 2)) - m_flLip));
	ASSERTSZ(m_vecPosition1 != m_vecPosition2, "door start/end positions are equal");
	if (FBitSet(pev->spawnflags, SF_DOOR_START_OPEN))
	{	// swap pos1 and pos2, put door at pos2
		UTIL_SetOrigin(pev, m_vecPosition2);
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = pev->origin;
	}

	m_toggle_state = TS_AT_BOTTOM;
}

void CBaseWater::SetToggleState(int state)
{
	if (state == TS_AT_TOP)
		UTIL_SetOrigin(pev, m_vecPosition2);
	else
		UTIL_SetOrigin(pev, m_vecPosition1);
}

void CBaseWater::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_hActivator = pActivator;
	// if not ready to be used, ignore "use" command.
	if (m_toggle_state == TS_AT_BOTTOM || FBitSet(pev->spawnflags, SF_DOOR_NO_AUTO_RETURN) && m_toggle_state == TS_AT_TOP)
		CBaseDoor::DoorActivate();
}