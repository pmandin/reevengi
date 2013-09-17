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

#define SEG1_FRONT 0
#define SEG1_BEHIND 1
#define SEG1_CLIP_LEFT 2
#define SEG1_CLIP_RIGHT 3

/*--- Types ---*/

typedef struct {
	sbuffer_point_t sbp[2]; /* 0:min, 1:max */
} poly_hline_t;

typedef struct {
	Uint16 id;	/* Index in sbuffer_segment_t array for this row */
	Uint16 dummy;
	Uint16 x1,x2;	/* Start,end on the row */
} sbuffer_span_t;

typedef struct {
	Uint16 num_segs;
	Uint16 num_spans;
	Uint8 seg_full;
	Uint8 span_full;
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
static Uint32 sbuffer_seg_id;

static sbuffer_draw_f draw_render_fill;
static sbuffer_draw_f draw_render_gouraud;
static sbuffer_draw_f draw_render_textured;

/*--- Functions prototypes ---*/

static void draw_shutdown(draw_t *this);

static void clear_sbuffer(void);

static void draw_resize(draw_t *this, int w, int h, int bpp);
static void draw_startFrame(draw_t *this);
static void draw_flushFrame(draw_t *this);
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
	draw->flushFrame = draw_flushFrame;
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

static void clear_sbuffer(void)
{
	int i;

	DEBUG_PRINT(("----------clearing sbuffer\n"));

	for (i=0; i<sbuffer_numrows; i++) {
		sbuffer_rows[i].num_segs = 0;
		sbuffer_rows[i].num_spans = 0;
		sbuffer_rows[i].seg_full = 0;
		sbuffer_rows[i].span_full = 0;
	}
	sbuffer_seg_id = 0;
}

static void draw_startFrame(draw_t *this)
{
	clear_sbuffer();
}

static void draw_flushFrame(draw_t *this)
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

#if 0
		if (row->seg_full) {
			fprintf(stderr,"Not enough segments for row %d\n",i);
		}
		if (row->span_full) {
			fprintf(stderr,"Not enough spans for row %d\n",i);
		}
#endif

		/* Render list of segment */
		for (j=0; j<row->num_spans; j++) {
			int last;
			sbuffer_segment_t *current = &segments[span[j].id];

			/* Find last segment to merge */
			for (last=j;
				(last<row->num_spans-1) && (span[j].id==span[last+1].id)
				&& (span[last+1].x1-span[last].x2 == 1);
				last++)
			{
			}

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

static void draw_endFrame(draw_t *this)
{
	draw_flushFrame(this);
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

	if (*cx == x1) {
		return (s1w2>s2w2 ? SEG1_FRONT : SEG1_BEHIND);
	} else if (*cx == x2) {
		return (s1w1>s2w1 ? SEG1_FRONT : SEG1_BEHIND);
	}

	return (s1w1>s2w1 ? SEG1_CLIP_LEFT : SEG1_CLIP_RIGHT);
}

#if 0
static void dump_sbuffer(void)
{
	int i,j;

	DEBUG_PRINT(("----------dump sbuffer start\n"));

	for (i=0; i<sbuffer_numrows; i++) {
		for (j=0; j<sbuffer_rows[i].num_segs; j++) {
			DEBUG_PRINT(("----- seg %d: %d,%d\n", i,
				sbuffer_rows[i].segment[j].start.x, sbuffer_rows[i].segment[j].end.x));
		}
		for (j=0; j<sbuffer_rows[i].num_spans; j++) {
			DEBUG_PRINT(("----- data %d (seg %d): %d,%d\n", i, sbuffer_rows[i].span[j].id,
				sbuffer_rows[i].span[j].x1, sbuffer_rows[i].span[j].x2));
		}
	}

	DEBUG_PRINT(("----------dump sbuffer end\n"));
}
#endif

static void add_base_segment(int y, const sbuffer_segment_t *segment)
{
	sbuffer_row_t *row = &sbuffer_rows[y];
	sbuffer_segment_t *new_seg = &(row->segment[row->num_segs]);

	memcpy(new_seg, segment, sizeof(sbuffer_segment_t));

	DEBUG_PRINT((">>base segment %d added\n", row->num_segs));

	++row->num_segs;

	row->seg_full |= (row->num_segs>=MAX_SEGMENTS);
}

static void add_base_segment_span(sbuffer_row_t *row, const sbuffer_segment_t *segment)
{
	sbuffer_segment_t *new_seg = &(row->segment[row->num_segs]);

	memcpy(new_seg, segment, sizeof(sbuffer_segment_t));

	DEBUG_PRINT((">>base segment %d added\n", row->num_segs));

	++row->num_segs;

	row->seg_full |= (row->num_segs>=MAX_SEGMENTS);
}

static void push_data_span(int num_seg, int num_span, sbuffer_row_t *row, int x1, int x2)
{
	sbuffer_span_t *new_span;

	if (num_span>=MAX_SPANS) {
		return;
	}

	assert(x1<=x2);

	/* Write new segment data */
	new_span = &(row->span[num_span]);

	new_span->id = num_seg;
	new_span->x1 = x1;
	new_span->x2 = x2;
}

static void push_data_segment(int num_seg, int num_span, int y, int x1, int x2)
{
	sbuffer_row_t *row = &sbuffer_rows[y];
	sbuffer_span_t *new_span;

	if (num_span>=MAX_SPANS) {
		return;
	}

	assert(x1<=x2);

	/* Write new segment data */
	new_span = &(row->span[num_span]);

	new_span->id = num_seg;
	new_span->x1 = x1;
	new_span->x2 = x2;
}

static void insert_data_span(int num_seg, int new_span, sbuffer_row_t *row, int x1, int x2)
{
	int num_spans = row->num_spans;

	/* Move stuff that starts after this segment */
	if ((num_spans>0) && (num_spans<MAX_SPANS-1)) {
		sbuffer_span_t *src = &(row->span[new_span]);
		sbuffer_span_t *dst = &(row->span[new_span+1]);
		memmove(dst, src, (num_spans-new_span)*sizeof(sbuffer_span_t));
	}

	push_data_span(num_seg, new_span, row, x1,x2);
	if (num_spans<MAX_SPANS) {
		++row->num_spans;

		row->span_full |= (row->num_spans>=MAX_SPANS);
	}
}

static void insert_data_segment(int num_seg, int new_span, int y, int x1, int x2)
{
	sbuffer_row_t *row = &sbuffer_rows[y];
	int num_spans = row->num_spans;

	/* Move stuff that starts after this segment */
	if ((num_spans>0) && (num_spans<MAX_SPANS-1)) {
		sbuffer_span_t *src = &(row->span[new_span]);
		sbuffer_span_t *dst = &(row->span[new_span+1]);
		memmove(dst, src, (num_spans-new_span)*sizeof(sbuffer_span_t));
	}

	push_data_segment(num_seg, new_span, y, x1,x2);
	if (num_spans<MAX_SPANS) {
		++row->num_spans;

		row->span_full |= (row->num_spans>=MAX_SPANS);
	}
}

static int gen_seg_spans(int y, const sbuffer_segment_t *segment)
{
	sbuffer_row_t *row = &sbuffer_rows[y];
	int nx1,nx2, i;
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

	if (nx1>nx2) {
		return 0;
	}

	DEBUG_PRINT(("-------add segment %d,%d (seg %d span %d)\n", nx1,nx2,
		row->num_segs, row->num_spans));

	/*--- Trivial cases ---*/

	/* Empty row ? */
	if (row->num_segs == 0) {
		DEBUG_PRINT(("----empty list\n"));
		insert_data_span(row->num_segs,0,row, nx1,nx2);
		return 1;
	}

	/*--- Need to check against current list ---*/
	for (i=0; (i<row->num_spans) && (nx1<=nx2); i++) {
		int clip_x1, clip_x2, ic = i;
		sbuffer_span_t *current = &(row->span[ic]);
		int cx1 = current->x1;
		int cx2 = current->x2;

		DEBUG_PRINT(("--new %d,%d against %d:%d,%d\n",nx1,nx2, ic,cx1, cx2));

		/* Start after current? Will process against next one
		ccccccc
			nnnnn
		*/
		if (nx1>cx2) {
			DEBUG_PRINT((" P1: new start after current %d\n",ic));
			continue;
		}

		/* Finish before current ? Insert before it
			ccccc
		nnnnnn
		  nnnnnnn
		*/
		if (nx2<cx1) {
			DEBUG_PRINT((" P2: new finishes before current %d\n",ic));
			insert_data_span(row->num_segs,ic,row, nx1,nx2);
			return 1;
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
			DEBUG_PRINT((" P3: new starts before current %d: insert %d,%d, will continue from pos %d\n", ic, nx1,cx1-1, cx1));

			insert_data_span(row->num_segs,ic,row, nx1,cx1-1);
			segbase_inserted = 1;

			/* End of list ? */
			if (++ic>=MAX_SPANS) {
				break;
			}

			nx1 = cx1;

			current = &(row->span[ic]);

			cx1 = current->x1;
			cx2 = current->x2;
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
			DEBUG_PRINT(("  split current %d, from %d to %d\n", ic, cx1, clip_x2-1));

			insert_data_span(current->id,ic,row, cx1,clip_x1-1);

			if (++ic>=MAX_SPANS) {
				break;
			}

			current = &(row->span[ic]);

			cx1 = current->x1 = clip_x1;
		}

		switch(clip_seg) {
			case SEG1_FRONT:
				DEBUG_PRINT(("  P4.0: current %d in front of new\n", ic));

				break;
			case SEG1_BEHIND:
				DEBUG_PRINT(("  P4.1: current %d behind new\n", ic));

				/* We have something like
				    ccccccccccccc	ccccccccccccc
				    nnnnnnn222222	nnnnnnnnnnnnn */

				if (clip_x2<cx2) {
					current->x1 = cx1 = clip_x2+1;
					DEBUG_PRINT(("   current %d reduced, from %d to %d\n", ic, cx1,cx2));

					DEBUG_PRINT(("   insert common part of new, from %d to %d\n", clip_x1,clip_x2));
					insert_data_span(row->num_segs,ic,row, clip_x1,clip_x2);
				} else {
					/* current completely overwritten by new */
					DEBUG_PRINT(("   replace current %d by new, from %d to %d\n", ic, clip_x1,clip_x2));
					push_data_span(row->num_segs,ic,row, clip_x1,clip_x2);
				}

				break;
			case SEG1_CLIP_LEFT:
				DEBUG_PRINT(("  P4.3: keep left part of current %d against new till pos %d\n", ic, clip_pos));

				/* We have something like this to do
				    cccccccc -> cccccccc
				    nnnnn222	CCnnn222 */

				/* Insert right part of current, after common zone */
				if (clip_x2<cx2) {
					DEBUG_PRINT(("  split current %d, from %d to %d\n", ic, clip_x2+1,cx2));
					insert_data_span(current->id,ic+1,row, clip_x2+1,cx2);
				}

				/* Insert new */
				DEBUG_PRINT(("  insert new from %d to %d\n", clip_pos,clip_x2));
				insert_data_span(row->num_segs,ic+1,row, clip_pos,clip_x2);

				/* Clip current before clip_pos */
				DEBUG_PRINT(("  clip current %d from %d to %d\n", ic, cx1,clip_pos-1));
				current->x2 = clip_pos-1;

				break;
			case SEG1_CLIP_RIGHT:
				DEBUG_PRINT(("  P4.4: keep right of current %d against new from pos %d\n", ic, clip_pos));

				/* We have something like this to do
				    cccccccc -> cccccccc
				    nnnnn222	nnCCC222 */

				DEBUG_PRINT(("  clip current %d from %d to %d\n", ic, clip_pos+1,cx2));
				current->x1 = clip_pos+1;

				/* Insert new */
				DEBUG_PRINT(("  insert new from %d to %d\n", clip_x1,clip_pos));
				insert_data_span(row->num_segs,ic,row, clip_x1,clip_pos);

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
		insert_data_span(row->num_segs,row->num_spans,row, nx1,nx2);
		segbase_inserted=1;
	}

	return segbase_inserted;
}

static int draw_add_segment(int y, const sbuffer_segment_t *segment)
{
	sbuffer_row_t *row = &sbuffer_rows[y];
	int x1,x2, i;
	int segbase_inserted = 0;
	int clip_seg, clip_pos;

	/* Still room for common segment data ? */
	if (row->seg_full || row->span_full) {
		return 0;
	}

	x1 = segment->start.x;
	x2 = segment->end.x;

	/* Clip if outside */
	if ((x2<0) || (x1>=video.viewport.w)) {
		return 0;
	}

	x1 = MAX(0, x1);
	x2 = MIN(video.viewport.w-1, x2);

	if (x2<x1) {
		return 0;
	}

	DEBUG_PRINT(("-------add segment %d %d,%d (seg %d span %d)\n", y, x1,x2,
		row->num_segs, row->num_spans));

	/*--- Trivial cases ---*/

	/* Empty row ? */
	if (row->num_segs == 0) {
		DEBUG_PRINT(("----empty list\n"));
		insert_data_span(row->num_segs,0,row, x1,x2);
		return 1;
	}

	/* Finish before first ? */
	if (x2 < row->span[0].x1) {
		DEBUG_PRINT(("----finish before first (%d<%d)\n",x2,row->span[0].x1));
		insert_data_span(row->num_segs,0,row, x1,x2);
		return 1;
	}

	/* Start after last ? */
	if (row->span[row->num_spans-1].x2 < x1) {
		DEBUG_PRINT(("----start after last (%d<%d)\n", row->span[row->num_spans-1].x2, x1));
		insert_data_span(row->num_segs,row->num_spans,row, x1,x2);
		return 1;
	}

	/*--- Need to check against current list ---*/
	for (i=0; (i<row->num_spans) && (x1<=x2); i++) {
		int clip_x1, clip_x2, current_end, ic = i;
		sbuffer_span_t *current = &(row->span[ic]);

		DEBUG_PRINT(("--new %d,%d against %d:%d,%d\n",x1,x2, ic,current->x1, current->x2));

		/* Start after current? Will process against next one
		ccccccc
			nnnnn
		*/
		if (current->x2 < x1) {
			DEBUG_PRINT(("  start after %d\n",ic));
			continue;
		}

		/* Finish before current ? Insert before it
			ccccc
		nnnnnn
		  nnnnnnn
		*/
		if (x2 <= current->x1) {
			DEBUG_PRINT(("  finish before or equal start %d\n",ic));
			if (current->x1 > 0) {
				if (x2 == current->x1) {
					--x2;
				}
				insert_data_span(row->num_segs,ic,row, x1,x2);
				segbase_inserted = 1;
			}
			goto label_insert_base;
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
		if (x1 < current->x1) {
			int next_x1 = current->x1;
			DEBUG_PRINT(("  new start before %d, insert %d,%d, will continue from pos %d\n", ic, x1,next_x1-1, next_x1));
			/*printf("   current before: %d,%d\n", current->start.x, current->end.x);**/

			insert_data_span(row->num_segs,ic,row, x1,next_x1-1);
			segbase_inserted = 1;

			++ic;

			x1 = next_x1;

			/* End of list ? */
			if (ic>=MAX_SPANS) {
				break;
			}

			current = &(row->span[ic]);
			/*printf("   current after: %d,%d\n", current->start.x, current->end.x);*/
		}

		/* Now Zcheck both current and new segment */

		/* Single pixel for current ?
			c
		1	n
		2	nnnnnn
		remains
		1
		2	 nnnnn
		*/
		if (current->x1 == current->x2) {
			/* Replace current with new if current behind */
			int cur_x = current->x1;
			int next_x1 = current->x2+1;
			DEBUG_PRINT(("  current is single pixel, will continue from pos %d\n", next_x1));
			/*printf("   new w=%.3f, cur w=%.3f\n", calc_w(start, end, x1), current->start.w);*/
			if (calc_w(segment, x1) > calc_w(&(row->segment[current->id]), cur_x)) {
				DEBUG_PRINT(("   replace current by new\n"));

				push_data_span(row->num_segs,ic,row, cur_x,cur_x);
				segbase_inserted=1;
			}
			x1 = next_x1;
			continue;
		}

		/* Single pixel for new ?
			cccccccccc
			    n
		*/
		if (x1 == x2) {
			DEBUG_PRINT((" new single pixel at %d\n", x1));

			/* Skip if we already inserted some part of it before */
			if (segbase_inserted) {
				DEBUG_PRINT(("  skip part already inserted\n"));
				goto label_insert_base;
			}

			/* Skip if new behind current */
			if (calc_w(&(row->segment[current->id]), x1) > calc_w(segment, x1)) {
				DEBUG_PRINT(("  new behind current, stop\n"));
				goto label_insert_base;
			}

			/*printf("  check %d:%d,%d against new %d\n", ic,current->start.x,current->end.x, x1);*/

			/* Insert new before current, clip current ?
				cccccccc
				n
			*/
			if (x1 == current->x1) {
				DEBUG_PRINT(("  clip current start from %d,%d at %d\n", current->x1,current->x2, x1+1));
				++current->x1;

				DEBUG_PRINT(("  insert new %d,%d at %d\n", x1,x2, ic));
				insert_data_span(row->num_segs,ic,row, x1,x2);
				return 1;
			}

			/* Clip current, insert new after current ?
				cccccccc
				       n
			*/
			if (x2 == current->x2) {
				DEBUG_PRINT(("  clip current end from %d,%d at %d\n", current->x1,current->x2, x2-1));
				--current->x2;

				DEBUG_PRINT(("  insert new %d,%d at %d\n", x1,x2, ic+1));
				insert_data_span(row->num_segs,ic+1,row, x1,x2);
				return 1;
			}

			/* Split current to insert new between both halves
				cccccccc
				   n
			becomes
				ccc
				   n
				    cccc
			*/
			DEBUG_PRINT(("  split current, insert new\n"));
			insert_data_span(current->id,ic+1,row, x1+1,current->x2);

			current->x2 = x1-1;

			insert_data_span(row->num_segs,ic+1,row, x1,x2);
			return 1;
		}

		/* Z check for multiple pixels
			ccccccccc
		1       nnnnn
		2	   nnnnn
		3	    nnnnnnnnnn
		4	        nnnnnn
		*/
		clip_x1 = MAX(x1, current->x1);
		clip_x2 = MIN(x2, current->x2);

		if (clip_x1==clip_x2) {
			DEBUG_PRINT((" Zcheck multiple pixels, single pixel common zone\n"));
			/* Skip if new behind current */
			if (calc_w(&(row->segment[current->id]), clip_x1) > calc_w(segment, clip_x1)) {
				DEBUG_PRINT(("  new behind current, continue from %d\n", clip_x1+1));
				x1 = clip_x1+1;
				continue;
			}

			/* Clip current if behind new */
			DEBUG_PRINT(("  clip current end from %d,%d at %d\n", current->x1,current->x2, clip_x1-1));
			current->x2 = clip_x1-1;
			x1 = clip_x1;
			continue;
		}

		clip_seg = check_behind(&(row->segment[current->id]),segment, clip_x1,clip_x2, &clip_pos);

		switch(clip_seg) {
			case SEG1_BEHIND:
				DEBUG_PRINT(("- %d behind new (common from %d->%d)\n", ic,clip_x1,clip_x2));
				current_end = current->x2;

				if (clip_x1 == current->x1) {
					if (current_end <= clip_x2) {
						DEBUG_PRINT((" new replace current from %d->%d\n", current->x1,current_end));
						/* Replace current by new */
						push_data_span(row->num_segs,ic,row, current->x1,current_end);
						segbase_inserted=1;
					} else {
						DEBUG_PRINT((" clip current start from %d to %d\n", current->x1,clip_x2+1));
						/* Clip current on the right */
						current->x1 = clip_x2+1;

						DEBUG_PRINT((" insert new from %d->%d\n", clip_x1,clip_x2));
						/* Insert new before current */
						insert_data_span(row->num_segs,ic,row, clip_x1,clip_x2);
						segbase_inserted=1;
					}
				} else {
					/* Insert current after clip_x2 ? */
					if (clip_x2 < current_end) {
						DEBUG_PRINT((" split current from %d->%d\n", clip_x2+1, current->x2));
						insert_data_span(current->id,ic+1,row, clip_x2+1,current->x2);
						segbase_inserted=1;
					}

					DEBUG_PRINT((" insert new from %d->%d\n", clip_x1,clip_x2));
					/* Insert new */
					insert_data_span(row->num_segs,ic+1,row, clip_x1,clip_x2);
					segbase_inserted=1;

					DEBUG_PRINT((" clip current end from %d to %d\n", current_end, clip_x1-1));
					/* Clip current before clip_x1 */
					current->x2 = clip_x1-1;
				}

				/* Continue with remaining */
				x1 = current_end+1;
				break;
			case SEG1_FRONT:
				DEBUG_PRINT(("- %d in front of new\n", ic));
				/* Continue with remaining part */
				x1 = current->x2+1;
				break;
			case SEG1_CLIP_LEFT:
				/*if ((clip_pos<clip_x1) || (clip_pos>clip_x2)) {
					printf("- check Z for %d:%d->%d against %d->%d (%d->%d at %d)\n",ic,
						current->x1,current->x2, x1,x2, clip_x1,clip_x2, clip_pos
					);
				}*/

				DEBUG_PRINT(("- keep left of %d against new at pos %d\n", ic, clip_pos));

				current_end = current->x2;

				/* Insert right part of current, after common zone */
				if (x2 < current->x2) {
					DEBUG_PRINT(("  insert right part of current %d (%d,%d)\n", ic, x2+1,current->x2));
					insert_data_span(current->id,ic+1,row, x2+1,current->x2);
					segbase_inserted=1;
				}

				/* Clip current before clip_pos */
				DEBUG_PRINT(("  clip end of %d (%d,%d) at %d\n", ic, current->x1,current->x2, clip_pos-1));
				current->x2 = clip_pos-1;

				/* Continue with remaining part */
				x1 = clip_pos;
				break;
			case SEG1_CLIP_RIGHT:
				/*if ((clip_pos<clip_x1) || (clip_pos>clip_x2))*/ {
					DEBUG_PRINT(("- check Z for %d:%d->%d against %d->%d (%d->%d at %d)\n",ic,
						current->x1,current->x2, x1,x2, clip_x1,clip_x2, clip_pos
					));
				}

				DEBUG_PRINT(("- keep right of %d against new at pos %d\n", ic, clip_pos));

				current_end = current->x2;

				/* Insert left part of current, before common zone */
				if (current->x1 < x1) {
					DEBUG_PRINT(("  insert left part of current %d (%d,%d)\n", ic, current->x1,x1-1));
					insert_data_span(current->id,ic,row, current->x1,x1-1);
					segbase_inserted=1;

					if (ic>=MAX_SPANS) {
						break;
					}

					current = &(row->span[ic]);
				}
				/* Clip current */
				DEBUG_PRINT(("  clip start of %d (%d,%d) at %d\n", ic, current->x1,current->x2, clip_pos+1));
				current->x1 = clip_pos+1;

				/* Insert new */
				DEBUG_PRINT(("  insert %d,%d at %d\n", x1,clip_pos, ic+1));
				insert_data_span(row->num_segs,ic,row, x1,clip_pos);
				segbase_inserted=1;

				/* Continue with remaining part */
				x1 = current_end+1;
				break;
		}
	}

	DEBUG_PRINT(("--remain %d,%d\n",x1,x2));
	if (x1<=x2) {
		/* Insert last */
		insert_data_span(row->num_segs,row->num_spans,row, x1,x2);
		segbase_inserted=1;
	}

label_insert_base:
	/* Return if we need to insert segment */
	return segbase_inserted;
}

static void draw_poly_sbuffer(draw_t *this, vertexf_t *vtx, int num_vtx)
{
	int miny = video.viewport.h, maxy = -1;
	int minx = video.viewport.w, maxx = -1;
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

		x1 = vtx[p1].pos[0] / vtx[p1].pos[2];
		y1 = vtx[p1].pos[1] / vtx[p1].pos[2];
		w1 = 1.0f /*vtx[p1].pos[3]*/ / vtx[p1].pos[2];
		x2 = vtx[p2].pos[0] / vtx[p2].pos[2];
		y2 = vtx[p2].pos[1] / vtx[p2].pos[2];
		w2 = 1.0f /*vtx[p2].pos[3]*/ / vtx[p2].pos[2];

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
		if (y1 < miny) {
			miny = y1;
		}
		if (y2 > maxy) {
			maxy = y2;
		}

		/*DEBUG_PRINT(("from p[%d]: u=%.3f, v=%.3f to p[%d]: u=%.3f,v=%.3f\n",
			v1, vtx[v1].tx[0], vtx[v1].tx[1],
			v2, vtx[v2].tx[0], vtx[v2].tx[1]
		));*/

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
			for (y=0; y<dy; y++) {
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

				/*DEBUG_PRINT(("line %d, side %d, %.3f,%.3f tu1=%.3f,tv1=%.3f,du=%.3f,dv=%.3f,dy=%d %.3f,%.3f\n",
					y1-1,num_array,
					poly_hlines[y1-1].sbp[num_array].u,poly_hlines[y1-1].sbp[num_array].v,
					tu1,tv1,du,dv,dy,(du*y)/dy,tu1 + ((du*y)/dy)
				));*/
			}
		}

		p1 = p2;
	}

	/* Render horizontal lines */
	miny=MAX(miny, 0);
	maxy=MIN(maxy, video.viewport.h);

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

	for (y=miny; y<maxy; y++) {
		int pminx = poly_hlines[y].sbp[0].x;
		int pmaxx = poly_hlines[y].sbp[1].x;

		minx=MIN(minx, pminx);
		maxx=MAX(maxx, pmaxx);

		segment.start = poly_hlines[y].sbp[0];
		segment.end = poly_hlines[y].sbp[1];

#if 1
		if (gen_seg_spans(y, &segment)) {
			add_base_segment(y, &segment);
		}
#else
		if (draw_add_segment(y, &segment)) {
			add_base_segment(y, &segment);
		}
#endif
	}

	/*dump_sbuffer();*/

	minx=MAX(minx, 0);
	maxx=MIN(maxx, video.viewport.w);

	/* Mark dirty rectangle */
	video.dirty_rects[video.numfb]->setDirty(video.dirty_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
	video.upload_rects[video.numfb]->setDirty(video.upload_rects[video.numfb],
		minx+video.viewport.x, miny+video.viewport.y, maxx-minx+1, maxy-miny+1);
}

/* Specific version for non filled polys */
static void draw_poly_sbuffer_line(draw_t *this, vertexf_t *vtx, int num_vtx)
{
	int miny = video.viewport.h, maxy = -1;
	int minx = video.viewport.w, maxx = -1;
	int y, p1, p2, prevx1, prevx2;
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

		x1 = vtx[p1].pos[0] / vtx[p1].pos[2];
		y1 = vtx[p1].pos[1] / vtx[p1].pos[2];
		w1 = 1.0f /*vtx[p1].pos[3]*/ / vtx[p1].pos[2];
		x2 = vtx[p2].pos[0] / vtx[p2].pos[2];
		y2 = vtx[p2].pos[1] / vtx[p2].pos[2];
		w2 = 1.0f /*vtx[p2].pos[3]*/ / vtx[p2].pos[2];

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
		if (y1 < miny) {
			miny = y1;
		}
		if (y2 > maxy) {
			maxy = y2;
		}

		/*DEBUG_PRINT(("from p[%d]: u=%.3f, v=%.3f to p[%d]: u=%.3f,v=%.3f\n",
			v1, vtx[v1].tx[0], vtx[v1].tx[1],
			v2, vtx[v2].tx[0], vtx[v2].tx[1]
		));*/

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
			for (y=0; y<dy; y++) {
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

				/*DEBUG_PRINT(("line %d, side %d, %.3f,%.3f tu1=%.3f,tv1=%.3f,du=%.3f,dv=%.3f,dy=%d %.3f,%.3f\n",
					y1-1,num_array,
					poly_hlines[y1-1].sbp[num_array].u,poly_hlines[y1-1].sbp[num_array].v,
					tu1,tv1,du,dv,dy,(du*y)/dy,tu1 + ((du*y)/dy)
				));*/
			}
		}

		p1 = p2;
	}

	/* Render horizontal lines */
	miny=MAX(miny, 0);
	maxy=MIN(maxy, video.viewport.h);

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

	prevx1 = poly_hlines[miny].sbp[0].x;
	prevx2 = poly_hlines[miny].sbp[1].x;

	for (y=miny; y<maxy; y++) {
		int pminx = poly_hlines[y].sbp[0].x;
		int pmaxx = poly_hlines[y].sbp[1].x;
		int add_seg;

		minx=MIN(minx, pminx);
		maxx=MAX(maxx, pmaxx);

		segment.start = poly_hlines[y].sbp[0];
		segment.end = poly_hlines[y].sbp[0];
		if (prevx1<pminx) {
			segment.start.x = prevx1+1;
		} else if (prevx1>pminx) {
			segment.end.x = prevx1-1;
		}

#if 1
		add_seg = gen_seg_spans(y, &segment);
#else
		add_seg = draw_add_segment(y, &segment);
#endif
		segment.start = poly_hlines[y].sbp[1];
		segment.end = poly_hlines[y].sbp[1];
		if (prevx2<pmaxx) {
			segment.start.x = prevx2+1;
		} else if (prevx2>pmaxx) {
			segment.end.x = prevx2-1;
		}

#if 1
		add_seg |= gen_seg_spans(y, &segment);
#else
		add_seg |= draw_add_segment(y, &segment);
#endif

		if (add_seg) {
			add_base_segment(y, &segment);
		}

		prevx1 = pminx;
		prevx2 = pmaxx;
	}

	/*dump_sbuffer();*/

	minx=MAX(minx, 0);
	maxx=MIN(maxx, video.viewport.w);

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

	/*printf("mask segment %d: %d,%d\n",y,x1,x2);*/

	segment.start.x = x1;
	segment.end.x = x2;
	segment.start.w = segment.end.w = w;
	segment.masking = 1;

#if 1
	if (gen_seg_spans(y, &segment)) {
		add_base_segment(y, &segment);
	}
#else
	if (draw_add_segment(y, &segment)) {
		add_base_segment(y, &segment);
	}
#endif

	/* Upper layer will update dirty rectangles */
}
