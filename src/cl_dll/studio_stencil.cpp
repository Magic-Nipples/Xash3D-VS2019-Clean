//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// studio_model.cpp
// routines for setting up to draw 3DStudio models

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


// Global engine <-> studio model rendering code interface
extern engine_studio_api_t IEngineStudio;
extern CGameStudioModelRenderer g_StudioRenderer;

#define Matrix3x4_Copy( out, in )	memcpy( out, in, sizeof( float[3][4] ))

/*
====================
StudioSetupShadows

====================
*/
void CStudioModelRenderer::StudioSetupShadows( void )
{
	if( IEngineStudio.IsHardware() != 1 )
		return;	
	
	// Determine the shading angle
	vec3_t shadeVector;

	if(gHUD.m_fShadowAngle[0] != 0 || gHUD.m_fShadowAngle[1] != 0 || gHUD.m_fShadowAngle[2] != 0)
	{
		shadeVector[0] = gHUD.m_fShadowAngle[0];
		shadeVector[1] = gHUD.m_fShadowAngle[1];
		shadeVector[2] = gHUD.m_fShadowAngle[2];
	}
	else
	{
		shadeVector[0] = m_pSkylightDirX->value;//0.3;
		shadeVector[1] = m_pSkylightDirY->value;//0.5;
		shadeVector[2] = m_pSkylightDirZ->value;//1;
	}

	VectorInverse(shadeVector);
	shadeVector.Normalize();

	m_vShadowLightVector = shadeVector;
}

/*
====================
StudioSetupModelSVD

====================
*/
void CStudioModelRenderer::StudioSetupModelSVD ( int bodypart )
{
	if (bodypart > m_pSVDHeader->numbodyparts)
		bodypart = 0;

	svdbodypart_t* pbodypart = (svdbodypart_t *)((byte *)m_pSVDHeader + m_pSVDHeader->bodypartindex) + bodypart;

	int index = m_pCurrentEntity->curstate.body / pbodypart->base;
	index = index % pbodypart->numsubmodels;

	m_pSVDSubModel = (svdsubmodel_t *)((byte *)m_pSVDHeader + pbodypart->submodelindex) + index;
}


/*
====================
StudioShouldDrawShadow

====================
*/
bool CStudioModelRenderer::StudioShouldDrawShadow ( void )
{
	if(m_pCvarDrawStencilShadows->value < 1 )
		return false;

	if( IEngineStudio.IsHardware() != 1 )
		return false;

	if( !m_pRenderModel->visdata )
		return false;

	//if(m_pCurrentEntity->curstate.renderfx == kRenderFxNoShadow)
	//	return false;

	// Fucking butt-ugly hack to make the shadows less annoying
	//pmtrace_t tr;
	//gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	//gEngfuncs.pEventAPI->EV_PlayerTrace( m_vRenderOrigin, m_pCurrentEntity->origin+Vector(0, 0, 1), PM_WORLD_ONLY, -1, &tr);

	//if(tr.fraction != 1.0)
	//	return false;

	return true;
}

/*
====================
StudioDrawShadow

====================
*/
void CStudioModelRenderer::StudioDrawShadow ( void )
{
	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

	// Disabable these to avoid slowdown bug
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(GL_TEXTURE2);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(GL_TEXTURE3);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer( 3, GL_FLOAT, sizeof(vec3_t), m_vertexTransform );
	glEnableClientState(GL_VERTEX_ARRAY);

	// Set SVD header
	m_pSVDHeader = (svdheader_t*)m_pRenderModel->visdata;

	glDepthMask(GL_FALSE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // disable writes to color buffer

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 0, ~0);

	if(m_bTwoSideSupported)
	{
		glDisable(GL_CULL_FACE);
		glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	}

	for (int i = 0; i < m_pStudioHeader->numbodyparts; i++)
	{
		StudioSetupModelSVD( i );
		StudioDrawShadowVolume( );
	}

	glDepthMask(GL_TRUE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_STENCIL_TEST);

	if(m_bTwoSideSupported)
	{
		glEnable(GL_CULL_FACE);
		glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	}

	glDisableClientState(GL_VERTEX_ARRAY);

	glPopClientAttrib();
}

/*
====================
StudioDrawShadowVolume

====================
*/
void CStudioModelRenderer::R_StudioComputeSkinMatrix(mstudioboneweight_t* boneweights, float result[3][4])
{
	float	flWeight0, flWeight1, flWeight2, flWeight3;
	int	i, numbones = 0;
	float	flTotal;
	float	world[MAXSTUDIOBONES][3][4];

	// NOTE: extended boneinfo goes immediately after bones
	mstudioboneinfo_t* boneinfo = (mstudioboneinfo_t*)((byte*)m_pStudioHeader + m_pStudioHeader->boneindex + m_pStudioHeader->numbones * sizeof(mstudiobone_t));

	for (int i = 0; i < m_pStudioHeader->numbones; i++)
		ConcatTransforms((*m_pbonetransform)[i], boneinfo[i].poseToBone, world[i]);


	if (g_StudioRenderer.m_pCvarShadowWeight->value) //for performance don't calculate boneweights just for shadows
	{
		Matrix3x4_Copy(result, world[boneweights->bone[0]]);
		return;
	}

	for (i = 0; i < 4; i++)
	{
		if (boneweights->bone[i] != -1)
			numbones++;
	}

	if (numbones == 4)
	{
		vec4_t* boneMat0 = (vec4_t*)world[boneweights->bone[0]];
		vec4_t* boneMat1 = (vec4_t*)world[boneweights->bone[1]];
		vec4_t* boneMat2 = (vec4_t*)world[boneweights->bone[2]];
		vec4_t* boneMat3 = (vec4_t*)world[boneweights->bone[3]];
		flWeight0 = boneweights->weight[0] / 255.0f;
		flWeight1 = boneweights->weight[1] / 255.0f;
		flWeight2 = boneweights->weight[2] / 255.0f;
		flWeight3 = boneweights->weight[3] / 255.0f;
		flTotal = flWeight0 + flWeight1 + flWeight2 + flWeight3;

		if (flTotal < 1.0f) flWeight0 += 1.0f - flTotal;	// compensate rounding error

		result[0][0] = boneMat0[0][0] * flWeight0 + boneMat1[0][0] * flWeight1 + boneMat2[0][0] * flWeight2 + boneMat3[0][0] * flWeight3;
		result[0][1] = boneMat0[0][1] * flWeight0 + boneMat1[0][1] * flWeight1 + boneMat2[0][1] * flWeight2 + boneMat3[0][1] * flWeight3;
		result[0][2] = boneMat0[0][2] * flWeight0 + boneMat1[0][2] * flWeight1 + boneMat2[0][2] * flWeight2 + boneMat3[0][2] * flWeight3;
		result[0][3] = boneMat0[0][3] * flWeight0 + boneMat1[0][3] * flWeight1 + boneMat2[0][3] * flWeight2 + boneMat3[0][3] * flWeight3;
		result[1][0] = boneMat0[1][0] * flWeight0 + boneMat1[1][0] * flWeight1 + boneMat2[1][0] * flWeight2 + boneMat3[1][0] * flWeight3;
		result[1][1] = boneMat0[1][1] * flWeight0 + boneMat1[1][1] * flWeight1 + boneMat2[1][1] * flWeight2 + boneMat3[1][1] * flWeight3;
		result[1][2] = boneMat0[1][2] * flWeight0 + boneMat1[1][2] * flWeight1 + boneMat2[1][2] * flWeight2 + boneMat3[1][2] * flWeight3;
		result[1][3] = boneMat0[1][3] * flWeight0 + boneMat1[1][3] * flWeight1 + boneMat2[1][3] * flWeight2 + boneMat3[1][3] * flWeight3;
		result[2][0] = boneMat0[2][0] * flWeight0 + boneMat1[2][0] * flWeight1 + boneMat2[2][0] * flWeight2 + boneMat3[2][0] * flWeight3;
		result[2][1] = boneMat0[2][1] * flWeight0 + boneMat1[2][1] * flWeight1 + boneMat2[2][1] * flWeight2 + boneMat3[2][1] * flWeight3;
		result[2][2] = boneMat0[2][2] * flWeight0 + boneMat1[2][2] * flWeight1 + boneMat2[2][2] * flWeight2 + boneMat3[2][2] * flWeight3;
		result[2][3] = boneMat0[2][3] * flWeight0 + boneMat1[2][3] * flWeight1 + boneMat2[2][3] * flWeight2 + boneMat3[2][3] * flWeight3;
	}
	else if (numbones == 3)
	{
		vec4_t* boneMat0 = (vec4_t*)world[boneweights->bone[0]];
		vec4_t* boneMat1 = (vec4_t*)world[boneweights->bone[1]];
		vec4_t* boneMat2 = (vec4_t*)world[boneweights->bone[2]];
		flWeight0 = boneweights->weight[0] / 255.0f;
		flWeight1 = boneweights->weight[1] / 255.0f;
		flWeight2 = boneweights->weight[2] / 255.0f;
		flTotal = flWeight0 + flWeight1 + flWeight2;

		if (flTotal < 1.0f) flWeight0 += 1.0f - flTotal;	// compensate rounding error

		result[0][0] = boneMat0[0][0] * flWeight0 + boneMat1[0][0] * flWeight1 + boneMat2[0][0] * flWeight2;
		result[0][1] = boneMat0[0][1] * flWeight0 + boneMat1[0][1] * flWeight1 + boneMat2[0][1] * flWeight2;
		result[0][2] = boneMat0[0][2] * flWeight0 + boneMat1[0][2] * flWeight1 + boneMat2[0][2] * flWeight2;
		result[0][3] = boneMat0[0][3] * flWeight0 + boneMat1[0][3] * flWeight1 + boneMat2[0][3] * flWeight2;
		result[1][0] = boneMat0[1][0] * flWeight0 + boneMat1[1][0] * flWeight1 + boneMat2[1][0] * flWeight2;
		result[1][1] = boneMat0[1][1] * flWeight0 + boneMat1[1][1] * flWeight1 + boneMat2[1][1] * flWeight2;
		result[1][2] = boneMat0[1][2] * flWeight0 + boneMat1[1][2] * flWeight1 + boneMat2[1][2] * flWeight2;
		result[1][3] = boneMat0[1][3] * flWeight0 + boneMat1[1][3] * flWeight1 + boneMat2[1][3] * flWeight2;
		result[2][0] = boneMat0[2][0] * flWeight0 + boneMat1[2][0] * flWeight1 + boneMat2[2][0] * flWeight2;
		result[2][1] = boneMat0[2][1] * flWeight0 + boneMat1[2][1] * flWeight1 + boneMat2[2][1] * flWeight2;
		result[2][2] = boneMat0[2][2] * flWeight0 + boneMat1[2][2] * flWeight1 + boneMat2[2][2] * flWeight2;
		result[2][3] = boneMat0[2][3] * flWeight0 + boneMat1[2][3] * flWeight1 + boneMat2[2][3] * flWeight2;
	}
	else if (numbones == 2)
	{
		vec4_t* boneMat0 = (vec4_t*)world[boneweights->bone[0]];
		vec4_t* boneMat1 = (vec4_t*)world[boneweights->bone[1]];
		flWeight0 = boneweights->weight[0] / 255.0f;
		flWeight1 = boneweights->weight[1] / 255.0f;
		flTotal = flWeight0 + flWeight1;

		if (flTotal < 1.0f) flWeight0 += 1.0f - flTotal;	// compensate rounding error

		result[0][0] = boneMat0[0][0] * flWeight0 + boneMat1[0][0] * flWeight1;
		result[0][1] = boneMat0[0][1] * flWeight0 + boneMat1[0][1] * flWeight1;
		result[0][2] = boneMat0[0][2] * flWeight0 + boneMat1[0][2] * flWeight1;
		result[0][3] = boneMat0[0][3] * flWeight0 + boneMat1[0][3] * flWeight1;
		result[1][0] = boneMat0[1][0] * flWeight0 + boneMat1[1][0] * flWeight1;
		result[1][1] = boneMat0[1][1] * flWeight0 + boneMat1[1][1] * flWeight1;
		result[1][2] = boneMat0[1][2] * flWeight0 + boneMat1[1][2] * flWeight1;
		result[1][3] = boneMat0[1][3] * flWeight0 + boneMat1[1][3] * flWeight1;
		result[2][0] = boneMat0[2][0] * flWeight0 + boneMat1[2][0] * flWeight1;
		result[2][1] = boneMat0[2][1] * flWeight0 + boneMat1[2][1] * flWeight1;
		result[2][2] = boneMat0[2][2] * flWeight0 + boneMat1[2][2] * flWeight1;
		result[2][3] = boneMat0[2][3] * flWeight0 + boneMat1[2][3] * flWeight1;
	}
	else
	{
		Matrix3x4_Copy(result, world[boneweights->bone[0]]);
	}
}

void CStudioModelRenderer::StudioDrawShadowVolume( void )
{
	float plane[4];
	vec3_t lightdir;
	vec3_t *pv1, *pv2, *pv3;

	if (!m_pSVDSubModel->numfaces)
		return;

	vec3_t *psvdverts = (vec3_t *)((byte *)m_pSVDHeader + m_pSVDSubModel->vertexindex);
	byte *pvertbone = ((byte *)m_pSVDHeader + m_pSVDSubModel->vertinfoindex);

	// Extrusion distance
	float extrudeDistance = m_pCvarShadowVolumeExtrudeDistance->value;

	// Calculate vertex coords
	if (m_pStudioHeader->flags & STUDIO_HAS_BONEWEIGHTS)
	{
		float	skinMat[3][4];
		for (int i = 0, j = 0; i < m_pSVDSubModel->numverts; i++, j += 2)
		{
			R_StudioComputeSkinMatrix(&m_pSVDSubModel->pvertweight[i], skinMat);

			VectorTransform(psvdverts[i], skinMat, m_vertexTransform[j]);
			VectorMA(m_vertexTransform[j], extrudeDistance, m_vShadowLightVector, m_vertexTransform[j + 1]);
		}
	}
	else
	{
		for (int i = 0, j = 0; i < m_pSVDSubModel->numverts; i++, j += 2)
		{
			VectorTransform(psvdverts[i], (*m_pbonetransform)[pvertbone[i]], m_vertexTransform[j]);
			VectorMA(m_vertexTransform[j], extrudeDistance, m_vShadowLightVector, m_vertexTransform[j+1]);
		}
	}

	// Process the faces
	int numIndexes = 0;
	svdface_t* pfaces = (svdface_t*)((byte *)m_pSVDHeader + m_pSVDSubModel->faceindex);

	// For a light vector
	for (int i = 0; i < m_pSVDSubModel->numfaces; i++)
	{
		pv1 = &m_vertexTransform[pfaces[i].vertex0];
		pv2 = &m_vertexTransform[pfaces[i].vertex1];
		pv3 = &m_vertexTransform[pfaces[i].vertex2];

		// Calculate normal of the face
		plane[0] = pv1->y * (pv2->z - pv3->z) + pv2->y * (pv3->z - pv1->z) + pv3->y * (pv1->z - pv2->z);
		plane[1] = pv1->z * (pv2->x - pv3->x) + pv2->z * (pv3->x - pv1->x) + pv3->z * (pv1->x - pv2->x);
		plane[2] = pv1->x * (pv2->y - pv3->y) + pv2->x * (pv3->y - pv1->y) + pv3->x * (pv1->y - pv2->y);

		m_trianglesFacingLight[i] = DotProduct(plane, m_vShadowLightVector) > 0;
		if (m_trianglesFacingLight[i])
		{
			m_shadowVolumeIndexes[numIndexes] = pfaces[i].vertex0;
			m_shadowVolumeIndexes[numIndexes + 1] = pfaces[i].vertex2;
			m_shadowVolumeIndexes[numIndexes + 2] = pfaces[i].vertex1;

			m_shadowVolumeIndexes[numIndexes + 3] = pfaces[i].vertex0 + 1;
			m_shadowVolumeIndexes[numIndexes + 4] = pfaces[i].vertex1 + 1;
			m_shadowVolumeIndexes[numIndexes + 5] = pfaces[i].vertex2 + 1;

			numIndexes += 6;
		}
	}

	// Process the edges
	svdedge_t* pedges = (svdedge_t*)((byte *)m_pSVDHeader + m_pSVDSubModel->edgeindex);
	for (int i = 0; i < m_pSVDSubModel->numedges; i++)
	{
		if (m_trianglesFacingLight[pedges[i].face0])
		{
			if ((pedges[i].face1 != -1) && m_trianglesFacingLight[pedges[i].face1])
				continue;

			m_shadowVolumeIndexes[numIndexes] = pedges[i].vertex0;
			m_shadowVolumeIndexes[numIndexes + 1] = pedges[i].vertex1;
		}
		else
		{
			if ((pedges[i].face1 == -1) || !m_trianglesFacingLight[pedges[i].face1])
				continue;

			m_shadowVolumeIndexes[numIndexes] = pedges[i].vertex1;
			m_shadowVolumeIndexes[numIndexes + 1] = pedges[i].vertex0;
		}

		m_shadowVolumeIndexes[numIndexes + 2] = m_shadowVolumeIndexes[numIndexes] + 1;
		m_shadowVolumeIndexes[numIndexes + 3] = m_shadowVolumeIndexes[numIndexes + 2];
		m_shadowVolumeIndexes[numIndexes + 4] = m_shadowVolumeIndexes[numIndexes + 1];
		m_shadowVolumeIndexes[numIndexes + 5] = m_shadowVolumeIndexes[numIndexes + 1] + 1;
		numIndexes += 6;
	}

	if(m_bTwoSideSupported)
	{
		glActiveStencilFaceEXT(GL_BACK);
		glStencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);
		glStencilMask(~0);

		glActiveStencilFaceEXT(GL_FRONT);
		glStencilOp(GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
		glStencilMask(~0);

		glDrawElements(GL_TRIANGLES, numIndexes, GL_UNSIGNED_SHORT, m_shadowVolumeIndexes);
	}
	else
	{
		// draw back faces incrementing stencil values when z fails
		glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
		glCullFace(GL_BACK);
		glDrawElements(GL_TRIANGLES, numIndexes, GL_UNSIGNED_SHORT, m_shadowVolumeIndexes);

		// draw front faces decrementing stencil values when z fails
		glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
		glCullFace(GL_FRONT);
		glDrawElements(GL_TRIANGLES, numIndexes, GL_UNSIGNED_SHORT, m_shadowVolumeIndexes);
	}
}
