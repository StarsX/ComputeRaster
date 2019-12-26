//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#define	FRAME_COUNT	3

#define	USE_TRIPPLE_RASTER	1

#define TILE_SIZE_LOG	3
#define TILE_SIZE		(1 << TILE_SIZE_LOG)
#define TILE_TO_BIN_LOG	3
#define BIN_SIZE_LOG	(TILE_SIZE_LOG + TILE_TO_BIN_LOG)
#define BIN_SIZE		(1 << BIN_SIZE_LOG)

#define CLEAR_COLOR	0.0f, 0.2f, 0.4f

#define	PIDIV4		0.785398163f

static const float g_FOVAngleY = PIDIV4;
static const float g_zNear = 1.0f;
static const float g_zFar = 1000.0f;
