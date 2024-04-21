//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"

//RAIN
#include "rain.h"
#include "com_model.h"
#include "studio_util.h"

//magic nipples - fog fix
#include <windows.h>
#include <gl/gl.h>

#include "StencilShadowRender.h" // STENCIL SHADOWS

#define DLLEXPORT __declspec( dllexport )

extern "C"
{
	void DLLEXPORT HUD_DrawNormalTriangles( void );
	void DLLEXPORT HUD_DrawTransparentTriangles( void );
};

void SetPoint(float x, float y, float z, float(*matrix)[4])
{
	vec3_t point, result;
	point[0] = x;
	point[1] = y;
	point[2] = z;

	VectorTransform(point, matrix, result);

	gEngfuncs.pTriAPI->Vertex3f(result[0], result[1], result[2]);
}

/*
=================================
DrawRain

draw raindrips and snowflakes
=================================
*/
extern cl_drip FirstChainDrip;
extern rain_properties Rain;

void DrawRain(void)
{
	if (FirstChainDrip.p_Next == NULL)
		return; // no drips to draw

	HLSPRITE hsprTexture;
	const model_s* pTexture;
	float visibleHeight = Rain.globalHeight - SNOWFADEDIST;

	if (Rain.weatherMode == 0)
		hsprTexture = LoadSprite("gfx/weather/rain.spr"); // load rain sprite
	else if (Rain.weatherMode == 2)
		hsprTexture = LoadSprite("gfx/weather/dust.spr"); // load dust sprite
	else
		hsprTexture = LoadSprite("gfx/weather/snow.spr"); // load snow sprite

	// usual triapi stuff
	pTexture = gEngfuncs.GetSpritePointer(hsprTexture);
	gEngfuncs.pTriAPI->SpriteTexture((struct model_s*)pTexture, 0);
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);

	glDisable(GL_FOG); //magic nipples - fog fix

	// go through drips list
	cl_drip* Drip = FirstChainDrip.p_Next;
	cl_entity_t* player = gEngfuncs.GetLocalPlayer();

	if (Rain.weatherMode == 0) // draw rain
	{
		while (Drip != NULL)
		{
			cl_drip* nextdDrip = Drip->p_Next;

			Vector2D toPlayer;
			toPlayer.x = player->origin[0] - Drip->origin[0];
			toPlayer.y = player->origin[1] - Drip->origin[1];
			toPlayer = toPlayer.Normalize();

			toPlayer.x *= DRIP_SPRITE_HALFWIDTH;
			toPlayer.y *= DRIP_SPRITE_HALFWIDTH;

			float shiftX = (Drip->xDelta / DRIPSPEED) * DRIP_SPRITE_HALFHEIGHT;
			float shiftY = (Drip->yDelta / DRIPSPEED) * DRIP_SPRITE_HALFHEIGHT;

			// --- draw triangle --------------------------
			gEngfuncs.pTriAPI->Color4f(1.0, 1.0, 1.0, Drip->alpha);
			gEngfuncs.pTriAPI->Begin(TRI_TRIANGLES);

			gEngfuncs.pTriAPI->TexCoord2f(0, 0);
			gEngfuncs.pTriAPI->Vertex3f(Drip->origin[0] - toPlayer.y - shiftX, Drip->origin[1] + toPlayer.x - shiftY, Drip->origin[2] + DRIP_SPRITE_HALFHEIGHT);

			gEngfuncs.pTriAPI->TexCoord2f(0.5, 1);
			gEngfuncs.pTriAPI->Vertex3f(Drip->origin[0] + shiftX, Drip->origin[1] + shiftY, Drip->origin[2] - DRIP_SPRITE_HALFHEIGHT);

			gEngfuncs.pTriAPI->TexCoord2f(1, 0);
			gEngfuncs.pTriAPI->Vertex3f(Drip->origin[0] + toPlayer.y - shiftX, Drip->origin[1] - toPlayer.x - shiftY, Drip->origin[2] + DRIP_SPRITE_HALFHEIGHT);

			gEngfuncs.pTriAPI->End();
			// --- draw triangle end ----------------------

			Drip = nextdDrip;
		}
	}
	/////////////////
	// SYS: 
	// Dust mode
	/////////////////
	else if (Rain.weatherMode == 2) // draw dust
	{
		vec3_t normal;
		gEngfuncs.GetViewAngles((float*)normal);

		float matrix[3][4];
		AngleMatrix(normal, matrix);	// calc view matrix

		while (Drip != NULL)
		{
			cl_drip* nextdDrip = Drip->p_Next;

			matrix[0][3] = Drip->origin[0]; // write origin to matrix
			matrix[1][3] = Drip->origin[1];
			matrix[2][3] = Drip->origin[2];

			// apply start fading effect
			float alpha = (Drip->origin[2] <= visibleHeight) ? Drip->alpha : ((Rain.globalHeight - Drip->origin[2]) / (float)SNOWFADEDIST) * Drip->alpha;

			// --- draw quad --------------------------
			gEngfuncs.pTriAPI->Color4f(1.0, 1.0, 1.0, alpha);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS);

			gEngfuncs.pTriAPI->TexCoord2f(0, 0);
			SetPoint(0, DUST_SPRITE_HALFSIZE, DUST_SPRITE_HALFSIZE, matrix);

			gEngfuncs.pTriAPI->TexCoord2f(0, 1);
			SetPoint(0, DUST_SPRITE_HALFSIZE, -DUST_SPRITE_HALFSIZE, matrix);

			gEngfuncs.pTriAPI->TexCoord2f(1, 1);
			SetPoint(0, -DUST_SPRITE_HALFSIZE, -DUST_SPRITE_HALFSIZE, matrix);

			gEngfuncs.pTriAPI->TexCoord2f(1, 0);
			SetPoint(0, -DUST_SPRITE_HALFSIZE, DUST_SPRITE_HALFSIZE, matrix);

			gEngfuncs.pTriAPI->End();
			// --- draw quad end ----------------------

			Drip = nextdDrip;
		}
	}
	/////////////////
	// SYS: 
	// Dust mode
	/////////////////
	else	// draw snow
	{
		vec3_t normal;
		gEngfuncs.GetViewAngles((float*)normal);

		float matrix[3][4];
		AngleMatrix(normal, matrix);	// calc view matrix

		while (Drip != NULL)
		{
			cl_drip* nextdDrip = Drip->p_Next;

			matrix[0][3] = Drip->origin[0]; // write origin to matrix
			matrix[1][3] = Drip->origin[1];
			matrix[2][3] = Drip->origin[2];

			// apply start fading effect
			float alpha = (Drip->origin[2] <= visibleHeight) ? Drip->alpha : ((Rain.globalHeight - Drip->origin[2]) / (float)SNOWFADEDIST) * Drip->alpha;

			// --- draw quad --------------------------
			gEngfuncs.pTriAPI->Color4f(1.0, 1.0, 1.0, alpha);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS);

			gEngfuncs.pTriAPI->TexCoord2f(0, 0);
			SetPoint(0, SNOW_SPRITE_HALFSIZE, SNOW_SPRITE_HALFSIZE, matrix);

			gEngfuncs.pTriAPI->TexCoord2f(0, 1);
			SetPoint(0, SNOW_SPRITE_HALFSIZE, -SNOW_SPRITE_HALFSIZE, matrix);

			gEngfuncs.pTriAPI->TexCoord2f(1, 1);
			SetPoint(0, -SNOW_SPRITE_HALFSIZE, -SNOW_SPRITE_HALFSIZE, matrix);

			gEngfuncs.pTriAPI->TexCoord2f(1, 0);
			SetPoint(0, -SNOW_SPRITE_HALFSIZE, SNOW_SPRITE_HALFSIZE, matrix);

			gEngfuncs.pTriAPI->End();
			// --- draw quad end ----------------------

			Drip = nextdDrip;
		}
	}
}

/*
=================================
DrawFXObjects
=================================
*/
extern cl_rainfx FirstChainFX;

#define MAX_SPLASH_FRAMES 14

void DrawFXObjects(void)
{
	if (FirstChainFX.p_Next == NULL)
		return; // no objects to draw

	if (gHUD.RainSplash->value == 0.0)
		return; // no splashes to draw

	float curtime = gEngfuncs.GetClientTime();

	vec3_t normal;
	gEngfuncs.GetViewAngles((float*)normal);

	float matrix[3][4];
	AngleMatrix(normal, matrix);	// calc view matrix

	// usual triapi stuff
	HLSPRITE hsprTexture;
	const model_s* pTexture;
	hsprTexture = LoadSprite("gfx/weather/splash.spr"); // load water ring sprite
	pTexture = gEngfuncs.GetSpritePointer(hsprTexture);
	gEngfuncs.pTriAPI->SpriteTexture((struct model_s*)pTexture, 0);
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);

	// go through objects list
	cl_rainfx* curFX = FirstChainFX.p_Next;
	while (curFX != NULL)
	{
		cl_rainfx* nextFX = curFX->p_Next;

		matrix[0][3] = curFX->origin[0]; // write origin to matrix
		matrix[1][3] = curFX->origin[1];
		matrix[2][3] = curFX->origin[2] + 8;

		curFX->frame += gHUD.m_flTimeDelta * 30;

		// fadeout
		float alpha = ((curFX->birthTime + curFX->life - curtime) / curFX->life) * curFX->alpha;

		if (curFX->frame >= (MAX_SPLASH_FRAMES - 1))
			curFX->frame = MAX_SPLASH_FRAMES - 1;

		//gEngfuncs.Con_Printf("%f %f\n", curFX->frame, gHUD.m_flTimeDelta);

		gEngfuncs.pTriAPI->SpriteTexture((struct model_s*)pTexture, curFX->frame);

		// --- draw quad --------------------------
		gEngfuncs.pTriAPI->Color4f(1.0, 1.0, 1.0, alpha);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);

		gEngfuncs.pTriAPI->TexCoord2f(0, 0);
		SetPoint(0, SPLASH_SPRITE_HALFSIZE, SPLASH_SPRITE_HALFSIZE, matrix);

		gEngfuncs.pTriAPI->TexCoord2f(0, 1);
		SetPoint(0, SPLASH_SPRITE_HALFSIZE, -SPLASH_SPRITE_HALFSIZE, matrix);

		gEngfuncs.pTriAPI->TexCoord2f(1, 1);
		SetPoint(0, -SPLASH_SPRITE_HALFSIZE, -SPLASH_SPRITE_HALFSIZE, matrix);

		gEngfuncs.pTriAPI->TexCoord2f(1, 0);
		SetPoint(0, -SPLASH_SPRITE_HALFSIZE, SPLASH_SPRITE_HALFSIZE, matrix);

		gEngfuncs.pTriAPI->End();
		// --- draw quad end ----------------------

		curFX = nextFX;
	}
}

//#define TEST_IT
#if defined( TEST_IT )

/*
=================
Draw_Triangles

Example routine.  Draws a sprite offset from the player origin.
=================
*/
void Draw_Triangles( void )
{
	cl_entity_t *player;
	vec3_t org;

	// Load it up with some bogus data
	player = gEngfuncs.GetLocalPlayer();
	if ( !player )
		return;

	org = player->origin;

	org.x += 50;
	org.y += 50;

	if (gHUD.m_hsprCursor == 0)
	{
		char sz[256];
		sprintf( sz, "sprites/cursor.spr" );
		gHUD.m_hsprCursor = SPR_Load( sz );
	}

	if ( !gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)gEngfuncs.GetSpritePointer( gHUD.m_hsprCursor ), 0 ))
	{
		return;
	}
	
	// Create a triangle, sigh
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );
	// Overload p->color with index into tracer palette, p->packedColor with brightness
	gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 1.0 );
	// UNDONE: This gouraud shading causes tracers to disappear on some cards (permedia2)
	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y, org.z );

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}

#endif

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/

void DLLEXPORT HUD_DrawNormalTriangles( void )
{
	RenderShadow(); // STENCIL SHADOWS
	//gHUD.m_Spectator.DrawOverview();

#if defined( TEST_IT )
	//	Draw_Triangles();
#endif
}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles( void )
{
#if defined( TEST_IT )
//	Draw_Triangles();
#endif

	ProcessRain();
	DrawRain();

	if (gHUD.RainSplash->value != 0)
	{
		ProcessFXObjects();
		DrawFXObjects();
	}
}

void RenderFog(void)
{
	int bOn;

	if (gHUD.g_ftargetValue >= 30000 && gHUD.g_iStartValue >= 30000)
		bOn = 0;
	else
		bOn = 1;

	gEngfuncs.pTriAPI->Fog(gHUD.FogColor, gHUD.g_fStartDist, gHUD.g_fFinalValue, bOn, gHUD.g_fskybox);
}