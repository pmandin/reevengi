/*
	2D drawing functions
	SBuffer renderer

	Copyright (C) 2008-2013	Patrice Mandin

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>
#include <assert.h>

#include "../video.h"
#include "../parameters.h"

#include "../r_common/render.h"
#include "../r_common/r_misc.h"

#include "dither.h"
#include "draw.h"
#include "draw_sbuffer.h"
#include "draw_sbuffer8.h"
#include "draw_sbuffer16.h"
#include "draw_sbuffer24.h"
#include "draw_sbuffer32.h"

/*--- Defines ---*/

/* Debug print info */
#if 0
#define DEBUG_PRINT(what) \
	{ \
		printf what; \
	}
#else
#define DEBUG_PRINT(what)
#endif

#define MAX_SEGMENTS 128
#define MAX_SPANS 256

#define SPAN_INVALID -1

#define SEG1_FRONT 0
#define SEG1_BEHIND 1
#define SEG1_CLIP_LEFT 2
#define SEG1_CLIP_RIGHT 3

/*--- Types ---*/

typedef struct {
	sbuffer_point_t sbp[2]; /* 0:min, 1:max */
} poly_hline_t;

typedef struct {
	Sint16 id;	/* Index in sbuffer_segment_t array for this row */
	Sint16 next;
	Uint16 x1,x2;	/* Start,end on the row */
} sbuffer_span_t;

typedef struct {
	Uint16 num_segs;
	Uint16 num_spans;
	Uint8 seg_full;
	Uint8 span_full;
	Sint16 first_span;
	sbuffer_segment_t segment[MAX_SEGMENTS];
	sbuffer_span_t span[MAX_SPANS];
} sbuffer_row_t;

typedef void (*sbuffer_draw_f)(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2);

/*--- Variables ---*/

/* for poly rendering */
static int size_poly_minmaxx = 0;
static poly_hline_t *poly_hlines = NULL;

/* Sbuffer */
static int sbuffer_numrows = 0;
static sbuffer_row_t *sbuffer_rows = NULL;

static sbuffer_draw_f draw_render_fill;
static sbuffer_draw_f draw_render_gouraud;
static sbuffer_draw_f draw_render_textured;

/*--- Functions prototypes ---*/

static void draw_shutdown(draw_t *this);

static void clear_sbuffer(void);
static void dump_sbuffer(void);
static void flush_sbuffer(draw_t *this);

static void draw_resize(draw_t *this, int w, int h, int bpp);
static void draw_startFrame(draw_t *this);
static void draw_endFrame(draw_t *this);

static void draw_poly_sbuffer(draw_t *this, vertexf_t *vtx, int num_vtx);
static void draw_poly_sbuffer_line(draw_t *this, vertexf_t *vtx, int num_vtx);
static void draw_mask_segment(draw_t *this, int y, int x1, int x2, float w);

/*--- Functions ---*/

void draw_init_sbuffer(draw_t *draw)
{
	draw_init(draw);

	draw->shutdown = draw_shutdown;

	draw->resize = draw_resize;
	draw->startFrame = draw_startFrame;
	draw->endFrame = draw_endFrame;

	draw->polyLine = draw_poly_sbuffer_line;
	draw->polyFill = draw_poly_sbuffer;
	draw->polyGouraud = draw_poly_sbuffer;
	draw->polyTexture = draw_poly_sbuffer;
	draw->addMaskSegment = draw_mask_segment;

	draw_render_fill = draw_render_fill8;
	draw_render_gouraud = draw_render_gouraud8;
	draw_render_textured = draw_render_textured8;

	clear_sbuffer();
}

static void draw_shutdown(draw_t *this)
{
	if (sbuffer_rows) {
		free(sbuffer_rows);
		sbuffer_rows = NULL;
	}
	sbuffer_numrows = 0;

	if (poly_hlines) {
		free(poly_hlines);
		poly_hlines = NULL;
	}
	size_poly_minmaxx = 0;
}

static void draw_resize(draw_t *this, int w, int h, int bpp)
{
	if (h>sbuffer_numrows) {
		sbuffer_rows = realloc(sbuffer_rows, h * sizeof(sbuffer_row_t));
		sbuffer_numrows = h;
	}

	if (!sbuffer_rows) {
		fprintf(stderr, "Not enough memory for Sbuffer rendering\n");
		return;
	}

	if (h>size_poly_minmaxx) {
		poly_hlines = realloc(poly_hlines, h * sizeof(poly_hline_t));
		size_poly_minmaxx = h;
	}

	if (!poly_hlines) {
		fprintf(stderr, "Not enough memory for poly rendering\n");
		return;
	}

	switch(bpp) {
		case 15:
		case 16:
			draw_render_fill = draw_render_fill16;
			draw_render_gouraud = draw_render_gouraud16;
			draw_render_textured = draw_render_textured16;
			break;
		case 24:
			draw_render_fill = draw_render_fill24;
			draw_render_gouraud = draw_render_gouraud24;
			draw_render_textured = draw_render_textured24;
			break;
		case 32:
			draw_render_fill = draw_render_fill32;
			draw_render_gouraud = draw_render_gouraud32;
			draw_render_textured = draw_render_textured32;
			break;
		default:
		case 8:
			draw_render_fill = draw_render_fill8;
			draw_render_gouraud = draw_render_gouraud8;
			draw_render_textured = draw_render_textured8;
			break;
	}
}

static void draw_startFrame(draw_t *this)
{
	clear_sbuffer();
}

static void draw_endFrame(draw_t *this)
{
	/*dump_sbuffer();*/
	flush_sbuffer(this);
}

static void dump_sbuffer(void)
{
	int i,j;

	printf("----------dump sbuffer start\n");

	for (i=0; i<sbuffer_numrows; i++) {
		sbuffer_row_t *row = &sbuffer_rows[i];
		if ((row->num_segs==0) && (row->num_spans==0))
			continue;

		printf("--- first span: %d\n", row->first_span);

		for (j=0; j<row->num_segs; j++) {
			printf("----- seg %d: %d,%d\n", j,
				row->segment[j].start.x, row->segment[j].end.x);
		}
		for (j=0; j<row->num_spans; j++) {
			printf("----- span %d (seg %d next %d): %d,%d\n", j,
				row->span[j].id, row->span[j].next,
				row->span[j].x1, row->span[j].x2);
		}
	}

	printf("----------dump sbuffer end\n");
}

static void clear_sbuffer(void)
{
	int i;

	DEBUG_PRINT(("----------clearing sbuffer\n"));

	for (i=0; i<sbuffer_numrows; i++) {
		sbuffer_rows[i].num_segs =
			sbuffer_rows[i].num_spans =
			sbuffer_rows[i].seg_full =
			sbuffer_rows[i].span_full = 0;
		sbuffer_rows[i].first_span = SPAN_INVALID;
	}
}

static void flush_sbuffer(draw_t *this)
{
	SDL_Surface *surf = video.screen;
	int i,j;
	Uint8 *dst = (Uint8 *) surf->pixels;

	if (SDL_MUSTLOCK(surf)) {
		SDL_LockSurface(surf);
	}

	dst += video.viewport.y * surf->pitch;
	dst += video.viewport.x * surf->format->BytesPerPixel;

	/* For each row */
	for (i=0; i<sbuffer_numrows; i++, dst += surf->pitch) {
		sbuffer_row_t *row = &sbuffer_rows[i];
		sbuffer_span_t *span = row->span;
		sbuffer_segment_t *segments = row->segment;

		if ((row->num_segs==0) || (row->num_spans==0)) {
			continue;
		}

		DEBUG_PRINT(("row %d: %d segs, %d spans\n",i,row->num_segs,row->num_spans));
#if 0
		if (row->seg_full) {
			fprintf(stderr,"Not enough segments for row %d\n",i);
		}
		if (row->span_full) {
			fprintf(stderr,"Not enough spans for row %d\n",i);
		}
#endif

		/* Render list of segment */
		for (j=row->first_span; j!=SPAN_INVALID; j=span[j].next) {
			int last, next_id;
			sbuffer_segment_t *current = &segments[span[j].id];

			/* Find last segment to merge */
			last = j;
			while (span[j].id==span[last].id) {
				next_id = span[last].next;
				if (next_id == SPAN_INVALID)
					break;
				if (span[next_id].x1-span[last].x2 != 1)
					break;
				if (span[next_id].id!=span[last].id)
					break;
				last = next_id;
			}

			DEBUG_PRINT(("final merge %d to %d (seg %d, %d->%d)\n",j,last, span[j].id, span[j].x1, span[last].x2));

			assert(span[j].x1 <= span[last].x2);

			if (!current->masking) {
				Uint8 *dst_line = dst + span[j].x1 * surf->format->BytesPerPixel;
				switch(current->render_mode) {
					case RENDER_WIREFRAME:
					case RENDER_FILLED:
						(*draw_render_fill)(surf, dst_line, current, span[j].x1, span[last].x2);
						break;
					case RENDER_GOURAUD:
						(*draw_render_gouraud)(surf, dst_line, current, span[j].x1, span[last].x2);
						break;
					case RENDER_TEXTURED:
						(*draw_render_textured)(surf, dst_line, current, span[j].x1, span[last].x2);
						break;
				}
			}

			j = last;
		}
	}

	if (SDL_MUSTLOCK(surf)) {
		SDL_UnlockSurface(surf);
	}

	/*clear_sbuffer();*/
}

/* Calc w coordinate for a given x */
static float calc_w(const sbuffer_segment_t *segment, int x)
{
	int dx,nx;
	float dw;

	dx = segment->end.x - segment->start.x + 1;
	nx = x - segment->start.x;

	dw = segment->end.w - segment->start.w;

	return (segment->start.w + ((dw * nx)/dx));
}

/* Check if a segment is in front or behind another */
static int check_behind(const sbuffer_segment_t *seg1, const sbuffer_segment_t *seg2,
	int x1, int x2, int *cx)
{
	float s1w1,s1w2, s2w1,s2w2, dw1,dw2;
	int dx;

	s1w1 = calc_w(seg1, x1);
	s1w2 = calc_w(seg1, x2);
	s2w1 = calc_w(seg2, x1);
	s2w2 = calc_w(seg2, x2);

	DEBUG_PRINT(("%d->%d: seg1: %p %.3f->%.3f, seg2: %p %.3f->%.3f\n",
		x1,x2, seg1,s1w1*4096.0f,s1w2*4096.0f, seg2,s2w1*4096.0f,s2w2*4096.0f));

	/* Do we have an intersection ? */
	if (s1w1>s2w1) {
		if (s1w2>s2w2) {
			return SEG1_FRONT;
		}
	} else {
		if (s1w2<s2w2) {
			return SEG1_BEHIND;
		}
	}

	/* Calc X coordinate of intersection where W is the same for both segments */
	dx = x2 - x1 + 1;
	dw1 = s1w2 - s1w1;
	dw2 = s2w2 - s2w1;

	if (dw2 - dw1 == 0.0f) {
		return SEG1_FRONT;
	}

	*cx = x1 + (((s1w1-s2w1)*dx)/(dw2-dw1));
	/*assert((*cx>=x1) && (*cx<=x2));*/

	if (*cx <= x1) {
		*cx = x1;
		return (s1w2>s2w2 ? SEG1_FRONT : SEG1_BEHIND);
	} else if (*cx >= x2) {
		*cx = x2;
		return (s1w1>s2w1 ? SEG1_FRONT : SEG1_BEHIND);
	}

	return (s1w1>s2w1 ? SEG1_CLIP_LEFT : SEG1_CLIP_RIGHT);
}

static void add_base_segment(int y, const sbuffer_segment_t *segment)
{
	sbuffer_row_t *row = &sbuffer_rows[y];
	sbuffer_segment_t *new_seg = &(row->segment[row->num_segs]);

	memcpy(new_seg, segment, sizeof(sbuffer_segment_t));

	++row->num_segs;

	row->seg_full |= (row->num_segs>=MAX_SEGMENTS);
}

static void write_first_span(int num_seg, sbuffer_row_t *row, int x1, int x2)
{
	sbuffer_span_t *new_span;

	new_span = &(row->span[0]);
	new_span->id = num_seg;
	new_span->next = SPAN_INVALID;
	new_span->x1 = x1;
	new_span->x2 = x2;

	row->first_span = 0;
	row->num_spans = 1;
}

static int insert_new_span(int num_seg, sbuffer_row_t *row, int x1, int x2,
	int psi /* prev span index*/, int nsi /* next span index */)
{
	sbuffer_span_t *new_span, *prev_span;

	if (psi==SPAN_INVALID) {
		/* New is now first */
		assert(row->first_span != SPAN_INVALID);
		assert(row->first_span == nsi);

		row->first_span = row->num_spans;
	} else {
		/* Merge with previous ? */
		prev_span = &(row->span[psi]);
		if ((prev_span->id==num_seg) && (prev_span->x2 - x1 == 1)) {
			prev_span->x2 = x2;
			return 0; /* no insertion of new span */
		}

		assert(prev_span->next == nsi);

		prev_span->next = row->num_spans;	/* Link previous to new */
	}

	assert(!(row->span_full));

	new_span = &(row->span[row->num_spans]);
	new_span->id = num_seg;
	new_span->next = nsi;	/* Link new to next */
	new_span->x1 = x1;
	new_span->x2 = x2;

	row->span_full |= ((++row->num_spans)>=MAX_SPANS);
	return 1;
}

static void overwrite_span(int num_seg, sbuffer_row_t *row, int x1, int x2, int csi /* current span index */)
{
	sbuffer_span_t *new_span;

	assert(csi<MAX_SPANS);

	new_span = &(row->span[csi]);
	new_span->id = num_seg;
	/*new_span->next = SPAN_INVALID;*/ /* Keep current value */
	new_span->x1 = x1;
	new_span->x2 = x2;
}

static int gen_seg_spans(int y, const sbuffer_segment_t *segment)
{
	sbuffer_row_t *row = &sbuffer_rows[y];
	int nx1,nx2, psi, nsi;
	int segbase_inserted = 0;
	int clip_seg, clip_pos;

	/* Still room for common segment data ? */
	if (row->seg_full || row->span_full) {
		return 0;
	}

	nx1 = segment->start.x;
	nx2 = segment->end.x;

	/* Clip if outside */
	if ((nx2<0) || (nx1>=video.viewport.w)) {
		return 0;
	}

	nx1 = MAX(0, nx1);
	nx2 = MIN(video.viewport.w-1, nx2);

	assert(nx1<=nx2);

	DEBUG_PRINT(("-------add segment %d,%d (seg %d span %d)\n", nx1,nx2,
		row->num_segs, row->num_spans));

	/*--- Trivial cases ---*/

	/* Empty row ? */
	if (row->num_segs == 0) {
		DEBUG_PRINT(("----empty list\n"));
		write_first_span(row->num_segs,row, nx1,nx2);
		return 1;
	}

	/*--- Need to check against current list ---*/
	psi = SPAN_INVALID;
	
	for (nsi=row->first_span ; (nsi!=SPAN_INVALID) && (nx1<=nx2); psi=nsi, nsi=row->span[nsi].next) {
		int clip_x1, clip_x2, span_inserted;
		sbuffer_span_t *current = &(row->span[nsi]);
		int cx1 = current->x1;
		int cx2 = current->x2;

		DEBUG_PRINT(("--new %d,%d against %d:%d,%d\n",nx1,nx2, nsi,cx1, cx2));

		/* Start after current? Will process against next one
		ccccccc
			nnnnn
		*/
		if (nx1>cx2) {
			DEBUG_PRINT((" P1: new start after current %d\n",nsi));
			continue;
		}

		/* Finish before current ? Insert before it
			ccccc
		nnnnnn
		  nnnnnnn
		*/
		if (nx2<cx1) {
			DEBUG_PRINT((" P2: new finishes before current %d\n",nsi));
			return insert_new_span(row->num_segs,row, nx1,nx2, psi,nsi);
		}

		/* Start before current, may finish after or in middle of it
		   Insert only non conflicting left part
			ccccccccc
		1 nnnnnnn
		2 nnnnnnnnnnn
		3 nnnnnnnnnnnnnnn
		4 nnnnnnnnnnnnnnnnn
		remains (to insert)
		1       n
		2       nnnnn
		3       nnnnnnnnn
		4       nnnnnnnnnnn
		*/
		if (nx1<cx1) {
			DEBUG_PRINT((" P3: new starts before current %d: insert %d,%d, will continue from pos %d\n", nsi, nx1,cx1-1, cx1));

			span_inserted = insert_new_span(row->num_segs,row, nx1,cx1-1, psi,nsi);
			segbase_inserted |= span_inserted;

			if (row->span_full)
				return segbase_inserted;

			nx1 = cx1;

			if (span_inserted) {
				/* Update previous with new inserted */
				psi = (	psi==SPAN_INVALID ?
					row->first_span :
					row->span[psi].next );
			}
		}

		/* Z check for multiple pixels
			ccccccccc
		1       nnnnn
		2	   nnnnn
		3	    nnnnnnnnnn
		4	        nnnnnn
		*/
		clip_x1 = MAX(nx1, cx1);
		clip_x2 = MIN(nx2, cx2);

		DEBUG_PRINT((" P4: solve conflict between new and current for pos %d to %d\n", clip_x1, clip_x2));

		clip_seg = check_behind(&(row->segment[current->id]),segment, clip_x1,clip_x2, &clip_pos);

		if ((cx1<clip_x1) && (clip_seg!=SEG1_FRONT)) {
			/* We have something like
			    ccccccccccccc
			    111111nnnn222 -> split current between 11111 and nnn222 */
			DEBUG_PRINT(("  split current %d, between %d to %d, %d to %d\n", nsi, cx1,clip_x1-1, clip_x1,cx2));

			insert_new_span(current->id,row, cx1,clip_x1-1, psi,nsi);
			current->x1 = cx1 = clip_x1;

			if (row->span_full)
				return segbase_inserted;

			/* Update previous with new inserted */
			psi = (	psi==SPAN_INVALID ?
				row->first_span :
				row->span[psi].next );
		}

		switch(clip_seg) {
			case SEG1_FRONT:
				DEBUG_PRINT(("  P4.0: current %d in front of new\n", nsi));

				break;
			case SEG1_BEHIND:
				DEBUG_PRINT(("  P4.1: current %d behind new\n", nsi));

				/* We have something like
				    ccccccccccccc	ccccccccccccc
				    nnnnnnn222222	nnnnnnnnnnnnn */

				if (clip_x2<cx2) {
					current->x1 = cx1 = clip_x2+1;
					DEBUG_PRINT(("   current %d reduced, from %d to %d\n", nsi, cx1,cx2));

					DEBUG_PRINT(("   insert common part of new, from %d to %d\n", clip_x1,clip_x2));
					span_inserted = insert_new_span(row->num_segs,row, clip_x1,clip_x2, psi, nsi);

					if (span_inserted) {
						/* Update previous with new inserted */
						psi = (	psi==SPAN_INVALID ?
							row->first_span :
							row->span[psi].next );
					}
				} else {
					/* current completely overwritten by new */
					DEBUG_PRINT(("   replace current %d by new, from %d to %d\n", nsi, clip_x1,clip_x2));
					overwrite_span(row->num_segs,row, clip_x1,clip_x2, nsi);
				}

				break;
			case SEG1_CLIP_LEFT:
				DEBUG_PRINT(("  P4.3: keep left part of current %d against new till pos %d\n", nsi, clip_pos));

				/* We have something like this to do
				    cccccccc -> cccccccc
				    nnnnn222	CCnnn222 */

				/* Insert right part of current, after common zone */
				if (clip_x2<cx2) {
					DEBUG_PRINT(("  split current %d, from %d to %d\n", nsi, clip_x2+1,cx2));
					/*span_inserted =*/ insert_new_span(current->id,row, clip_x2+1,cx2, nsi, row->span[nsi].next);
				}

				/* Insert new */
				DEBUG_PRINT(("  insert new from %d to %d\n", clip_pos,clip_x2));
				/*span_inserted =*/ insert_new_span(row->num_segs,row, clip_pos,clip_x2, nsi, row->span[nsi].next);

				/* Clip current before clip_pos */
				DEBUG_PRINT(("  clip current %d from %d to %d\n", nsi, cx1,clip_pos-1));
				current->x2 = clip_pos-1;

				break;
			case SEG1_CLIP_RIGHT:
				DEBUG_PRINT(("  P4.4: keep right of current %d against new from pos %d\n", nsi, clip_pos));

				/* We have something like this to do
				    cccccccc -> cccccccc
				    nnnnn222	nnCCC222 */

				DEBUG_PRINT(("  clip current %d from %d to %d\n", nsi, clip_pos+1,cx2));
				current->x1 = clip_pos+1;

				/* Insert new */
				DEBUG_PRINT(("  insert new from %d to %d\n", clip_x1,clip_pos));
				span_inserted = insert_new_span(row->num_segs,row, clip_x1,clip_pos, psi, nsi);

				if (span_inserted) {
					/* Update previous with new inserted */
					psi = (	psi==SPAN_INVALID ?
						row->first_span :
						row->span[psi].next );
				}

				break;
		}

		/* Continue with remaining part */
		nx1 = clip_x2+1;

		if (clip_seg!=SEG1_FRONT) {
			segbase_inserted=1;
		}
	}

	DEBUG_PRINT(("--remain %d,%d\n",nx1,nx2));
	if (nx1<=nx2) {
		/* Insert last */
		insert_new_span(row->num_segs,row, nx1,nx2, psi, nsi);
		segbase_inserted=1;
	}

	return segbase_inserted;
}

static void draw_poly_sbuffer(draw_t *this, vertexf_t *vtx, int num_vtx)
{
	int miny = video.viewport.h-1, maxy = 0;
	int minx = video.viewport.w-1, maxx = 0;
	int y, p1, p2;
	sbuffer_segment_t segment;
	int num_array = 1; /* max array */

	/* Fill poly min/max array with segments */
	p1 = num_vtx-1;
	for (p2=0; p2<num_vtx; p2++) {
		int v1 = p1;
		int v2 = p2;
		int x1,y1, x2,y2;
		int dy;
		float w1, w2;

		num_array = 1; /* max */

		x1 = vtx[p1].pos[0] / vtx[p1].pos[3];
		y1 = vtx[p1].pos[1] / vtx[p1].pos[3];
		w1 = (vtx[p1].pos[2]==0.0f ? 1.0f : vtx[p1].pos[3] / vtx[p1].pos[2]);

		x2 = vtx[p2].pos[0] / vtx[p2].pos[3];
		y2 = vtx[p2].pos[1] / vtx[p2].pos[3];
		w2 = (vtx[p2].pos[2]==0.0f ? 1.0f : vtx[p2].pos[3] / vtx[p2].pos[2]);

		/* Swap if p1 lower than p2 */
		if (y1 > y2) {
			int tmp;
			float tmpz;
			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			tmpz = w1; w1 = w2; w2 = tmpz;
			num_array = 0;	/* min */
			v1 = p2;
			v2 = p1;
		}

		miny = MIN(y1, miny);
		maxy = MAX(y2, maxy);

		dy = y2 - y1;
		if (dy>0) {
			int dx = x2 - x1;
			float r1 = vtx[v1].col[0];
			float dr = vtx[v2].col[0] - r1;
			float g1 = vtx[v1].col[1];
			float dg = vtx[v2].col[1] - g1;
			float b1 = vtx[v1].col[2];
			float db = vtx[v2].col[2] - b1;
			float tu1 = vtx[v1].tx[0];
			float du = vtx[v2].tx[0] - tu1;
			float tv1 = vtx[v1].tx[1];
			float dv = vtx[v2].tx[1] - tv1;
			float dw = w2 - w1;
			if (draw.correctPerspective>0) {
				r1 *= w1;
				dr = vtx[v2].col[0]*w2 - r1;
				g1 *= w1;
				dg = vtx[v2].col[1]*w2 - g1;
				b1 *= w1;
				db = vtx[v2].col[2]*w2 - b1;

				tu1 *= w1;
				du = vtx[v2].tx[0]*w2 - tu1;
				tv1 *= w1;
				dv = vtx[v2].tx[1]*w2 - tv1;
			}
			for (y=0; y<=dy; y++) {
				float coef_dy;

				if ((y1<0) || (y1>=video.viewport.h)) {
					continue;
				}

				coef_dy = (float) y / dy;
				poly_hlines[y1].sbp[num_array].r = r1 + (dr * coef_dy);
				poly_hlines[y1].sbp[num_array].g = g1 + (dg * coef_dy);
				poly_hlines[y1].sbp[num_array].b = b1 + (db * coef_dy);
				poly_hlines[y1].sbp[num_array].u = tu1 + (du * coef_dy);
				poly_hlines[y1].sbp[num_array].v = tv1 + (dv * coef_dy);
				poly_hlines[y1].sbp[num_array].w = w1 + (dw * coef_dy);
				poly_hlines[y1++].sbp[num_array].x = x1 + (dx * coef_dy);
			}
		}

		p1 = p2;
	}

	/* Render horizontal lines */
	miny=MAX(miny, 0);
	maxy=MIN(maxy, video.viewport.h-1);

	/* Copy to other array for a single segment */
	if (num_vtx==2) {
		for (y=miny; y<maxy; y++) {
			poly_hlines[y].sbp[num_array ^ 1] = poly_hlines[y].sbp[num_array];
		}
	}

	segment.render_mode = render.render_mode;
	segment.tex_num_pal = render.tex_pal;
	segment.texture = render.texture;
	segment.masking = render.bitmap.masking;

	for (y=miny; y<=maxy; y++) {
		int pminx = poly_hlines[y].sbp[0].x;
		int pmaxx = poly_hlines[y].sbp[1].x;

		if (pminx>pmaxx) {
			continue;
		}

		minx=MIN(minx, pminx);
		maxx=MAX(maxx, pmaxx);

		segment.start = poly_hlines[y].sbp[0];
		segment.end = poly_hlines[y].sbp[1];

		if (gen_seg_spans(y, &segment)) {
			add_base_segment(y, &segment);
		}
	}

	minx=MAX(minx, 0);
	maxx=MIN(maxx, video.viewport.w-1);

	/* Mark dirty rectangle */
	video.dirty_rects[video.numfb]->setDirty(video.dirty_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
	video.upload_rects[video.numfb]->setDirty(video.upload_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
}

/* Specific version for non filled polys */
static void draw_poly_sbuffer_line(draw_t *this, vertexf_t *vtx, int num_vtx)
{
	int miny = video.viewport.h-1, maxy = 0;
	int minx = video.viewport.w-1, maxx = 0;
	int p1,p2;
	sbuffer_segment_t segment;
	sbuffer_point_t *sp1, *sp2;

	segment.render_mode = render.render_mode;
	segment.tex_num_pal = render.tex_pal;
	segment.texture = render.texture;
	segment.masking = render.bitmap.masking;

	p1 = num_vtx-1;
	for (p2=0; p2<num_vtx; p2++) {
		int v1 = p1;
		int v2 = p2;
		int x1,y1, x2,y2;
		int dy, dx;
		float w1, w2;
		float r1,g1,b1,dr,dg,db;
		float tu1,du,tv1,dv,dw;

		/* Draw each line between vertices p1 and p2 */
		x1 = vtx[p1].pos[0] / vtx[p1].pos[3];
		y1 = vtx[p1].pos[1] / vtx[p1].pos[3];
		w1 = (vtx[p1].pos[2]==0.0f ? 1.0f : vtx[p1].pos[3] / vtx[p1].pos[2]);

		x2 = vtx[p2].pos[0] / vtx[p2].pos[3];
		y2 = vtx[p2].pos[1] / vtx[p2].pos[3];
		w2 = (vtx[p2].pos[2]==0.0f ? 1.0f : vtx[p2].pos[3] / vtx[p2].pos[2]);

		/* Swap if p1 lower than p2 */
		if (y1 > y2) {
			int tmp;
			float tmpz;

			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			tmpz = w1; w1 = w2; w2 = tmpz;
			v1 = p2;
			v2 = p1;
		}

		miny = MIN(y1, miny);
		maxy = MAX(y2, maxy);
		if (x1<=x2) {
			minx = MIN(x1, minx);
			maxx = MAX(x2, maxx);
		} else {
			minx = MIN(x2, minx);
			maxx = MAX(x2, maxx);
		}

		dy = y2-y1;
		dx = x2-x1;

		dx = x2 - x1;
		r1 = vtx[v1].col[0];
		dr = vtx[v2].col[0] - r1;
		g1 = vtx[v1].col[1];
		dg = vtx[v2].col[1] - g1;
		b1 = vtx[v1].col[2];
		db = vtx[v2].col[2] - b1;
		tu1 = vtx[v1].tx[0];
		du = vtx[v2].tx[0] - tu1;
		tv1 = vtx[v1].tx[1];
		dv = vtx[v2].tx[1] - tv1;
		dw = w2 - w1;
		if (draw.correctPerspective>0) {
			r1 *= w1;
			dr = vtx[v2].col[0]*w2 - r1;
			g1 *= w1;
			dg = vtx[v2].col[1]*w2 - g1;
			b1 *= w1;
			db = vtx[v2].col[2]*w2 - b1;

			tu1 *= w1;
			du = vtx[v2].tx[0]*w2 - tu1;
			tv1 *= w1;
			dv = vtx[v2].tx[1]*w2 - tv1;
		}

		if (dy==0) {
			/* Horizontal line */
			sp1 = (x1<x2 ? &segment.start : &segment.end);
			sp2 = (x1<x2 ? &segment.end : &segment.start);

			sp1->r = vtx[v1].col[0];
			sp1->g = vtx[v1].col[0];
			sp1->b = vtx[v1].col[0];
			sp1->u = vtx[v1].tx[0];
			sp1->v = vtx[v1].tx[1];
			sp1->w = w1;
			sp1->x = x1;

			sp2->r = vtx[v2].col[0];
			sp2->g = vtx[v2].col[1];
			sp2->b = vtx[v2].col[2];
			sp2->u = vtx[v2].tx[0];
			sp2->v = vtx[v2].tx[1];
			sp2->w = w2;
			sp2->x = x2;

			if ((y1>=0) && (y1<video.viewport.h)) {
				if (gen_seg_spans(y1, &segment)) {
					add_base_segment(y1, &segment);
				}
			}
		} else if (dy>=abs(dx)) {
			/* Mostly vertical */
			float coef_dy;
			int y;

			sp1 = &segment.start;
			sp2 = &segment.end;

			for (y=0; y<=dy; y++,y1++) {
				if ((y1<0) || (y1>=video.viewport.h)) {
					continue;
				}

				coef_dy = (float) y / dy;
				sp1->r = sp2->r = r1 + (dr * coef_dy);
				sp1->g = sp2->g = g1 + (dg * coef_dy);
				sp1->b = sp2->b = b1 + (db * coef_dy);
				sp1->u = sp2->u = tu1 + (du * coef_dy);
				sp1->v = sp2->v = tv1 + (dv * coef_dy);
				sp1->w = sp2->w = w1 + (dw * coef_dy);
				sp1->x = sp2->x = x1 + (dx * coef_dy);

				if (gen_seg_spans(y1, &segment)) {
					add_base_segment(y1, &segment);
				}
			}
		} else {
			/* Mostly horizontal */
		}

		p1 = p2;
	}

#if 0
	int miny = video.viewport.h-1, maxy = 0;
	int minx = video.viewport.w-1, maxx = 0;
	int y, p1, p2, prevx1, prevx2;
	sbuffer_segment_t segment;
	int num_array = 1; /* max array */

	/* Fill poly min/max array with segments */
	p1 = num_vtx-1;
	for (p2=0; p2<num_vtx; p2++) {
		int v1 = p1;
		int v2 = p2;
		int x1,y1, x2,y2;
		int dy, dx;
		float w1, w2;
		float r1,g1,b1,dr,dg,db;
		float tu1,du,tv1,dv,dw;

		num_array = 1; /* max */

		x1 = vtx[p1].pos[0] / vtx[p1].pos[3];
		y1 = vtx[p1].pos[1] / vtx[p1].pos[3];
		w1 = (vtx[p1].pos[2]==0.0f ? 1.0f : vtx[p1].pos[3] / vtx[p1].pos[2]);

		x2 = vtx[p2].pos[0] / vtx[p2].pos[3];
		y2 = vtx[p2].pos[1] / vtx[p2].pos[3];
		w2 = (vtx[p2].pos[2]==0.0f ? 1.0f : vtx[p2].pos[3] / vtx[p2].pos[2]);

		/*printf("%d,%d (%.3f) -> %d,%d (%.3f)\n",
			x1,y1,w1, x2,y2,w2);*/

		/* Swap if p1 lower than p2 */
		if (y1 > y2) {
			int tmp;
			float tmpz;
			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			tmpz = w1; w1 = w2; w2 = tmpz;
			num_array = 0;	/* min */
			v1 = p2;
			v2 = p1;
		}

		miny = MIN(y1, miny);
		maxy = MAX(y2, maxy);


		/*DEBUG_PRINT(("from p[%d]: u=%.3f, v=%.3f to p[%d]: u=%.3f,v=%.3f\n",
			v1, vtx[v1].tx[0], vtx[v1].tx[1],
			v2, vtx[v2].tx[0], vtx[v2].tx[1]
		));*/

		dx = x2 - x1;
		r1 = vtx[v1].col[0];
		dr = vtx[v2].col[0] - r1;
		g1 = vtx[v1].col[1];
		dg = vtx[v2].col[1] - g1;
		b1 = vtx[v1].col[2];
		db = vtx[v2].col[2] - b1;
		tu1 = vtx[v1].tx[0];
		du = vtx[v2].tx[0] - tu1;
		tv1 = vtx[v1].tx[1];
		dv = vtx[v2].tx[1] - tv1;
		dw = w2 - w1;
		if (draw.correctPerspective>0) {
			r1 *= w1;
			dr = vtx[v2].col[0]*w2 - r1;
			g1 *= w1;
			dg = vtx[v2].col[1]*w2 - g1;
			b1 *= w1;
			db = vtx[v2].col[2]*w2 - b1;

			tu1 *= w1;
			du = vtx[v2].tx[0]*w2 - tu1;
			tv1 *= w1;
			dv = vtx[v2].tx[1]*w2 - tv1;
		}

		dy = y2 - y1;
		if (dy>0) {
			for (y=0; y<=dy; y++) {
				float coef_dy;

				if ((y1<0) || (y1>=video.viewport.h)) {
					continue;
				}

				coef_dy = (float) y / dy;
				poly_hlines[y1].sbp[num_array].r = r1 + (dr * coef_dy);
				poly_hlines[y1].sbp[num_array].g = g1 + (dg * coef_dy);
				poly_hlines[y1].sbp[num_array].b = b1 + (db * coef_dy);
				poly_hlines[y1].sbp[num_array].u = tu1 + (du * coef_dy);
				poly_hlines[y1].sbp[num_array].v = tv1 + (dv * coef_dy);
				poly_hlines[y1].sbp[num_array].w = w1 + (dw * coef_dy);
				poly_hlines[y1++].sbp[num_array].x = x1 + (dx * coef_dy);
			}
		} else {
			/* Horizontal line */
			if ((y1>=0) && (y1<video.viewport.h)) {
				num_array = (x2<x1);

				poly_hlines[y1].sbp[num_array].r = r1;
				poly_hlines[y1].sbp[num_array].g = g1;
				poly_hlines[y1].sbp[num_array].b = b1;
				poly_hlines[y1].sbp[num_array].u = tu1;
				poly_hlines[y1].sbp[num_array].v = tv1;
				poly_hlines[y1].sbp[num_array].w = w1;
				poly_hlines[y1].sbp[num_array].x = x1;

				poly_hlines[y1].sbp[num_array ^ 1].r = r1 + dr;
				poly_hlines[y1].sbp[num_array ^ 1].g = g1 + dg;
				poly_hlines[y1].sbp[num_array ^ 1].b = b1 + db;
				poly_hlines[y1].sbp[num_array ^ 1].u = tu1 + du;
				poly_hlines[y1].sbp[num_array ^ 1].v = tv1 + dv;
				poly_hlines[y1].sbp[num_array ^ 1].w = w1 + dw;
				poly_hlines[y1].sbp[num_array ^ 1].x = x2;
			}
		}

		p1 = p2;
	}

	/* Render horizontal lines */
	miny=MAX(miny, 0);
	maxy=MIN(maxy, video.viewport.h-1);

	/* Copy to other array for a single segment */
	if (num_vtx==2) {
		for (y=miny; y<maxy; y++) {
			poly_hlines[y].sbp[num_array ^ 1] = poly_hlines[y].sbp[num_array];
		}
	}

	prevx1 = poly_hlines[miny].sbp[0].x;
	prevx2 = poly_hlines[miny].sbp[1].x;

	segment.render_mode = render.render_mode;
	segment.tex_num_pal = render.tex_pal;
	segment.texture = render.texture;
	segment.masking = render.bitmap.masking;

	for (y=miny; y<=maxy; y++) {
		int pminx = poly_hlines[y].sbp[0].x;
		int pmaxx = poly_hlines[y].sbp[1].x;
		int add_seg = 0;

		if (pminx>pmaxx) {
			continue;
		}

		minx=MIN(minx, pminx);
		maxx=MAX(maxx, pmaxx);

		if (miny==maxy) {
			/* Horizontal segment */
			segment.start = poly_hlines[y].sbp[0];
			segment.end = poly_hlines[y].sbp[1];

			add_seg = gen_seg_spans(y, &segment);
		} else {
			segment.start = poly_hlines[y].sbp[0];
			segment.end = poly_hlines[y].sbp[0];
			if (prevx1<pminx) {
				segment.start.x = prevx1+1;
			} else if (prevx1>pminx) {
				segment.end.x = prevx1-1;
			}

			add_seg = gen_seg_spans(y, &segment);

			segment.start = poly_hlines[y].sbp[1];
			segment.end = poly_hlines[y].sbp[1];
			if (prevx2<pmaxx) {
				segment.start.x = prevx2+1;
			} else if (prevx2>pmaxx) {
				segment.end.x = prevx2-1;
			}

			add_seg |= gen_seg_spans(y, &segment);
		}

		if (add_seg) {
			add_base_segment(y, &segment);
		}

		prevx1 = pminx;
		prevx2 = pmaxx;
	}

	/*dump_sbuffer();*/
#endif

	minx=MAX(minx, 0);
	maxx=MIN(maxx, video.viewport.w-1);

	/* Mark dirty rectangle */
	video.dirty_rects[video.numfb]->setDirty(video.dirty_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
	video.upload_rects[video.numfb]->setDirty(video.upload_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
}

static void draw_mask_segment(draw_t *this, int y, int x1, int x2, float w)
{
	sbuffer_segment_t segment;

	if ((y<0) || (y>=video.viewport.h)) {
		return;
	}
	if ((x1>=video.viewport.w) || (x2<0)) {
		return;
	}

	segment.start.x = x1;
	segment.end.x = x2;
	segment.start.w = segment.end.w = w;
	segment.masking = 1;

	if (gen_seg_spans(y, &segment)) {
		add_base_segment(y, &segment);
	}

	/* Upper layer will update dirty rectangles */
}
