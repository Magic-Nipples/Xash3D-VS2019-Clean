//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SVD_RENDER_H
#define SVD_RENDER_H

#include "ref_params.h"

extern void SVD_VidInit( void );
extern void SVD_Shutdown( void );

extern void SVD_CalcRefDef( ref_params_t* pparams );
extern void SVD_DrawTransparentTriangles( void );
#endif