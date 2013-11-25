/*#define CONCAT2(x,y)	x ## y
#define FNDEF2(name,bpp)	CONCAT2(name,bpp)
#define CONCAT3(x,y,z)	x ## y ## z
#define FNDEF3(name,bpp,perscorr)	CONCAT3(name,bpp,perscorr)
#define CONCAT4(x,y,z,w)	x ## y ## z ## w
#define FNDEF4(name,bpp,perscorr,alphatest)	CONCAT4(name,bpp,perscorr,alphatest)

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

#define FUNC_SUFFIX opaque
#define WRITE_ALPHATESTED_PIXEL \
	color = palette[tex_pixels[pv|pu]];	\
	WRITE_PIXEL_GONEXT(dst_col, color)

#define FUNC_SUFFIX trans
#define WRITE_ALPHATESTED_PIXEL \
	{	\
		Uint8 c = tex_pixels[pv|pu];	\
		if (alpha_pal[c]) {	\
			color = palette[c];	\
			WRITE_PIXEL(dst_col, color)	\
		}	\
		PIXEL_GONEXT(dst_col)	\
	}

*/

void FNDEF4(draw_render_textured, BPP, _pc0, FUNC_SUFFIX) (SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	int dxtotal, dx, i;
	render_texture_t *tex = segment->texture;
	Uint32 ui,vi,dui,dvi;
	Uint32 ubits, umask, vbits, vmask;
	PIXEL_TYPE *dst_col = (PIXEL_TYPE *) dst_line;

	dxtotal = segment->end.x - segment->start.x + 1;
	dx = x1-segment->start.x;

	u1 = segment->start.u * 65536.0f;
	v1 = segment->start.v * 65536.0f;
	u2 = segment->end.u * 65536.0f;
	v2 = segment->end.v * 65536.0f;

	du = (u2-u1)/dxtotal;
	dv = (v2-v1)/dxtotal;

	ui = u1 + du * dx;
	vi = v1 + dv * dx;
	dui = du;
	dvi = dv;

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
			Uint32 pu,pv;

			pu = ui>>16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv = vi>>(16-ubits);	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

			WRITE_ALPHATESTED_PIXEL

			ui += dui;
			vi += dvi;
		}
	} else {
		TEXTURE_PIXEL_TYPE *tex_pixels = (TEXTURE_PIXEL_TYPE *) tex->pixels;

		for (i=x1; i<=x2; i++) {
			Uint32 pu,pv;

			pu = ui>>16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv = vi>>(16-ubits);	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

			color = tex_pixels[pv|pu];
			WRITE_PIXEL_GONEXT(dst_col, color)

			ui += dui;
			vi += dvi;
		}
	}
}

void FNDEF4(draw_render_textured, BPP, _pc1, FUNC_SUFFIX) (SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float invw;
	int dxtotal, dx, i;
	render_texture_t *tex = segment->texture;
	Uint32 ui,vi,dui,dvi;
	Uint32 ubits, umask, vbits, vmask;
	PIXEL_TYPE *dst_col = (PIXEL_TYPE *) dst_line;

	dxtotal = segment->end.x - segment->start.x + 1;
	dx = x1-segment->start.x;

	invw = 65536.0f / segment->start.w;
	u1 = segment->start.u * invw;
	v1 = segment->start.v * invw;
	invw = 65536.0f / segment->end.w;
	u2 = segment->end.u * invw;
	v2 = segment->end.v * invw;

	du = (u2-u1)/dxtotal;
	dv = (v2-v1)/dxtotal;

	ui = u1 + du * dx;
	vi = v1 + dv * dx;
	dui = du;
	dvi = dv;

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
			Uint32 pu,pv;

			pu = ui>>16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv = vi>>(16-ubits);	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

			WRITE_ALPHATESTED_PIXEL

			ui += dui;
			vi += dvi;
		}
	} else {
		TEXTURE_PIXEL_TYPE *tex_pixels = (TEXTURE_PIXEL_TYPE *) tex->pixels;

		for (i=x1; i<=x2; i++) {
			Uint32 pu,pv;

			pu = ui>>16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv = vi>>(16-ubits);	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

			color = tex_pixels[pv|pu];
			WRITE_PIXEL_GONEXT(dst_col, color)

			ui += dui;
			vi += dvi;
		}
	}
}

void FNDEF4(draw_render_textured, BPP, _pc2, FUNC_SUFFIX) (SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float invw;
	float du16,dv16,dw16;
	int dxtotal, dx, i;
	Uint32 ubits, umask, vbits, vmask;
	render_texture_t *tex = segment->texture;
	PIXEL_TYPE *dst_col = (PIXEL_TYPE *) dst_line;
	Uint32 ui,vi,dui,dvi;
	float uuf, vvf, uu2f, vv2f;

	dxtotal = segment->end.x - segment->start.x + 1;
	dx = x1-segment->start.x;

	u1 = segment->start.u;
	v1 = segment->start.v;
	w1 = segment->start.w;

	u2 = segment->end.u;
	v2 = segment->end.v;
	w2 = segment->end.w;

	du = (u2-u1)/dxtotal;
	dv = (v2-v1)/dxtotal;
	dw = (w2-w1)/dxtotal;

	u1 += du * dx;
	v1 += dv * dx;
	w1 += dw * dx;

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

		for (i=x1; x2-i>=16; i+=16) {
			int j;

			u2 = u1 + du16;
			v2 = v1 + dv16;
			w2 = w1 + dw16;

			invw = 65536.0f / w1;
			uuf = u1 * invw;
			vvf = v1 * invw;
			invw = 65536.0f / w2;
			uu2f = u2 * invw;
			vv2f = v2 * invw;

			dui = (uu2f-uuf)/16.0f;
			dvi = (vv2f-vvf)/16.0f;
			ui = uuf;
			vi = vvf;

			for (j=0; j<16; j++) {
				Uint32 pu,pv;

				pu = ui>>16;		/* 0000XXXX */
				pu &= umask;		/* 0000---X */
				pv = vi>>(16-ubits);	/* 000YYYYy */
				pv &= vmask;		/* 000YYYY- */

				WRITE_ALPHATESTED_PIXEL

				ui += dui;
				vi += dvi;
			}

			u1 = u2;
			v1 = v2;
			w1 = w2;
		}

		/* Remaining part */
		u2 = u1 + du16;
		v2 = v1 + dv16;
		w2 = w1 + dw16;

		invw = 65536.0f / w1;
		uuf = u1 * invw;
		vvf = v1 * invw;
		invw = 65536.0f / w2;
		uu2f = u2 * invw;
		vv2f = v2 * invw;

		dui = (uu2f-uuf)/16.0f;
		dvi = (vv2f-vvf)/16.0f;
		ui = uuf;
		vi = vvf;

		for ( ; i<=x2; i++) {
			Uint32 pu,pv;

			pu = ui>>16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv = vi>>(16-ubits);	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

			WRITE_ALPHATESTED_PIXEL

			ui += dui;
			vi += dvi;
		}
	} else {
		TEXTURE_PIXEL_TYPE *tex_pixels = (TEXTURE_PIXEL_TYPE *) tex->pixels;

		for (i=x1; x2-i>=16; i+=16) {
			int j;

			u2 = u1 + du16;
			v2 = v1 + dv16;
			w2 = w1 + dw16;

			invw = 65536.0f / w1;
			uuf = u1 * invw;
			vvf = v1 * invw;
			invw = 65536.0f / w2;
			uu2f = u2 * invw;
			vv2f = v2 * invw;

			dui = (uu2f-uuf)/16.0f;
			dvi = (vv2f-vvf)/16.0f;
			ui = uuf;
			vi = vvf;

			for (j=0; j<16; j++) {
				Uint32 pu,pv;

				pu = ui>>16;		/* 0000XXXX */
				pu &= umask;		/* 0000---X */
				pv = vi>>(16-ubits);	/* 000YYYYy */
				pv &= vmask;		/* 000YYYY- */

				color = tex_pixels[pv|pu];
				WRITE_PIXEL_GONEXT(dst_col, color)

				ui += dui;
				vi += dvi;
			}

			u1 = u2;
			v1 = v2;
			w1 = w2;
		}

		/* Remaining part */
		u2 = u1 + du16;
		v2 = v1 + dv16;
		w2 = w1 + dw16;

		invw = 65536.0f / w1;
		uuf = u1 * invw;
		vvf = v1 * invw;
		invw = 65536.0f / w2;
		uu2f = u2 * invw;
		vv2f = v2 * invw;

		dui = (uu2f-uuf)/16.0f;
		dvi = (vv2f-vvf)/16.0f;
		ui = uuf;
		vi = vvf;

		for ( ; i<=x2; i++) {
			Uint32 pu,pv;

			pu = ui>>16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv = vi>>(16-ubits);	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

			color = tex_pixels[pv|pu];
			WRITE_PIXEL_GONEXT(dst_col, color)

			ui += dui;
			vi += dvi;
		}
	}
}

void FNDEF4(draw_render_textured, BPP, _pc3, FUNC_SUFFIX) (SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float invw;
	int dxtotal, i;
	Uint32 ubits, umask, vbits, vmask;
	render_texture_t *tex = segment->texture;
	PIXEL_TYPE *dst_col = (PIXEL_TYPE *) dst_line;

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
			Uint32 pu,pv;

			invw = 65536.0f / w;
			pu = u * invw;	/* XXXXxxxx */
			pv = v * invw;	/* YYYYyyyy */

			pu >>= 16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv >>= 16-ubits;	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

			WRITE_ALPHATESTED_PIXEL

			u += du;
			v += dv;
			w += dw;
		}
	} else {
		TEXTURE_PIXEL_TYPE *tex_pixels = (TEXTURE_PIXEL_TYPE *) tex->pixels;

		for (i=x1; i<=x2; i++) {
			Uint32 pu,pv;

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
