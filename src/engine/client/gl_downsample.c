/*
gl_downsample.c - take a shot of the current resolution and shrink it by a factor

Copyright (C) 2019 Magic_Nipples
*/

#include "common.h"
#include "client.h"
#include "gl_local.h"

static int DOWNSAMPLE_SIZE_X;
static int DOWNSAMPLE_SIZE_Y;

static int r_initsampletexture;
static int r_sampleeffecttexture;
static int r_samplebackuptexture;
static int r_downsampletexture;

static int r_screendownsamplingtexture_size;
static int screen_texture_width, screen_texture_height;
static int r_screenbackuptexture_width, r_screenbackuptexture_height;

// current refdef size:
static int curView_x;
static int curView_y;
static int curView_width;
static int curView_height;

// texture coordinates of screen data inside screentexture
static float screenTex_tcw;
static float screenTex_tch;

static int sample_width;
static int sample_height;

// texture coordinates of adjusted textures
static float sampleText_tcw;
static float sampleText_tch;

/*
=================
R_Bloom_InitBackUpTexture
=================
*/
static void R_Bloom_InitBackUpTexture(int width, int height)
{
	r_screenbackuptexture_width = width;
	r_screenbackuptexture_height = height;

	r_samplebackuptexture = GL_CreateTexture("*samplebackuptexture", width, height, NULL, TF_NEAREST);
}

/*
=================
R_Bloom_InitEffectTexture
=================
*/
static void R_Bloom_InitEffectTexture(void)
{
	if (r_downsample->value < 0)
		r_downsample->value = 0;

	if (r_downsample->value > 5.0f)
		r_downsample->value = 5.0f;

	/*if (cl.local.waterlevel >= 3)
	{
		DOWNSAMPLE_SIZE_X = screen_texture_width / (2.5);
		DOWNSAMPLE_SIZE_Y = screen_texture_width / (2.5);
	}
	else*/
	{
		DOWNSAMPLE_SIZE_X = screen_texture_width / (r_downsample->value * 2);
		DOWNSAMPLE_SIZE_Y = screen_texture_height / (r_downsample->value * 2);
	}

	r_sampleeffecttexture = GL_CreateTexture("*sampleeffecttexture", DOWNSAMPLE_SIZE_X, DOWNSAMPLE_SIZE_Y, NULL, TF_NEAREST);
}

/*
=================
R_Sampling_InitTextures
=================
*/
static void R_Sampling_InitTextures(void)
{
	screen_texture_width = glState.width;
	screen_texture_height = glState.height;

	// disable blooms if we can't handle a texture of that size
	if (screen_texture_width > glConfig.max_2d_texture_size || screen_texture_height > glConfig.max_2d_texture_size)
	{
		screen_texture_width = screen_texture_height = 0;
		//ALERT( at_warning, "'R_InitBloomScreenTexture' too high resolution for light bloom, effect disabled\n" );
		Con_Printf("'R_Sampling_InitTextures' too high of a resolution, effect disabled\n");
		return;
	}

	r_initsampletexture = GL_CreateTexture("*samplescreentexture", screen_texture_width, screen_texture_height, NULL, TF_NEAREST);

	// validate bloom size and init the bloom effect texture
	R_Bloom_InitEffectTexture();

	// if screensize is more than 2x the bloom effect texture, set up for stepped downsampling
	r_downsampletexture = 0;
	r_screendownsamplingtexture_size = 0;

	R_Bloom_InitBackUpTexture(DOWNSAMPLE_SIZE_X, DOWNSAMPLE_SIZE_Y);
}

/*
=================
R_InitDownSampleTextures
=================
*/
void R_InitDownSampleTextures(void)
{
	DOWNSAMPLE_SIZE_X = 0;
	DOWNSAMPLE_SIZE_Y = 0;

	if (r_initsampletexture)
		GL_FreeTexture(r_initsampletexture);
	if (r_sampleeffecttexture)
		GL_FreeTexture(r_sampleeffecttexture);
	if (r_samplebackuptexture)
		GL_FreeTexture(r_samplebackuptexture);
	if (r_downsampletexture)
		GL_FreeTexture(r_downsampletexture);

	r_initsampletexture = r_sampleeffecttexture = 0;
	r_samplebackuptexture = r_downsampletexture = 0;

	//if ((!r_downsample->value) && (cl.local.waterlevel < 3))
	if (!r_downsample->value)
		return;

	R_Sampling_InitTextures();
}

/*
=================
R_Sampling_Quad
=================
*/
static _inline void R_Sampling_Quad(int x, int y, int w, int h, float texwidth, float texheight)
{
	pglBegin(GL_QUADS);
	pglTexCoord2f(0, texheight);
	pglVertex2f(x, y);
	pglTexCoord2f(0, 0);
	pglVertex2f(x, y + h);
	pglTexCoord2f(texwidth, 0);
	pglVertex2f(x + w, y + h);
	pglTexCoord2f(texwidth, texheight);
	pglVertex2f(x + w, y);
	pglEnd();
}

/*
=================
R_Sampling_DrawEffect
=================
*/
static void R_Sampling_DrawEffect(void)
{
	GL_Bind(GL_TEXTURE0, r_sampleeffecttexture);

	pglBegin(GL_QUADS);
	pglTexCoord2f(0, sampleText_tch);
	pglVertex2f(curView_x, curView_y);
	pglTexCoord2f(0, 0);
	pglVertex2f(curView_x, curView_y + curView_height);
	pglTexCoord2f(sampleText_tcw, 0);
	pglVertex2f(curView_x + curView_width, curView_y + curView_height);
	pglTexCoord2f(sampleText_tcw, sampleText_tch);
	pglVertex2f(curView_x + curView_width, curView_y);
	pglEnd();
}

/*
=================
R_Sampling_GenerateTexture
=================
*/
static void R_Sampling_GenerateTexture(void)
{
	// set up sample size workspace
	pglScissor(0, 0, sample_width, sample_height);
	pglViewport(0, 0, sample_width, sample_height);
	pglMatrixMode(GL_PROJECTION);
	pglLoadIdentity();
	pglOrtho(0, sample_width, sample_height, 0, -10, 100);
	pglMatrixMode(GL_MODELVIEW);
	pglLoadIdentity();

	// copy small scene into r_sampleeffecttexture
	GL_Bind(GL_TEXTURE0, r_sampleeffecttexture);
	pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sample_width, sample_height);

	// restore full screen workspace
	pglScissor(0, 0, glState.width, glState.height);
	pglViewport(0, 0, glState.width, glState.height);
	pglMatrixMode(GL_PROJECTION);
	pglLoadIdentity();
	pglOrtho(0, glState.width, glState.height, 0, -10, 100);
	pglMatrixMode(GL_MODELVIEW);
	pglLoadIdentity();
}

/*
=================
R_Downsample_View
=================
*/
static void R_Downsample_View(void)
{
	// stepped downsample
	int midsample_width = (r_screendownsamplingtexture_size * sampleText_tcw);
	int midsample_height = (r_screendownsamplingtexture_size * sampleText_tch);

	pglDisable(GL_BLEND);

	// copy the screen and draw resized
	GL_Bind(GL_TEXTURE0, r_initsampletexture);
	pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, curView_x, glState.height - (curView_y + curView_height), curView_width, curView_height);
	R_Sampling_Quad(0, glState.height - midsample_height, midsample_width, midsample_height, screenTex_tcw, screenTex_tch);

	// now copy into downsampling (mid-sized) texture
	GL_Bind(GL_TEXTURE0, r_downsampletexture);
	pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, midsample_width, midsample_height);

	// now draw again in bloom size
	R_Sampling_Quad(0, glState.height - sample_height, sample_width, sample_height, sampleText_tcw, sampleText_tch);

	// now blend the big screen texture into the bloom generation space (hoping it adds some blur)
	GL_Bind(GL_TEXTURE0, r_initsampletexture);
	R_Sampling_Quad(0, glState.height - sample_height, sample_width, sample_height, screenTex_tcw, screenTex_tch);
}

/*
=================
R_DownSampling
=================
*/
void R_DownSampling(void)
{
	//if ( (!r_downsample->value) && (cl.local.waterlevel < 3))
	if (!r_downsample->value)
		return;

	if (!DOWNSAMPLE_SIZE_X && !DOWNSAMPLE_SIZE_Y)
		R_Sampling_InitTextures();

	if (screen_texture_width < DOWNSAMPLE_SIZE_X || screen_texture_height < DOWNSAMPLE_SIZE_Y)
		return;

	// set up full screen workspace
	pglScissor(0, 0, glState.width, glState.height);
	pglViewport(0, 0, glState.width, glState.height);
	pglMatrixMode(GL_PROJECTION);
	pglLoadIdentity();

	pglOrtho(0, glState.width, glState.height, 0, -10, 100);

	pglMatrixMode(GL_MODELVIEW);
	pglLoadIdentity();

	pglDisable(GL_DEPTH_TEST);
	pglDisable(GL_ALPHA_TEST);
	pglDepthMask(GL_FALSE);
	pglDisable(GL_BLEND);

	GL_Cull(0);
	pglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// set up current sizes
	curView_x = RI.viewport[0];
	curView_y = RI.viewport[1];
	curView_width = RI.viewport[2];
	curView_height = RI.viewport[3];

	screenTex_tcw = ((float)curView_width / (float)screen_texture_width);
	screenTex_tch = ((float)curView_height / (float)screen_texture_height);
	if (curView_height > curView_width)
	{
		sampleText_tcw = ((float)curView_width / (float)curView_height);
		sampleText_tch = 1.0f;
	}
	else
	{
		sampleText_tcw = 1.0f;
		sampleText_tch = ((float)curView_height / (float)curView_width);
	}

	sample_width = (DOWNSAMPLE_SIZE_X * sampleText_tcw);
	sample_height = (DOWNSAMPLE_SIZE_Y * sampleText_tch);

	// copy the screen space we'll use to work into the backup texture
	GL_Bind(GL_TEXTURE0, r_samplebackuptexture);
	pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, r_screenbackuptexture_width * sampleText_tcw, r_screenbackuptexture_height * sampleText_tch);

	// create the bloom image
	R_Downsample_View();
	R_Sampling_GenerateTexture();

	pglDisable(GL_BLEND);
	// restore the screen-backup to the screen
	GL_Bind(GL_TEXTURE0, r_samplebackuptexture);


	pglScissor(RI.viewport[0], RI.viewport[1], RI.viewport[2], RI.viewport[3]);

	R_Sampling_DrawEffect();

	pglViewport(RI.viewport[0], RI.viewport[1], RI.viewport[2], RI.viewport[3]);

	pglMatrixMode(GL_PROJECTION);
	GL_LoadMatrix(RI.projectionMatrix);

	pglMatrixMode(GL_MODELVIEW);
	GL_LoadMatrix(RI.worldviewMatrix);

	pglEnable(GL_DEPTH_TEST);
	pglDepthMask(GL_TRUE);
	pglDisable(GL_BLEND);
	GL_Cull(GL_FRONT);
}