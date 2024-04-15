//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

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

vec3_t		vertexdata[MaxShadowFaceCount * 5];
GLushort	indexdata[MaxShadowFaceCount * 3 * 5];

/*
====================
StudioShouldDrawShadow
====================
*/
bool CStudioModelRenderer::StudioShouldDrawShadow(void)
{
	if (m_pCvarDrawStencilShadows->value < 1)
		return false;

	if (IEngineStudio.IsHardware() != 1)
		return false;

	if (m_pCurrentEntity->curstate.effects & EF_NOSHADOW)
		return false;

	return true;
}

/*
====================
GetShadowVector
====================
*/
void CStudioModelRenderer::GetShadowVector(vec3_t& vecOut)
{
	if (IEngineStudio.IsHardware() != 1)
		return;

	// Determine the shading angle
	vec3_t shadeVector;

	if (gHUD.m_fShadowAngle[0] != 0 || gHUD.m_fShadowAngle[1] != 0 || gHUD.m_fShadowAngle[2] != 0)
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

	vecOut = shadeVector;
}

/*
====================
BuildFaces
====================
*/
void CStudioModelRenderer::BuildFaces(SubModelData& dst, mstudiomodel_t* src)
{
	int n = 0;
	// get number of triangles in all meshes
	mstudiomesh_t* pmeshes = (mstudiomesh_t*)((byte*)m_pStudioHeader + src->meshindex);
	for (int i = 0; i < src->nummesh; i++)
	{
		int j;
		short* ptricmds = (short*)((byte*)m_pStudioHeader + pmeshes[i].triindex);
		while (j = *(ptricmds++))
		{
			if (j < 0)
				j *= -1;

			n += (j - 2);
			ptricmds += 4 * j;
		}
	}

	// No faces in this submodel
	if (n == 0)
		return;

	dst.faces.reserve(n); // save data

	for (int i = 0; i < src->nummesh; i++)
	{
		short* ptricmds = (short*)((byte*)m_pStudioHeader + pmeshes[i].triindex);

		int j;
		while (j = *(ptricmds++))
		{
			if (j > 0)
			{
				// convert triangle strip
				j -= 3;

				short indices[3];
				indices[0] = ptricmds[0]; ptricmds += 4;
				indices[1] = ptricmds[0]; ptricmds += 4;
				indices[2] = ptricmds[0]; ptricmds += 4;

				// Add the face
				dst.faces.push_back(Face(indices[0], indices[1], indices[2]));

				bool reverse = false;
				for (; j > 0; j--, ptricmds += 4)
				{
					indices[0] = indices[1];
					indices[1] = indices[2];
					indices[2] = ptricmds[0];

					// Add the face
					if (!reverse)
						dst.faces.push_back(Face(indices[2], indices[1], indices[0]));
					else
						dst.faces.push_back(Face(indices[0], indices[1], indices[2]));

					// Switch the order
					reverse = !reverse;
				}
			}
			else
			{
				// convert triangle fan
				j = -j - 3;

				short indices[3];
				indices[0] = ptricmds[0]; ptricmds += 4;
				indices[1] = ptricmds[0]; ptricmds += 4;
				indices[2] = ptricmds[0]; ptricmds += 4;

				// Add the face
				dst.faces.push_back(Face(indices[0], indices[1], indices[2]));

				for (; j > 0; j--, ptricmds += 4)
				{
					indices[1] = indices[2];
					indices[2] = ptricmds[0];

					// Add the face
					dst.faces.push_back(Face(indices[0], indices[1], indices[2]));
				}
			}
		}
	}
}

/*
====================
BuildEdges
====================
*/
void CStudioModelRenderer::BuildEdges(SubModelData& dst, mstudiomodel_t* src)
{
	if (dst.faces.size() == 0) return;

	dst.edges.reserve(dst.faces.size() * 3); // this is maximum
	for (int i = 0; unsigned(i) < dst.faces.size(); i++) //unsigned to fix C4018 warning.
	{
		Face& f = dst.faces[i];
		AddEdge(dst, i, f.vertex0, f.vertex1);
		AddEdge(dst, i, f.vertex1, f.vertex2);
		AddEdge(dst, i, f.vertex2, f.vertex0);
	}

	dst.edges.resize(dst.edges.size()); // can i free unused memory by doing this?
}

/*
====================
AddEdge
====================
*/
void CStudioModelRenderer::AddEdge(SubModelData& dst, int face, int v0, int v1)
{
	// first look for face's neighbour
	for (int i = 0; unsigned(i) < dst.edges.size(); i++)
	{
		Edge& e = dst.edges[i];
		if ((e.vertex0 == v1) && (e.vertex1 == v0) && (e.face1 == -1))
		{
			e.face1 = face;
			return;
		}
	}

	// add new edge to list
	Edge e;
	e.face0 = face;
	e.face1 = -1;
	e.vertex0 = v0;
	e.vertex1 = v1;
	dst.edges.push_back(e);
}


void SpecialProcess(SubModelData& dst)
{
	int i;
	for (i = 0; unsigned(i) < dst.faces.size(); i++)
	{
		Face& f = dst.faces[i];
		f.vertex0 *= 2;
		f.vertex1 *= 2;
		f.vertex2 *= 2;
	}

	for (i = 0; unsigned(i) < dst.edges.size(); i++)
	{
		Edge& e = dst.edges[i];
		e.vertex0 *= 2;
		e.vertex1 *= 2;
	}
}

/*
====================
SetupModelExtraData
====================
*/
void CStudioModelRenderer::SetupModelExtraData(void)
{
	m_pCurretExtraData = &m_ExtraData[m_pRenderModel->name];

	if (m_pCurretExtraData->submodels.size() > 0)
		return;

	// generate extra data for this model
	gEngfuncs.Con_Printf("Generating extra data for model %s\n", m_pRenderModel->name);

	// get number of submodels
	int i, n = 0;
	mstudiobodyparts_t* bp = (mstudiobodyparts_t*)((byte*)m_pStudioHeader + m_pStudioHeader->bodypartindex);
	for (i = 0; i < m_pStudioHeader->numbodyparts; i++)
		n += bp[i].nummodels;

	if (n == 0)
	{
		gEngfuncs.Con_Printf("Error: model %s has 0 submodels\n", m_pRenderModel->name);
		return;
	}

	m_pCurretExtraData->submodels.resize(n);

	// convert strips and fans to triangles, generate adjacency info
	n = 0;
	int facecounter = 0, edgecounter = 0;
	for (i = 0; i < m_pStudioHeader->numbodyparts; i++)
	{
		mstudiomodel_t* sm = (mstudiomodel_t*)((byte*)m_pStudioHeader + bp[i].modelindex);
		for (int j = 0; j < bp[i].nummodels; j++)
		{
			if (unsigned(n) >= m_pCurretExtraData->submodels.size())
			{
				gEngfuncs.Con_Printf("Warning: in model %s submodel index %d is out of range\n", m_pRenderModel->name, n);
				return;
			}

			BuildFaces(m_pCurretExtraData->submodels[n], &sm[j]);
			BuildEdges(m_pCurretExtraData->submodels[n], &sm[j]);
			SpecialProcess(m_pCurretExtraData->submodels[n]);

			facecounter += m_pCurretExtraData->submodels[n].faces.size();
			edgecounter += m_pCurretExtraData->submodels[n].edges.size();

			n++;
		}
	}
	gEngfuncs.Con_Printf("Done (%d polys, %d edges)\n", facecounter, edgecounter);
}

/*
====================
DrawShadowsForEnt
====================
*/
void CStudioModelRenderer::DrawShadowsForEnt(void)
{
	SetupModelExtraData();

	if (!m_pCurretExtraData)
		return;

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

	glVertexPointer(3, GL_FLOAT, sizeof(vec3_t), vertexdata);
	glEnableClientState(GL_VERTEX_ARRAY);

	glDepthMask(GL_FALSE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // disable writes to color buffer

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 0, ~0);

	if (m_bTwoSideSupported)
	{
		glDisable(GL_CULL_FACE);
		glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	}

	// i expected IEngineStudio.StudioSetupModel to return valid pointers to current
	// bodypart and submodel, but looks like it doesn't. Make it myself.		
	mstudiobodyparts_t* bp = (mstudiobodyparts_t*)((byte*)m_pStudioHeader + m_pStudioHeader->bodypartindex);

	int baseindex = 0;
	for (int i = 0; i < m_pStudioHeader->numbodyparts; i++)
	{
		int index = m_pCurrentEntity->curstate.body / bp[i].base;
		index = index % bp[i].nummodels;

		mstudiomodel_t* sm = (mstudiomodel_t*)((byte*)m_pStudioHeader + bp[i].modelindex) + index;
		DrawShadowVolume(m_pCurretExtraData->submodels[index + baseindex], sm);
		baseindex += bp[i].nummodels;
	}

	glDepthMask(GL_TRUE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_STENCIL_TEST);

	if (m_bTwoSideSupported)
	{
		glEnable(GL_CULL_FACE);
		glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glPopClientAttrib();
}

/*
====================
boneweight calculation
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

/*
====================
DrawShadowVolume
====================
*/
void CStudioModelRenderer::DrawShadowVolume(SubModelData& data, mstudiomodel_t* model)
{
	int i, j;

	if ((data.faces.size() == 0) || (data.faces.size() > MaxShadowFaceCount))
		return;

	GetShadowVector(m_ShadowDir);

	// Extrusion distance
	float extrudeDistance = m_pCvarShadowVolumeExtrudeDistance->value;

	// get pointer to untransformed vertices
	vec3_t* pstudioverts = (vec3_t*)((byte*)m_pStudioHeader + model->vertindex);

	// get pointer to bone index for each vertex
	byte* pvertbone = ((byte*)m_pStudioHeader + model->vertinfoindex);

	if (m_pStudioHeader->flags & STUDIO_HAS_BONEWEIGHTS)
	{
		mstudioboneweight_t* pvertweight = (mstudioboneweight_t*)((byte*)m_pStudioHeader + model->blendvertinfoindex);

		for (i = 0, j = 0; i < model->numverts; i++, j += 2)
		{
			float	skinMat[3][4];
			R_StudioComputeSkinMatrix(&pvertweight[i], skinMat);

			VectorTransform(pstudioverts[i], skinMat, vertexdata[j]);
			VectorMA(vertexdata[j], extrudeDistance, m_ShadowDir, vertexdata[j + 1]);
		}
	}
	else
	{
		for (i = 0, j = 0; i < model->numverts; i++, j += 2)
		{
			VectorTransform(pstudioverts[i], (*m_pbonetransform)[pvertbone[i]], vertexdata[j]);
			VectorMA(vertexdata[j], extrudeDistance, m_ShadowDir, vertexdata[j + 1]);
		}
	}

	// Process the faces
	int numIndexes = 0;
	std::vector<Face>::iterator f;

	for (f = data.faces.begin(), i = 0; f < data.faces.end(); ++f, ++i)
	{
		vec3_t v1, v2, norm;
		VectorSubtract(vertexdata[f->vertex1], vertexdata[f->vertex0], v1);
		VectorSubtract(vertexdata[f->vertex2], vertexdata[f->vertex1], v2);
		CrossProduct(v2, v1, norm);
		m_trianglesFacingLight[i] = DotProduct(norm, m_ShadowDir) > 0;

		if (m_trianglesFacingLight[i])
		{
			m_shadowVolumeIndexes[numIndexes] =	f->vertex0;
			m_shadowVolumeIndexes[numIndexes + 1] = f->vertex2;
			m_shadowVolumeIndexes[numIndexes + 2] = f->vertex1;

			m_shadowVolumeIndexes[numIndexes + 3] = f->vertex0 + 1;
			m_shadowVolumeIndexes[numIndexes + 4] = f->vertex1 + 1;
			m_shadowVolumeIndexes[numIndexes + 5] = f->vertex2 + 1;

			numIndexes += 6;
		}
	}

	std::vector<Edge>::iterator e;
	for (e = data.edges.begin(); e < data.edges.end(); ++e)
	{
		if (m_trianglesFacingLight[e->face0])
		{
			if ((e->face1 != -1) && m_trianglesFacingLight[e->face1])
				continue;

			m_shadowVolumeIndexes[numIndexes] = e->vertex0;
			m_shadowVolumeIndexes[numIndexes + 1] = e->vertex1;
		}
		else
		{
			if ((e->face1 == -1) || !m_trianglesFacingLight[e->face1])
				continue;

			m_shadowVolumeIndexes[numIndexes] = e->vertex1;
			m_shadowVolumeIndexes[numIndexes + 1] = e->vertex0;
		}
		m_shadowVolumeIndexes[numIndexes + 2] = m_shadowVolumeIndexes[numIndexes] + 1;
		m_shadowVolumeIndexes[numIndexes + 3] = m_shadowVolumeIndexes[numIndexes + 2];
		m_shadowVolumeIndexes[numIndexes + 4] = m_shadowVolumeIndexes[numIndexes + 1];
		m_shadowVolumeIndexes[numIndexes + 5] = m_shadowVolumeIndexes[numIndexes + 1] + 1;
		numIndexes += 6;
	}

	if (m_bTwoSideSupported)
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