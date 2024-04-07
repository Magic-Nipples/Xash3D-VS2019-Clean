/***
*
*	Copyright (c) 2024, Magic Nipples.
*
*	Use and modification of this code is allowed as long
*	as credit is provided! Enjoy!
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"

extern int gmsgShadowInfo;

class CInfoShadow : public CBaseEntity
{
public:
	void	KeyValue(KeyValueData* pkvd);
	void	Spawn(void);
	void	EXPORT Think(void);
	void	Activate(void);

	int		ObjectCaps(void)
	{
		return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION);
	}

	float m_flAngX;
	float m_flAngY;
	float m_flAngZ;
	float m_flAlpha;

	virtual int		Save(CSave& save);
	virtual int		Restore(CRestore& restore);
	static	TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS(info_shadow, CInfoShadow);

TYPEDESCRIPTION	CInfoShadow::m_SaveData[] =
{
	DEFINE_FIELD(CInfoShadow, m_flAngX, FIELD_FLOAT),
	DEFINE_FIELD(CInfoShadow, m_flAngY, FIELD_FLOAT),
	DEFINE_FIELD(CInfoShadow, m_flAngZ, FIELD_FLOAT),
	DEFINE_FIELD(CInfoShadow, m_flAlpha, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CInfoShadow, CBaseEntity);


void CInfoShadow::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_flAngX"))
	{
		m_flAngX = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flAngY"))
	{
		m_flAngY = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flAngZ"))
	{
		m_flAngZ = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flAlpha"))
	{
		m_flAlpha = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseEntity::KeyValue(pkvd);
	}
}

void CInfoShadow::Spawn()
{
	//ALERT(at_console, "%0.2f %0.2f %0.2f %0.2f\n", m_flAngX, m_flAngY, m_flAngZ, m_flAlpha);
	//ALERT(at_console, "%0.2f %0.2f %0.2f\n", pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z);
	Activate();
}

void CInfoShadow::Activate(void)
{
	//ALERT(at_console, "ACTIVATE\n");
	pev->nextthink = gpGlobals->time + 0.1;
}

void CInfoShadow::Think(void)
{
	MESSAGE_BEGIN(MSG_ALL, gmsgShadowInfo, NULL);
	WRITE_COORD(m_flAngX);
	WRITE_COORD(m_flAngY);
	WRITE_COORD(m_flAngZ);
	WRITE_COORD(m_flAlpha);

	WRITE_COORD(pev->rendercolor.x);
	WRITE_COORD(pev->rendercolor.y);
	WRITE_COORD(pev->rendercolor.z);
	MESSAGE_END();

	pev->nextthink = -1;
}