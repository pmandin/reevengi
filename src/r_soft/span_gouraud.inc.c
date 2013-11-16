/*#define CONCAT2(x,y)	x ## y
#define FNDEF2(name,bpp)	CONCAT2(name,bpp)
#define CONCAT3(x,y,z)	x ## y ## z
#define FNDEF3(name,bpp,perscorr)	CONCAT3(name,bpp,perscorr)

#define BPP 32
#define PIXEL_TYPE	Uint32
#define WRITE_PIXEL(output, color)
		*output = color;
#define WRITE_PIXEL_GONEXT(output, color)
		*output++ = color;
#define PIXEL_GONEXT(output) \
	output++;
*/

void FNDEF3(draw_render_gouraud, BPP, _pc0) (SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float r1,g1,b1, r2,g2,b2, r,g,b, dr,dg,db;
	int dxtotal, dx, i;
	PIXEL_TYPE *dst_col = (PIXEL_TYPE *) dst_line;

	dxtotal = segment->end.x - segment->start.x + 1;
	dx = x1-segment->start.x;

	r1 = segment->start.r;
	g1 = segment->start.g;
	b1 = segment->start.b;
	r2 = segment->end.r;
	g2 = segment->end.g;
	b2 = segment->end.b;

	dr = (r2-r1)/dxtotal;
	dg = (g2-g1)/dxtotal;
	db = (b2-b1)/dxtotal;

	r = r1 + dr * dx;
	g = g1 + dg * dx;
	b = b1 + db * dx;

	for (i=x1; i<=x2; i++) {
		Uint32 color = SDL_MapRGB(surf->format, r,g,b);

		WRITE_PIXEL_GONEXT(dst_col, color)

		r += dr;
		g += dg;
		b += db;
	}
}

void FNDEF3(draw_render_gouraud, BPP, _pc1) (SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float r1,g1,b1, r2,g2,b2, r,g,b, dr,dg,db, invw;
	int dxtotal, dx, i;
	PIXEL_TYPE *dst_col = (PIXEL_TYPE *) dst_line;

	dxtotal = segment->end.x - segment->start.x + 1;
	dx = x1-segment->start.x;

	invw = 1.0f / segment->start.w;
	r1 = segment->start.r * invw;
	g1 = segment->start.g * invw;
	b1 = segment->start.b * invw;
	invw = 1.0f / segment->end.w;
	r2 = segment->end.r * invw;
	g2 = segment->end.g * invw;
	b2 = segment->end.b * invw;

	dr = (r2-r1)/dxtotal;
	dg = (g2-g1)/dxtotal;
	db = (b2-b1)/dxtotal;

	r = r1 + dr * dx;
	g = g1 + dg * dx;
	b = b1 + db * dx;

	for (i=x1; i<=x2; i++) {
		Uint32 color = SDL_MapRGB(surf->format, r,g,b);

		WRITE_PIXEL_GONEXT(dst_col, color)

		r += dr;
		g += dg;
		b += db;
	}
}

void FNDEF3(draw_render_gouraud, BPP, _pc3) (SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float r1,g1,b1, r2,g2,b2, r,g,b, dr,dg,db;
	float w1, w2, w, dw, invw;
	int dxtotal, dx, i;
	PIXEL_TYPE *dst_col = (PIXEL_TYPE *) dst_line;

	dxtotal = segment->end.x - segment->start.x + 1;
	dx = x1-segment->start.x;

	r1 = segment->start.r;
	g1 = segment->start.g;
	b1 = segment->start.b;
	w1 = segment->start.w;
	r2 = segment->end.r;
	g2 = segment->end.g;
	b2 = segment->end.b;
	w2 = segment->end.w;

	dr = (r2-r1)/dxtotal;
	dg = (g2-g1)/dxtotal;
	db = (b2-b1)/dxtotal;
	dw = (w2-w1)/dxtotal;

	r = r1 + dr * dx;
	g = g1 + dg * dx;
	b = b1 + db * dx;
	w = w1 + dw * dx;

	for (i=x1; i<=x2; i++) {
		int rr,gg,bb;
		Uint32 color;

		invw = 1.0f / w;
		rr = r * invw;
		gg = g * invw;
		bb = b * invw;

		color = SDL_MapRGB(surf->format, rr,gg,bb);

		WRITE_PIXEL_GONEXT(dst_col, color)

		r += dr;
		g += dg;
		b += db;
		w += dw;
	}
}
