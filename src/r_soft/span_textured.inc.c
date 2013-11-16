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
#define PIXEL_FROM_RGB(color, r,g,b) \
	color = SDL_MapRGB(surf->format, r,g,b);
*/

void FNDEF3(draw_render_textured, BPP, _pc0) (SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, duf,dvf;
	int dxtotal, dx, i;
	render_texture_t *tex = segment->texture;
	Uint32 u,v,du,dv;
	Uint32 ubits, umask, vbits, vmask;
	PIXEL_TYPE *dst_col = (PIXEL_TYPE *) dst_line;
	Uint32 color;

	dxtotal = segment->end.x - segment->start.x + 1;
	dx = x1-segment->start.x;

	u1 = segment->start.u * 65536.0f;
	v1 = segment->start.v * 65536.0f;
	u2 = segment->end.u * 65536.0f;
	v2 = segment->end.v * 65536.0f;

	duf = (u2-u1)/dxtotal;
	dvf = (v2-v1)/dxtotal;

	u = u1 + duf * dx;
	v = v1 + dvf * dx;
	du = duf;
	dv = dvf;

	ubits = logbase2(tex->pitchw);
	umask = (1<<ubits)-1;
	vbits = logbase2(tex->pitchh);
	vmask = (1<<vbits)-1;
	vmask <<= ubits;

	if (tex->paletted) {
		Uint32 *palette = tex->palettes[segment->tex_num_pal];
		Uint8 *alpha_pal = tex->alpha_palettes[segment->tex_num_pal];
		Uint8 *tex_pixels = (Uint8 *) tex->pixels;

		for (i=x1; i<=x2; i++) {
			Uint8 c;
			Uint32 pu,pv;

			pu = u>>16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv = v>>(16-ubits);	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

#ifdef FORCE_OPAQUE
			color = palette[tex_pixels[pv|pu]];
			WRITE_PIXEL_GONEXT(dst_col, color)
#else
			c = tex_pixels[pv|pu];
			if (alpha_pal[c]) {
				color = palette[c];
				WRITE_PIXEL(dst_col, color)
			}
			PIXEL_GONEXT(dst_col)
#endif

			u += du;
			v += dv;
		}
	} else {
		Uint32 *tex_pixels = (Uint32 *) tex->pixels;

		for (i=x1; i<=x2; i++) {
			Uint32 pu,pv;

			pu = u>>16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv = v>>(16-ubits);	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

			color = tex_pixels[pv|pu];
			WRITE_PIXEL_GONEXT(dst_col, color)

			u += du;
			v += dv;
		}
	}
}

void FNDEF3(draw_render_textured, BPP, _pc1) (SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, invw, duf,dvf;
	int dxtotal, dx, i;
	render_texture_t *tex = segment->texture;
	Uint32 u,v,du,dv;
	Uint32 ubits, umask, vbits, vmask;
	PIXEL_TYPE *dst_col = (PIXEL_TYPE *) dst_line;
	Uint32 color;

	dxtotal = segment->end.x - segment->start.x + 1;
	dx = x1-segment->start.x;

	invw = 65536.0f / segment->start.w;
	u1 = segment->start.u * invw;
	v1 = segment->start.v * invw;
	invw = 65536.0f / segment->end.w;
	u2 = segment->end.u * invw;
	v2 = segment->end.v * invw;

	duf = (u2-u1)/dxtotal;
	dvf = (v2-v1)/dxtotal;

	u = u1 + duf * dx;
	v = v1 + dvf * dx;
	du = duf;
	dv = dvf;

	ubits = logbase2(tex->pitchw);
	umask = (1<<ubits)-1;
	vbits = logbase2(tex->pitchh);
	vmask = (1<<vbits)-1;
	vmask <<= ubits;

	if (tex->paletted) {
		Uint32 *palette = tex->palettes[segment->tex_num_pal];
		Uint8 *alpha_pal = tex->alpha_palettes[segment->tex_num_pal];
		Uint8 *tex_pixels = (Uint8 *) tex->pixels;

		for (i=x1; i<=x2; i++) {
			Uint8 c;
			Uint32 pu,pv;

			pu = u>>16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv = v>>(16-ubits);	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

#ifdef FORCE_OPAQUE
			color = palette[tex_pixels[pv|pu]];
			WRITE_PIXEL_GONEXT(dst_col, color)
#else
			c = tex_pixels[pv|pu];
			if (alpha_pal[c]) {
				color = palette[c];
				WRITE_PIXEL(dst_col, color)
			}
			PIXEL_GONEXT(dst_col)
#endif

			u += du;
			v += dv;
		}
	} else {
		Uint32 *tex_pixels = (Uint32 *) tex->pixels;

		for (i=x1; i<=x2; i++) {
			Uint32 pu,pv;

			pu = u>>16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv = v>>(16-ubits);	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

			color = tex_pixels[pv|pu];
			WRITE_PIXEL_GONEXT(dst_col, color)

			u += du;
			v += dv;
		}
	}
}

void FNDEF3(draw_render_textured, BPP, _pc2) (SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, du,dv, u,v;
	float w1, w2, w, dw, invw;
	float du16,dv16,dw16;
	int dxtotal, i;
	Uint32 ubits, umask, vbits, vmask;
	render_texture_t *tex = segment->texture;
	PIXEL_TYPE *dst_col = (PIXEL_TYPE *) dst_line;
	Uint32 color;

	dxtotal = segment->end.x - segment->start.x + 1;

	u1 = segment->start.u;
	v1 = segment->start.v;
	w1 = segment->start.w;

	u2 = segment->end.u;
	v2 = segment->end.v;
	w2 = segment->end.w;

	du = (u2-u1)/dxtotal;
	dv = (v2-v1)/dxtotal;
	dw = (w2-w1)/dxtotal;

	u1 += du * (x1-segment->start.x);
	v1 += dv * (x1-segment->start.x);
	w1 += dw * (x1-segment->start.x);

	du16 = du * 16.0f;
	dv16 = dv * 16.0f;
	dw16 = dw * 16.0f;

	ubits = logbase2(tex->pitchw);
	umask = (1<<ubits)-1;
	vbits = logbase2(tex->pitchh);
	vmask = (1<<vbits)-1;
	vmask <<= ubits;

	if (tex->paletted) {
		Uint32 *palette = tex->palettes[segment->tex_num_pal];
		Uint8 *alpha_pal = tex->alpha_palettes[segment->tex_num_pal];
		Uint8 *tex_pixels = (Uint8 *) tex->pixels;

		for (i=x1; i<=x2; i+=16) {
			int j;
			Uint32 dui, dvi, uu, vv, uu2,vv2;

			u2 = u1 + du16;
			v2 = v1 + dv16;
			w2 = w1 + dw16;

			invw = 65536.0f / w1;
			uu = u1 * invw;
			vv = v1 * invw;
			invw = 65536.0f / w2;
			uu2 = u2 * invw;
			vv2 = v2 * invw;

			dui = (uu2-uu)/16.0f;
			dvi = (vv2-vv)/16.0f;

			for (j=0; j<MIN(x2-i+1,16); j++) {
				Uint8 c;
				Uint32 pu,pv;

				pu = uu>>16;		/* 0000XXXX */
				pu &= umask;		/* 0000---X */
				pv = vv>>(16-ubits);	/* 000YYYYy */
				pv &= vmask;		/* 000YYYY- */

#ifdef FORCE_OPAQUE
				color = palette[tex_pixels[pv|pu]];
				WRITE_PIXEL_GONEXT(dst_col, color)
#else
				c = tex_pixels[pv|pu];
				if (alpha_pal[c]) {
					color = palette[c];
					WRITE_PIXEL(dst_col, color)
				}
				PIXEL_GONEXT(dst_col)
#endif

				uu += dui;
				vv += dvi;
			}

			u1 = u2;
			v1 = v2;
			w1 = w2;
		}
	} else {
		Uint32 *tex_pixels = (Uint32 *) tex->pixels;

		for (i=x1; i<=x2; i+=16) {
			int j;
			Uint32 dui, dvi, uu, vv, uu2,vv2;

			u2 = u1 + du16;
			v2 = v1 + dv16;
			w2 = w1 + dw16;

			invw = 65536.0f / w1;
			uu = u1 * invw;
			vv = v1 * invw;
			invw = 65536.0f / w2;
			uu2 = u2 * invw;
			vv2 = v2 * invw;

			dui = (uu2-uu)/16.0f;
			dvi = (vv2-vv)/16.0f;

			for (j=0; j<MIN(x2-i+1,16); j++) {
				Uint32 pu,pv;

				pu = uu>>16;		/* 0000XXXX */
				pu &= umask;		/* 0000---X */
				pv = vv>>(16-ubits);	/* 000YYYYy */
				pv &= vmask;		/* 000YYYY- */

				color = tex_pixels[pv|pu];
				WRITE_PIXEL_GONEXT(dst_col, color)

				uu += dui;
				vv += dvi;
			}

			u1 = u2;
			v1 = v2;
			w1 = w2;
		}
	}
}

void FNDEF3(draw_render_textured, BPP, _pc3) (SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, du,dv, u,v;
	float w1, w2, w, dw;
	int dxtotal, i;
	Uint32 ubits, umask, vbits, vmask;
	render_texture_t *tex = segment->texture;
	PIXEL_TYPE *dst_col = (PIXEL_TYPE *) dst_line;
	Uint32 color;

	dxtotal = segment->end.x - segment->start.x + 1;

	u1 = segment->start.u;
	v1 = segment->start.v;
	w1 = segment->start.w;
	u2 = segment->end.u;
	v2 = segment->end.v;
	w2 = segment->end.w;

	du = (u2-u1)/dxtotal;
	dv = (v2-v1)/dxtotal;
	dw = (w2-w1)/dxtotal;

	u = u1 + du * (x1-segment->start.x);
	v = v1 + dv * (x1-segment->start.x);
	w = w1 + dw * (x1-segment->start.x);

	ubits = logbase2(tex->pitchw);
	umask = (1<<ubits)-1;
	vbits = logbase2(tex->pitchh);
	vmask = (1<<vbits)-1;
	vmask <<= ubits;

	if (tex->paletted) {
		Uint32 *palette = tex->palettes[segment->tex_num_pal];
		Uint8 *alpha_pal = tex->alpha_palettes[segment->tex_num_pal];
		Uint8 *tex_pixels = (Uint8 *) tex->pixels;

		for (i=x1; i<=x2; i++) {
			Uint8 c;
			Uint32 pu,pv;
			float invw;

			invw = 65536.0f / w;
			pu = u * invw;	/* XXXXxxxx */
			pv = v * invw;	/* YYYYyyyy */

			pu >>= 16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv >>= 16-ubits;	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

#ifdef FORCE_OPAQUE
			color = palette[tex_pixels[pv|pu]];
			WRITE_PIXEL_GONEXT(dst_col, color)
#else
			c = tex_pixels[pv|pu];
			if (alpha_pal[c]) {
				color = palette[c];
				WRITE_PIXEL(dst_col, color)
			}
			PIXEL_GONEXT(dst_col)
#endif

			u += du;
			v += dv;
			w += dw;
		}
	} else {
		Uint32 *tex_pixels = (Uint32 *) tex->pixels;

		for (i=x1; i<=x2; i++) {
			Uint32 pu,pv;
			float invw;

			invw = 65536.0f / w;
			pu = u * invw;	/* XXXXxxxx */
			pv = v * invw;	/* YYYYyyyy */

			pu >>= 16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv >>= 16-ubits;	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

			color = tex_pixels[pv|pu];
			WRITE_PIXEL_GONEXT(dst_col, color)

			u += du;
			v += dv;
			w += dw;
		}
	}
}
