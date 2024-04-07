//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SVD_FORMAT_HEADER
#define SVD_FORMAT_HEADER

#include "com_model.h"
#include "studio.h"

#define MAX_SVD_FILES	512
#define SVD_VERSION		4

struct svdedge_t
{
	int vertex0;
	int vertex1;
	int face0;
	int face1;
};

struct svdface_t
{
	int vertex0;
	int vertex1;
	int vertex2;
};

struct svdsubmodel_t
{
	int faceindex;
	int numfaces;

	int edgeindex;
	int numedges;

	int vertinfoindex;
	int vertexindex;
	int numverts;

	mstudioboneweight_t* pvertweight;
};

struct svdbodypart_t
{
	int base;
	int submodelindex;
	int numsubmodels;
};

struct svdheader_t
{
	int version;
	char modelname[64];
	int mdl_size;

	int bodypartindex;
	int numbodyparts;

	int num_faces;
	int num_edges;
};

svdheader_t* SVD_Create( char* filename, model_t* pmodel );
bool SVD_LoadSVDForModel( model_t* pmodel );
void SVD_BuildFaces ( svdsubmodel_t* psubmodel, mstudiomodel_t* pstudiosubmodel, studiohdr_t* phdr );
void SVD_BuildEdges ( svdsubmodel_t* psubmodel );
void SVD_AddEdge ( svdedge_t* pedgebuffer, int* pnumedges, int face, int v0, int v1 );

void SVD_VidInit( void );
void SVD_Clear( void );
void SVD_CheckInit( void );
#endif