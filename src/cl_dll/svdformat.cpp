// studio_model.cpp
// routines for setting up to draw 3DStudio models

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
#include "svdformat.h"

// Temporary buffer size for SVDs
#define SVD_TEMP_BUFFER_SIZE 1024*1024*4

// Pointers used for data construction
int				g_iSVDDataOffset;
byte*			g_pSVDBufferData;
svdheader_t*	g_pSVDHeader;

// Structure holding pointers to svd data
svdheader_t*	g_pSVDHeaders[MAX_SVD_FILES];
int				g_iNumSVDFiles;

// Boolean to signal that we need to load svds
bool			g_bNeedLoadSVD;

extern engine_studio_api_t IEngineStudio;

/*
====================
SVD_BuildFaces

====================
*/
void SVD_BuildFaces( svdsubmodel_t* psubmodel, mstudiomodel_t* pstudiosubmodel, studiohdr_t* phdr )
{
	// get number of triangles in all meshes
	mstudiomesh_t *pmeshes = (mstudiomesh_t *)((byte *)phdr + pstudiosubmodel->meshindex);
	for (int i = 0; i < pstudiosubmodel->nummesh; i++) 
	{
		int j;
		short *ptricmds = (short *)((byte *)phdr + pmeshes[i].triindex);		
		while (j = *(ptricmds++))
		{
			if (j < 0) 
				j *= -1;

			psubmodel->numfaces += (j - 2);
			ptricmds += 4 * j;
		}
	}

	// No faces in this submodel
	if (psubmodel->numfaces == 0)  
		return;

	// Allocate spot for face data
	psubmodel->faceindex = g_iSVDDataOffset;
	g_iSVDDataOffset += sizeof(svdface_t)*psubmodel->numfaces;

	// Copy over data
	int faceindex = 0;
	svdface_t* pfaces = (svdface_t *)((byte *)(svdheader_t *)g_pSVDHeader + psubmodel->faceindex);

	for (int i = 0; i < pstudiosubmodel->nummesh; i++) 
	{
		short *ptricmds = (short *)((byte *)phdr + pmeshes[i].triindex);

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
				pfaces[faceindex].vertex0 = indices[0];
				pfaces[faceindex].vertex1 = indices[1];
				pfaces[faceindex].vertex2 = indices[2];
				faceindex++;

				bool reverse = false;
				for( ; j > 0; j--, ptricmds += 4)
				{
					indices[0] = indices[1];
					indices[1] = indices[2];
					indices[2] = ptricmds[0];

					// Add the face
					if (!reverse)
					{
						pfaces[faceindex].vertex0 = indices[2];
						pfaces[faceindex].vertex1 = indices[1];
						pfaces[faceindex].vertex2 = indices[0];
						faceindex++;
					}
					else
					{
						pfaces[faceindex].vertex0 = indices[0];
						pfaces[faceindex].vertex1 = indices[1];
						pfaces[faceindex].vertex2 = indices[2];
						faceindex++;
					}

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
				pfaces[faceindex].vertex0 = indices[0];
				pfaces[faceindex].vertex1 = indices[1];
				pfaces[faceindex].vertex2 = indices[2];
				faceindex++;

				for( ; j > 0; j--, ptricmds += 4)
				{
					indices[1] = indices[2];
					indices[2] = ptricmds[0];

					// Add the face
					pfaces[faceindex].vertex0 = indices[0];
					pfaces[faceindex].vertex1 = indices[1];
					pfaces[faceindex].vertex2 = indices[2];
					faceindex++;
				}
			}
		}
	}
}

/*
====================
SVD_BuildEdges

====================
*/
void SVD_BuildEdges( svdsubmodel_t* psubmodel )
{
	if(!psubmodel->numfaces)
		return;

	// Allocate a temporary buffer
	svdedge_t* pedgebuffer = new svdedge_t[psubmodel->numfaces*3];
	memset(pedgebuffer, 0, sizeof(svdedge_t)*psubmodel->numfaces*3);
	
	// Process each face
	svdface_t* pfaces = (svdface_t *)((byte *)(svdheader_t *)g_pSVDHeader + psubmodel->faceindex);
	for (int i = 0; i < psubmodel->numfaces; i++)
	{
		SVD_AddEdge(pedgebuffer, &psubmodel->numedges, i, pfaces[i].vertex0, pfaces[i].vertex1);
		SVD_AddEdge(pedgebuffer, &psubmodel->numedges, i, pfaces[i].vertex1, pfaces[i].vertex2);
		SVD_AddEdge(pedgebuffer, &psubmodel->numedges, i, pfaces[i].vertex2, pfaces[i].vertex0);
	}

	// Allocate spot for edges
	psubmodel->edgeindex = g_iSVDDataOffset;
	g_iSVDDataOffset += sizeof(svdedge_t)*psubmodel->numedges;

	// Copy over data
	svdedge_t* poutedges = (svdedge_t *)((byte *)(svdheader_t *)g_pSVDHeader + psubmodel->edgeindex);
	memcpy(poutedges, pedgebuffer, sizeof(svdedge_t)*psubmodel->numedges);

	// Free edges
	delete [] pedgebuffer;
}

/*
====================
SVD_AddEdge

====================
*/
void SVD_AddEdge( svdedge_t* pedgebuffer, int* pnumedges, int face, int v0, int v1 )
{
	// first look for face's neighbour
	for (int i = 0; i < (*pnumedges); i++)
	{
		if ((pedgebuffer[i].vertex0 == v1) && (pedgebuffer[i].vertex1 == v0) && (pedgebuffer[i].face1 == -1))
		{
			pedgebuffer[i].face1 = face;
			return;
		}
	}

	// add new edge to list
	svdedge_t* pnew = &pedgebuffer[(*pnumedges)];
	(*pnumedges)++;

	pnew->face0 = face;
	pnew->face1 = -1;
	pnew->vertex0 = v0;
	pnew->vertex1 = v1;
}

/*
====================
SVD_IndexShift

====================
*/
void SVD_IndexShift( svdsubmodel_t* psubmodel )
{
	svdface_t* pfaces = (svdface_t *)((byte *)g_pSVDHeader + psubmodel->faceindex);
	for (int i = 0; i < psubmodel->numfaces; i++)
	{
		pfaces[i].vertex0 *= 2;
		pfaces[i].vertex1 *= 2;
		pfaces[i].vertex2 *= 2;
	}

	svdedge_t* pedges = (svdedge_t *)((byte *)g_pSVDHeader + psubmodel->edgeindex);
	for (int i = 0; i < psubmodel->numedges; i++)
	{
		pedges[i].vertex0 *= 2;
		pedges[i].vertex1 *= 2;
	}
}

/*
====================
SVD_SetVertexes

====================
*/
void SVD_SetVertexes( svdsubmodel_t* psubmodel, mstudiomodel_t* pstsubmodel, studiohdr_t* phdr )
{
	// Get pointers to vertex data
	vec3_t* pstudioverts = (vec3_t *)((byte *)phdr + pstsubmodel->vertindex);
	byte* pvertbones = (byte *)((byte *)phdr + pstsubmodel->vertinfoindex);

	// Allocate and copy vertex data
	psubmodel->vertexindex = g_iSVDDataOffset;
	psubmodel->numverts = pstsubmodel->numverts;

	vec3_t* pdestverts = (vec3_t *)((byte *)g_pSVDBufferData + g_iSVDDataOffset);
	g_iSVDDataOffset += sizeof(vec3_t)*psubmodel->numverts;

	memcpy(pdestverts, pstudioverts, sizeof(vec3_t)*psubmodel->numverts);

	// Allocate and copy vertex bone info
	psubmodel->vertinfoindex = g_iSVDDataOffset;
	byte* pdestvertbones = ((byte *)g_pSVDBufferData + g_iSVDDataOffset);
	g_iSVDDataOffset += sizeof(byte)*psubmodel->numverts;

	memcpy(pdestvertbones, pvertbones, sizeof(byte)*psubmodel->numverts);


	psubmodel->pvertweight = (mstudioboneweight_t*)((byte*)phdr + pstsubmodel->blendvertinfoindex);
}

/*
====================
SVD_SetupModel

====================
*/
svdheader_t* SVD_Create( char* filename, model_t* pmodel )
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata(pmodel);

	// Fail if no bodyparts
	if (pstudiohdr->numbodyparts == 0)
	{
		gEngfuncs.Con_Printf("Error: model %s has 0 submodels\n", pmodel->name);
		return false;
	}
	
	// Allocate the buffer
	g_iSVDDataOffset = 0;
	g_pSVDBufferData = new byte[SVD_TEMP_BUFFER_SIZE];
	memset(g_pSVDBufferData, 0, sizeof(byte)*SVD_TEMP_BUFFER_SIZE);

	// Get header
	g_pSVDHeader = (svdheader_t *)g_pSVDBufferData;
	g_iSVDDataOffset += sizeof(svdheader_t);

	// Set basics
	strcpy(g_pSVDHeader->modelname, pmodel->name);
	g_pSVDHeader->mdl_size = pstudiohdr->length;
	g_pSVDHeader->version = SVD_VERSION;

	// Allocate submodels
	svdbodypart_t *pbodyparts = (svdbodypart_t *)(g_pSVDBufferData + g_iSVDDataOffset);
	g_pSVDHeader->bodypartindex = g_iSVDDataOffset;
	g_pSVDHeader->numbodyparts = pstudiohdr->numbodyparts;
	g_iSVDDataOffset += sizeof(svdbodypart_t)*pstudiohdr->numbodyparts;

	// convert strips and fans to triangles, generate adjacency info
	for (int i = 0; i < pstudiohdr->numbodyparts; i++)
	{
		mstudiobodyparts_t* pstbodypart = (mstudiobodyparts_t *)((byte *)pstudiohdr + pstudiohdr->bodypartindex) + i;

		// Set bodypart info
		pbodyparts[i].submodelindex = g_iSVDDataOffset;
		pbodyparts[i].numsubmodels = pstbodypart->nummodels;
		pbodyparts[i].base = pstbodypart->base;

		// Allocate submodel data
		svdsubmodel_t *psubmodels = (svdsubmodel_t *)(g_pSVDBufferData + g_iSVDDataOffset);
		g_iSVDDataOffset += sizeof(svdsubmodel_t)*pstbodypart->nummodels;

		for (int j = 0; j < pstbodypart->nummodels; j++)
		{
			mstudiomodel_t *pstsubmodel = (mstudiomodel_t *)((byte *)pstudiohdr + pstbodypart->modelindex) + j;
			
			SVD_SetVertexes(&psubmodels[j], pstsubmodel, pstudiohdr);
			SVD_BuildFaces(&psubmodels[j], pstsubmodel, pstudiohdr);
			SVD_BuildEdges(&psubmodels[j]);
			SVD_IndexShift(&psubmodels[j]);
			
			g_pSVDHeader->num_faces += psubmodels[j].numfaces;
			g_pSVDHeader->num_edges += psubmodels[j].numedges;
		}
	}

	// Save the data to the disk
	char outPath[MAX_PATH];
	sprintf(outPath, "%s/%s", gEngfuncs.pfnGetGameDirectory(), filename);

	FILE *pFile = fopen(outPath, "wb");
	if(!pFile)
		return NULL;

	fwrite(g_pSVDBufferData, sizeof(byte)*g_iSVDDataOffset, 1, pFile);
	fclose(pFile);

	// Allocate the return data
	byte* pbuffer = new byte[g_iSVDDataOffset];
	memcpy(pbuffer, g_pSVDBufferData, sizeof(byte)*g_iSVDDataOffset);

	// Free temp buffer
	delete [] g_pSVDBufferData;
	g_pSVDBufferData = NULL;

	return (svdheader_t*)pbuffer;
}

/*
====================
SVD_SetupModel

====================
*/
bool SVD_LoadSVDForModel( model_t* pmodel )
{
	if(g_iNumSVDFiles == MAX_SVD_FILES)
		return false;

	// Try and load the model from the disk
	char outName[MAX_PATH];
	strcpy(outName, pmodel->name);
	strcpy(&outName[strlen(outName)-3], "svd");

	int fileSize = 0;
	byte* pFile = gEngfuncs.COM_LoadFile(outName, 5, &fileSize);

	// Pointer to svd header
	svdheader_t* psvdheader = NULL;

	// File was found, verify that sizes match
	if(pFile)
	{
		svdheader_t *pfileheader = (svdheader_t *)pFile;
		studiohdr_t* pstudiohdr = (studiohdr_t *)IEngineStudio.Mod_Extradata( pmodel );

		if(pfileheader->version == SVD_VERSION
			&& pfileheader->mdl_size == pstudiohdr->length
			&& !strcmp(pfileheader->modelname, pmodel->name))
		{
			// Allocate new buffer
			byte* pbuffer = new byte[fileSize];
			memcpy(pbuffer, pFile, fileSize);
			
			// Set the pointer
			psvdheader = (svdheader_t*)pbuffer;
		}

		gEngfuncs.COM_FreeFile(pFile);

		//FIXME - magic nipples - havent figured out saving the data for boneweights to the svd file. creating one prevents crash but sucks obviously... 
		if (pstudiohdr->flags & STUDIO_HAS_BONEWEIGHTS)
			psvdheader = SVD_Create(outName, pmodel);
	}

	// Failed for some reason, so create
	if(!psvdheader)
		psvdheader = SVD_Create(outName, pmodel);

	// Total failure
	if(!psvdheader)
		return false;

	// Add it to the array
	g_pSVDHeaders[g_iNumSVDFiles] = psvdheader;
	g_iNumSVDFiles++;

	// Set pointer in model
	pmodel->visdata = (byte*)psvdheader;
	return true;
}

/*
====================
SVD_CheckInit

====================
*/
void SVD_CheckInit( void )
{
	if(!g_bNeedLoadSVD)
		return;

	model_t* pmodel = NULL;
	for(int i = 0; i < MAX_SVD_FILES; i++)
	{
		pmodel = IEngineStudio.GetModelByIndex(i+1);
		if(!pmodel)
			break;

		if(pmodel->type != mod_studio)
			continue;

		if(!SVD_LoadSVDForModel(pmodel))
			gEngfuncs.Con_Printf("Failed to set SVD data for %s\n", pmodel->name);
	}

	g_bNeedLoadSVD = false;
}

/*
====================
SVD_Clear

====================
*/
void SVD_Clear( void )
{
	for(int i = 0; i < g_iNumSVDFiles; i++)
	{
		delete g_pSVDHeaders[i];
		g_pSVDHeaders[i] = NULL;
	}

	g_iNumSVDFiles = NULL;

	// Flag for next load
	g_bNeedLoadSVD = true;
}
