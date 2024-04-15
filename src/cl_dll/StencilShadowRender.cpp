//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "windows.h"
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "studio_util.h"
#include "r_studioint.h"

#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"

#include "pmtrace.h"
#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"
#include "pm_defs.h"
#include "bspfile.h"
#include "gl/glext.h"


extern CGameStudioModelRenderer g_StudioRenderer;
extern engine_studio_api_t IEngineStudio;

#define BIT( n )		(1<<( n )) //magic nipples - using bspfile for SURF defs need BIT defined as well

// Globals used by shadow rendering
model_t*	g_pWorld;
int			g_visFrame = 0;
int			g_frameCount = 0;

/*
====================
SetupBuffer
====================
*/
void SetupBuffer(void)
{
	glClear(GL_STENCIL_BUFFER_BIT); // Might be using hacky DLL
}

/*
====================
Mod_PointInLeaf
====================
*/
mleaf_t* Mod_PointInLeaf(vec3_t p, model_t* model) // quake's func
{
	mnode_t* node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t*)node;
		mplane_t* plane = node->plane;
		float d = DotProduct(p, plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}
	return NULL;	// never reached
}

/*
====================
RecursiveDrawWorld
====================
*/
void RecursiveDrawWorld(mnode_t* node)
{
	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->visframe != g_visFrame)
		return;

	if (node->contents < 0)
		return;		// faces already marked by engine

	// recurse down the children, Order doesn't matter
	RecursiveDrawWorld(node->children[0]);
	RecursiveDrawWorld(node->children[1]);

	// draw stuff
	int c = node->numsurfaces;
	if (c)
	{
		msurface_t* surf = g_pWorld->surfaces + node->firstsurface;

		for (; c; c--, surf++)
		{
			if (surf->visframe != g_frameCount)
				continue;

			if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB | SURF_UNDERWATER | SURF_REFLECT))
				continue;

			glpoly_t* p = surf->polys;
			float* v = p->verts[0];

			glBegin(GL_POLYGON);
			for (int i = 0; i < p->numverts; i++, v += VERTEXSIZE)
			{
				glTexCoord2f(v[3], v[4]);
				glVertex3fv(v);
			}
			glEnd();
		}
	}
}

void RenderShadow(void)
{
	if (IEngineStudio.IsHardware() != 1)
		return;
	
	glPushAttrib(GL_TEXTURE_BIT);

	// BUzer: workaround half-life's bug, when multitexturing left enabled after
	// rendering brush entities
	g_StudioRenderer.glActiveTexture(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_2D);
	g_StudioRenderer.glActiveTexture(GL_TEXTURE0_ARB);

	glDepthMask(GL_FALSE);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	if (gHUD.m_iShadowColor[0] || gHUD.m_iShadowColor[1] || gHUD.m_iShadowColor[2] || gHUD.m_fShadowAlpha)
		glColor4f(gHUD.m_iShadowColor[0], gHUD.m_iShadowColor[1], gHUD.m_iShadowColor[2], gHUD.m_fShadowAlpha);
	else
		glColor4f(GL_ZERO, GL_ZERO, GL_ZERO, g_StudioRenderer.m_pCvarShadowAlpha->value); //glColor4f(GL_ZERO, GL_ZERO, GL_ZERO, 0.5);

	glDepthFunc(GL_EQUAL);
	glStencilFunc(GL_NOTEQUAL, 0, ~0);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glEnable(GL_STENCIL_TEST);

	// get current visframe number
	g_pWorld = gEngfuncs.GetEntityByIndex(0)->model;
	mleaf_t* leaf = Mod_PointInLeaf(g_StudioRenderer.m_vRenderOrigin, g_pWorld);
	g_visFrame = leaf->visframe;

	// get current frame number
	g_frameCount = g_StudioRenderer.m_nFrameCount;

	// draw world
	RecursiveDrawWorld(g_pWorld->nodes);

	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_STENCIL_TEST);
	glDepthFunc(GL_LEQUAL);

	glPopAttrib();
}