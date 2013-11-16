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
*/

void FNDEF2(draw_render_fill, BPP) (SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	Uint32 color;
	float r,g,b;
	PIXEL_TYPE *dst_col = (PIXEL_TYPE *) dst_line;
	int i;

	r = segment->start.r;
	g = segment->start.g;
	b = segment->start.b;
	if (draw.correctPerspective>0) {
		r /= segment->start.w;
		g /= segment->start.w;
		b /= segment->start.w;
	}

	color = SDL_MapRGB(surf->format, r,g,b);

	for (i=x1; i<=x2; i++) {
		WRITE_PIXEL_GONEXT(dst_col, color)
	}
}
