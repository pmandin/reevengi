/*
	Render engine

	Copyright (C) 2008	Patrice Mandin

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef RENDER_H
#define RENDER_H 1

#include "render_bitmap.h"
#include "render_mask.h"
#include "render_skel.h"
#include "render_mesh.h"
#include "render_texture.h"

/*--- Defines ---*/

enum {
	RENDER_WIREFRAME=0,
	RENDER_FILLED,
	RENDER_GOURAUD,
	RENDER_TEXTURED
};

enum {
	NO_PERSCORR=0,		/* No perspective correction */
	PERSCORR_LINE=1,	/* Perspective correction per line */
	PERSCORR_P16=2,		/* Perspective correction per 16-pixels group */
	PERSCORR_PIX=3		/* Perspective correction per pixel */
};

#define TRI_LIST_SIZE 16

#define RENDER_Z_NEAR	16.0f
#define RENDER_Z_FAR	(RENDER_Z_NEAR+65536.0f)

/*--- Types ---*/

typedef struct vertex_s vertex_t;

struct vertex_s {
	Sint16 x,y,z;	/* Vertex coords */
	Uint16 u,v;	/* Texture coords */
};

typedef struct vertexf_s vertexf_t;

struct vertexf_s {
	float pos[4];	/* x,y,z,w */
	float tx[2];	/* u,v */
	float col[4];	/* r,g,b,a */
};

typedef struct render_s render_t;

struct render_s {
	void (*shutdown)(void);

	void (*resize)(int w, int h, int bpp);
	void (*startFrame)(void);
	void (*endFrame)(void);

	render_texture_t *(*createTexture)(int flags);
	render_mesh_t *(*createMesh)(render_texture_t *texture);
	render_skel_t *(*createSkel)(void *emd_file, Uint32 emd_length, render_texture_t *texture);
	render_mask_t *(*createMask)(render_texture_t *texture);

	void (*set_viewport)(int x, int y, int w, int h);
	void (*set_projection)(float angle, float aspect,
		float z_near, float z_far);
	void (*set_ortho)(float left, float right, float bottom, float top,
		float p_near, float p_far);
	void (*set_modelview)(float x_from, float y_from, float z_from,
		float x_to, float y_to, float z_to,
		float x_up, float y_up, float z_up);
	void (*set_identity)(void);
	void (*scale)(float x, float y, float z);
	void (*translate)(float x, float y, float z);
	void (*rotate)(float angle, float x, float y, float z);
	void (*push_matrix)(void);
	void (*pop_matrix)(void);

	void (*get_proj_matrix)(float mtx[4][4]);
	void (*get_model_matrix)(float mtx[4][4]);
	void (*set_proj_matrix)(float mtx[4][4]);
	void (*set_model_matrix)(float mtx[4][4]);

	void (*set_color)(Uint32 color);	/* color in ARGB format */
	void (*set_render)(int num_render);
	void (*set_texture)(int num_pal, render_texture_t *render_tex);
	void (*set_blending)(int enable);
	void (*set_dithering)(int enable);
	void (*set_depth)(int enable);
	void (*set_useDirtyRects)(int enable);
	void (*set_pers_corr)(int perscorr);

	int render_mode;
	int dithering;
	int depth_test;
	int useDirtyRects;	/* For bitmap ops, only refresh dirty rects */

	/* Misc functions */
	void (*sortBackToFront)(int num_vtx, int *num_idx, vertex_t *vtx);

	/* Wireframe functions */
	void (*line)(vertex_t *v1, vertex_t *v2);
	void (*triangle_wf)(vertex_t *v1, vertex_t *v2, vertex_t *v3);
	void (*quad_wf)(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

	/* Model drawing functions */
	void (*triangle)(vertex_t *v1, vertex_t *v2, vertex_t *v3);
	void (*quad)(vertex_t *v1, vertex_t *v2, vertex_t *v3, vertex_t *v4);

	/* Bitmap functions */
	render_bitmap_t	bitmap;

	Uint32 color;	/* Active color */
	render_texture_t *texture;
	int tex_pal;	/* Palette to use */

	/* Display depth buffer */
	int render_depth;
	void (*setRenderDepth)(int show_depth);
	void (*copyDepthToColor)(void);
};

/*--- Variables ---*/

extern render_t render;

/*--- Functions ---*/

void render_init(render_t *this);

#endif /* RENDER_H */
