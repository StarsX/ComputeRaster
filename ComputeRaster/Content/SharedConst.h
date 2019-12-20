//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#define	FRAME_COUNT	3

#define GRID_SIZE	64
#define SHOW_MIP	0

#define USING_SRV	0

#define	USE_NORMAL	1

#define	USE_MUTEX	0

#if	USE_NORMAL
#define	DEPTH_SCALE	0.25
#else
#define	DEPTH_SCALE	0.5
#endif

#define CLEAR_COLOR	0.0f, 0.2f, 0.4f

#define	PIDIV4		0.785398163f

static const float g_FOVAngleY = PIDIV4;
static const float g_zNear = 1.0f;
static const float g_zFar = 1000.0f;
