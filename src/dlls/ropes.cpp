/*
AR Software (c) 2004-2007. ArrangeMent, S2P Physics, Spirit Of Half-Life and their
logos are the property of their respective owners.

Edits by: Magic_Nipples, 2019.
*/
//magic nipples - ropes

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"

extern int gmsgAddRope;

class CPys_Rope : public CPointEntity
{
public:

	void Spawn(void);
	void Precache(void);

	void EXPORT Think(void);

	BOOL m_fRopeActive;
};

LINK_ENTITY_TO_CLASS(env_rope, CPys_Rope);

#define SF_ROPE_STARTON 1

void CPys_Rope::Spawn(void)
{
	ALERT(at_console, "Rope Spawned!\n");

	Precache();

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin(pev, pev->origin);
	SET_MODEL(edict(), "sprites/null.spr");

	//SetThink(Think);
	pev->nextthink = gpGlobals->time + 0.2;

	//if (pev->spawnflags & SF_ROPE_STARTON)
		m_fRopeActive = false;
	//else
	//	m_fRopeActive = true;
}

void CPys_Rope::Precache(void)
{
	PRECACHE_MODEL("sprites/null.spr");
}


void CPys_Rope::Think(void)
{
	if (!m_fRopeActive)
	{
		CBaseEntity* pTarget = UTIL_FindEntityByTargetname(NULL, STRING(pev->target));

		if (pTarget != NULL)
		{
			MESSAGE_BEGIN(MSG_ALL, gmsgAddRope, NULL);

			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);

			WRITE_COORD(pTarget->pev->origin.x);
			WRITE_COORD(pTarget->pev->origin.y);
			WRITE_COORD(pTarget->pev->origin.z);

			WRITE_STRING(STRING(pev->message));

			MESSAGE_END();
		}

		m_fRopeActive = true;
	}

	pev->nextthink = gpGlobals->time + 0.2;
}
