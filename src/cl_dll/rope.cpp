/*
AR Software (c) 2004-2007. ArrangeMent, S2P Physics, Spirit Of Half-Life and their
logos are the property of their respective owners.

Edits by: Magic_Nipples, 2019.
*/
//magic nipples - ropes

#include "hud.h"
#include "cl_util.h"

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"

#include "pm_defs.h"
#include "event_api.h"
#include "pmtrace.h"						

#include "rope.h"

#include "triangleapi.h"

#include "r_studioint.h"
#include "com_model.h"

extern engine_studio_api_t IEngineStudio;
extern void RenderFog(void);

GLRopeRender gRopeRender;

GLRopeRender::GLRopeRender()
{
	ResetRopes();
}
GLRopeRender::~GLRopeRender()
{
}

void GLRopeRender::ResetRopes()
{
	rope_id = 0;

	for (int p = 0; p < MAX_ROPES; p++)//iterate through all rope IDs
		hRope[p].bDraw = FALSE;
}

void GLRopeRender::DrawBeam(vec3_t start, vec3_t end, float width, char* Sprite, bool bCheckLight, int r, int g, int b)
{
	//if there's lag, blame this piece of code
	if (bCheckLight)
	{
		// create temporary entity to get illumination from it
		alight_t lighting;
		cl_entity_t* LightEntity;
		vec3_t light_dir;

		// bogus data as valve calls it :)
		LightEntity = gEngfuncs.GetLocalPlayer();

		if (!LightEntity)
			return;

		// move it in the particles location
		LightEntity->origin = start;

		// I have no idea what this does but if you don't do it -> crash
		lighting.plightvec = light_dir;

		IEngineStudio.StudioDynamicLight(LightEntity, &lighting);
		IEngineStudio.StudioSetupLighting(&lighting);
		IEngineStudio.StudioEntityLight(&lighting);

		r = (int)(lighting.color[0] * lighting.shadelight + 50);
		g = (int)(lighting.color[1] * lighting.shadelight + 50);
		b = (int)(lighting.color[2] * lighting.shadelight + 50);
	}

	HLSPRITE texture = SPR_Load(Sprite);
	extern vec3_t v_angles, v_origin;

	struct model_s* pModel;

	pModel = (struct model_s*)gEngfuncs.GetSpritePointer(texture);

	if (!gEngfuncs.pTriAPI->SpriteTexture(pModel, 0))
		return;

	vec3_t dir = (start - end).Normalize();
	vec3_t view, v_right, mid;

	gEngfuncs.GetViewAngles(view);
	AngleVectors(view, view, NULL, NULL);
	view = view.Normalize();

	float l = (start - end).Length();
	mid = start + (start - end).Normalize() * l * 0.5;

	view = (v_origin - mid).Normalize();

	v_right = CrossProduct(view, dir);

	v_right = v_right.Normalize();

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);

	//RenderFog();

	gEngfuncs.pTriAPI->Begin(TRI_TRIANGLES);

	//	if( bCheckLight )

	gEngfuncs.pTriAPI->Color4ub(r, g, b, 255);
	//	else
	//	gEngfuncs.pTriAPI->Color4f(1,1,1,1);

	gEngfuncs.pTriAPI->Brightness(1);

	gEngfuncs.pTriAPI->TexCoord2f(0, 1);
	gEngfuncs.pTriAPI->Vertex3fv(end - v_right * width);

	gEngfuncs.pTriAPI->TexCoord2f(1, 1);
	gEngfuncs.pTriAPI->Vertex3fv(end + v_right * width);

	gEngfuncs.pTriAPI->TexCoord2f(1, 0);
	gEngfuncs.pTriAPI->Vertex3fv(start + v_right * width);

	//Repeats for triangles
	gEngfuncs.pTriAPI->TexCoord2f(0, 1);
	gEngfuncs.pTriAPI->Vertex3fv(end - v_right * width);

	gEngfuncs.pTriAPI->TexCoord2f(1, 0);
	gEngfuncs.pTriAPI->Vertex3fv(start + v_right * width);
	//End repeats

	gEngfuncs.pTriAPI->TexCoord2f(0, 0);
	gEngfuncs.pTriAPI->Vertex3fv(start - v_right * width);

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

//MSG me!
//int rope_num_segments = 20;
//int rope_lenght = 100;


void GLRopeRender::CreateRope(char* datafile, vec3_t start_source, vec3_t end_source)
{
	vec3_t vBezier;//temp

	if (!start_source)
	{
		gEngfuncs.Con_Printf("No Start_Ent\n");
		return;
	}

	if (!end_source)
	{
		gEngfuncs.Con_Printf("No End_Ent\n");
		return;
	}



	char file[512];

	sprintf(file, "scripts/ropes/%s.rpe", datafile);
	//	sprintf( file, "scripts/rpe/test.rpe" );

	char* szFile = (char*)gEngfuncs.COM_LoadFile(file, 5, NULL);
	char szToken[512];

	if (!szFile)
	{
		gEngfuncs.Con_Printf("Couldn't open Rope file %s for simulation\n", file);
		return;
	}
	else
	{
		szFile = gEngfuncs.COM_ParseFile(szFile, szToken);

		while (szFile)
		{
			if (!_stricmp(szToken, "num_segments"))
			{
				szFile = gEngfuncs.COM_ParseFile(szFile, szToken);
				hRope[rope_id].mySegments = atoi(szToken);
			}
			else if (!_stricmp(szToken, "rope_length"))
			{
				szFile = gEngfuncs.COM_ParseFile(szFile, szToken);
				hRope[rope_id].myLenght = atoi(szToken);
			}
			else if (!_stricmp(szToken, "scale"))
			{
				szFile = gEngfuncs.COM_ParseFile(szFile, szToken);
				hRope[rope_id].myScale = atof(szToken);
			}
			else if (!_stricmp(szToken, "sprite"))
			{
				szFile = gEngfuncs.COM_ParseFile(szFile, szToken);
				strncpy(hRope[rope_id].mySpriteFile, szToken, sizeof(hRope[rope_id].mySpriteFile));
			}
			else if (!_stricmp(szToken, "r"))
			{
				szFile = gEngfuncs.COM_ParseFile(szFile, szToken);
				hRope[rope_id].r = atoi(szToken);
			}
			else if (!_stricmp(szToken, "g"))
			{
				szFile = gEngfuncs.COM_ParseFile(szFile, szToken);
				hRope[rope_id].g = atoi(szToken);
			}

			else if (!_stricmp(szToken, "b"))
			{
				szFile = gEngfuncs.COM_ParseFile(szFile, szToken);
				hRope[rope_id].b = atoi(szToken);
			}
			else if (!_stricmp(szToken, "light_check"))
			{
				szFile = gEngfuncs.COM_ParseFile(szFile, szToken);
				if (!_stricmp(szToken, "true"))
				{
					hRope[rope_id].bCheckLight = true;
				}
				if (!_stricmp(szToken, "false"))
				{
					hRope[rope_id].bCheckLight = false;
				}
			}
			szFile = gEngfuncs.COM_ParseFile(szFile, szToken);
		}
	}

	gEngfuncs.COM_FreeFile(szFile);


	if (hRope[rope_id].mySegments > MAX_SEGMENTS)
		hRope[rope_id].mySegments = MAX_SEGMENTS;

	if (hRope[rope_id].myScale <= 0.1)
		hRope[rope_id].myScale = 0.1;

	float detailBias = 1.0 / (float)hRope[rope_id].mySegments; //we'll put 51 points on out curve (0.02 detail bias)
	float count = 0.0;  //used as our counter



//	hRope[rope_id].mySegments = rope_num_segments;
//	hRope[rope_id].myLenght = rope_lenght;

//	sprintf( hRope[rope_id].mySpriteFile, "sprites/rope.spr" );

//	hRope[rope_id].myScale = 1;

	hRope[rope_id].myPoints[START] = start_source;
	hRope[rope_id].myPoints[END] = end_source;

	hRope[rope_id].myPoints[MID_POINT].x = (hRope[rope_id].myPoints[START].x + hRope[rope_id].myPoints[END].x) / 2;
	hRope[rope_id].myPoints[MID_POINT].y = (hRope[rope_id].myPoints[START].y + hRope[rope_id].myPoints[END].y) / 2;
	hRope[rope_id].myPoints[MID_POINT].z = (hRope[rope_id].myPoints[START].z + hRope[rope_id].myPoints[END].z) / 2;


	//rope starts above me! I am not the start
	if (hRope[rope_id].myPoints[END].z > hRope[rope_id].myPoints[START].z)
		hRope[rope_id].myPoints[MID_POINT].z = hRope[rope_id].myPoints[START].z;
	else//I am the start, it ends belows me
		hRope[rope_id].myPoints[MID_POINT].z = hRope[rope_id].myPoints[END].z;

	hRope[rope_id].myPoints[MID_POINT].z -= hRope[rope_id].myLenght;//this is because rope lenght

	int vs_id = 0;

	//create lot of vectors which creates a curve
	do
	{
		vBezier.x = hRope[rope_id].myPoints[START].x * B1(count) + hRope[rope_id].myPoints[MID_POINT].x * B2(count) + hRope[rope_id].myPoints[END].x * B3(count);
		vBezier.y = hRope[rope_id].myPoints[START].y * B1(count) + hRope[rope_id].myPoints[MID_POINT].y * B2(count) + hRope[rope_id].myPoints[END].y * B3(count);
		vBezier.z = hRope[rope_id].myPoints[START].z * B1(count) + hRope[rope_id].myPoints[MID_POINT].z * B2(count) + hRope[rope_id].myPoints[END].z * B3(count);

		hRope[rope_id].mySpline[vs_id] = vBezier;

		vs_id++;//this will executed detailBias=value times!
		//CONPRINT ( "x %f, y %f, z %f - vs_id: %i\n",vBezier.x,vBezier.y,vBezier.z, vs_id );

		count += detailBias;

	} while (count <= 1.0);

	//if everything was okay, sum a rope and check draw it
	hRope[rope_id].bDraw = TRUE;
	rope_id++;
}

//draw ALL segments for ALL splines
void GLRopeRender::DrawRopes(float fltime)
{
	for (int p = 0; p < MAX_ROPES; p++)//iterate through all rope IDs
	{
		if (hRope[p].bDraw == TRUE)
			for (int i = 0; i < hRope[p].mySegments; i++)
			{
				if (i == hRope[p].mySegments - 1)//no info on the next vector, tie up with the end
					DrawBeam(hRope[p].mySpline[i], hRope[p].myPoints[START], hRope[p].myScale, hRope[p].mySpriteFile, hRope[p].bCheckLight, hRope[p].r, hRope[p].g, hRope[p].b);
				else
					DrawBeam(hRope[p].mySpline[i], hRope[p].mySpline[i + 1], hRope[p].myScale, hRope[p].mySpriteFile, hRope[p].bCheckLight, hRope[p].r, hRope[p].g, hRope[p].b);
			}
	}
}