#define FORCE_OPAQUE 1

void draw_render_textured8_pc0opaquem68k(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, duf,dvf;
	int dxtotal, dx, i;
	render_texture_t *tex = segment->texture;
	Uint32 u,v,du,dv;
	Uint32 ubits, umask, vbits, vmask;
	Uint8 *dst_col = dst_line;
	Uint32 *palette;
	Uint8 *alpha_pal;
	Uint8 *tex_pixels;

	if (!tex->paletted)
		return;

	ubits = logbase2(tex->pitchw);
	umask = (1<<ubits)-1;
	vbits = logbase2(tex->pitchh);
	vmask = (1<<vbits)-1;
	vmask <<= ubits;

	if (ubits+vbits>16) {
		draw_render_textured8_pc0opaque(surf, dst_line, segment, x1, x2);
		return;
	}

	palette = tex->palettes[segment->tex_num_pal];
	alpha_pal = tex->alpha_palettes[segment->tex_num_pal];
	tex_pixels = tex->pixels;

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

/*	XXxxYYyy	uv, duv
	XXxx--YY	lsr.w
	xx--YYXX	rol.l	*/

	dx = x2 - x1;

	/* Write first single pixel */
	if (dx & 1) {
		Uint8 c;
		Uint32 pu,pv;

		pu = u>>16;		/* 0000XXXX */
		pu &= umask;		/* 0000---X */
		pv = v>>(16-ubits);	/* 000YYYYy */
		pv &= vmask;		/* 000YYYY- */

#ifdef FORCE_OPAQUE
		*dst_col++ = palette[tex_pixels[pv|pu]];
#else
		c = tex_pixels[pv|pu];
		if (alpha_pal[c]) {
			*dst_col = palette[c];
		}
		dst_col++;
#endif

		u += du;
		v += dv;

		--dx;
	}

	if (dx) {
		Uint32 uv, duv;

		uv = (u<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		uv |= (v>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */
		duv = (du<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		duv |= (dv>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */

		/* for signed d0:w addressing */
		if (ubits+vbits>15) {
			tex_pixels += 32768;
			uv ^= 0x8000;
		}

		vbits = 16-vbits;

		dx >>= 1;

__asm__ __volatile__ (
	"movel	%5,d4\n\t"
	"lsrw	%7,d4\n\t"
	"roll	%6,d4\n\t"
	"moveql	#0,d5\n\t"
	"moveql	#0,d1\n"

"0:\n\t"
	"moveb	%2@(0,d4:w),d5\n\t"
	"movel	%5,d0\n\t"

	"moveb	%3@(3,d5:w*4),d4\n\t"
	"lsrw	%7,d0\n\t"

	"moveb	d4,%0@+\n\t"
	"roll	%6,d0\n\t"

	"addal	%4,%5\n\t"
	"moveb	%2@(0,d0:w),d1\n\t"

	"movel	%5,d4\n\t"
	"moveb	%3@(3,d1:w*4),d0\n\t"

	"lsrw	%7,d4\n\t"
	"moveb	d0,%0@+\n\t"

	"roll	%6,d4\n\t"
	"orw	d5,d5\n\t"

	"subqw	#1,%1\n\t"
	"addal	%4,%5\n\t"

	"bpls	0b\n"

	: /* output */
		"+a"(dst_col) /*%0*/
	: /* input */
		"d"(dx) /*%1*/, "a"(tex_pixels) /*%2*/, "a"(palette) /*%3*/,
		"a"(duv) /*%4*/, "a"(uv) /*%5*/, "d"(ubits) /*%6*/, "d"(vbits) /*%7*/
	: /* clobbered registers */
		"d0", "d1", "d2", "d4", "d5", "cc", "memory" 
);
	}
}

void draw_render_textured8_pc1opaquem68k(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, invw, duf,dvf;
	int dxtotal, dx, i;
	render_texture_t *tex = segment->texture;
	Uint32 u,v,du,dv;
	Uint32 ubits, umask, vbits, vmask;
	Uint8 *dst_col = dst_line;
	Uint32 *palette;
	Uint8 *alpha_pal;
	Uint8 *tex_pixels;

	if (!tex->paletted)
		return;

	ubits = logbase2(tex->pitchw);
	umask = (1<<ubits)-1;
	vbits = logbase2(tex->pitchh);
	vmask = (1<<vbits)-1;
	vmask <<= ubits;

	if (ubits+vbits>16) {
		draw_render_textured8_pc1opaque(surf, dst_line, segment, x1, x2);
		return;
	}

	palette = tex->palettes[segment->tex_num_pal];
	alpha_pal = tex->alpha_palettes[segment->tex_num_pal];
	tex_pixels = tex->pixels;

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

/*	XXxxYYyy	uv, duv
	XXxx--YY	lsr.w
	xx--YYXX	rol.l	*/

	dx = x2 - x1;

	/* Write first single pixel */
	if (dx & 1) {
		Uint8 c;
		Uint32 pu,pv;

		pu = u>>16;		/* 0000XXXX */
		pu &= umask;		/* 0000---X */
		pv = v>>(16-ubits);	/* 000YYYYy */
		pv &= vmask;		/* 000YYYY- */

#ifdef FORCE_OPAQUE
		*dst_col++ = palette[tex_pixels[pv|pu]];
#else
		c = tex_pixels[pv|pu];
		if (alpha_pal[c]) {
			*dst_col = palette[c];
		}
		dst_col++;
#endif

		u += du;
		v += dv;

		--dx;
	}

	if (dx) {
		Uint32 uv, duv;

		uv = (u<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		uv |= (v>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */
		duv = (du<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		duv |= (dv>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */

		/* for signed d0:w addressing */
		if (ubits+vbits>15) {
			tex_pixels += 32768;
			uv ^= 0x8000;
		}

		vbits = 16-vbits;

		dx >>= 1;

__asm__ __volatile__ (
	"movel	%5,d4\n\t"
	"lsrw	%7,d4\n\t"
	"roll	%6,d4\n\t"
	"moveql	#0,d5\n\t"
	"moveql	#0,d1\n"

"0:\n\t"
	"moveb	%2@(0,d4:w),d5\n\t"
	"movel	%5,d0\n\t"

	"moveb	%3@(3,d5:w*4),d4\n\t"
	"lsrw	%7,d0\n\t"

	"moveb	d4,%0@+\n\t"
	"roll	%6,d0\n\t"

	"addal	%4,%5\n\t"
	"moveb	%2@(0,d0:w),d1\n\t"

	"movel	%5,d4\n\t"
	"moveb	%3@(3,d1:w*4),d0\n\t"

	"lsrw	%7,d4\n\t"
	"moveb	d0,%0@+\n\t"

	"roll	%6,d4\n\t"
	"orw	d5,d5\n\t"

	"subqw	#1,%1\n\t"
	"addal	%4,%5\n\t"

	"bpls	0b\n"

	: /* output */
		"+a"(dst_col) /*%0*/
	: /* input */
		"d"(dx) /*%1*/, "a"(tex_pixels) /*%2*/, "a"(palette) /*%3*/,
		"a"(duv) /*%4*/, "a"(uv) /*%5*/, "d"(ubits) /*%6*/, "d"(vbits) /*%7*/
	: /* clobbered registers */
		"d0", "d1", "d2", "d4", "d5", "cc", "memory" 
);
	}
}

void draw_render_textured8_pc2opaquem68k(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, du,dv, u,v;
	float w1, w2, w, dw, invw;
	float du16,dv16,dw16;
	int dxtotal, dx, i;
	Uint32 ubits, umask, vbits, vmask;
	render_texture_t *tex = segment->texture;
	Uint8 *dst_col = dst_line;
	Uint32 *palette;
	Uint8 *alpha_pal;
	Uint8 *tex_pixels;
#if defined(__GNUC__) && defined(__m68k__)
	int vbits1;
	Uint8 *tex_pixels1;
#endif

	if (!tex->paletted)
		return;

	ubits = logbase2(tex->pitchw);
	umask = (1<<ubits)-1;
	vbits = logbase2(tex->pitchh);
	vmask = (1<<vbits)-1;
	vmask <<= ubits;

	if (ubits+vbits>16) {
		draw_render_textured8_pc2opaque(surf, dst_line, segment, x1, x2);
		return;
	}

	palette = tex->palettes[segment->tex_num_pal];
	alpha_pal = tex->alpha_palettes[segment->tex_num_pal];
	tex_pixels = tex->pixels;

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

	/* for signed d0:w addressing */
	tex_pixels1 = tex_pixels;
	if (ubits+vbits>15) {
		tex_pixels1 += 32768;
	}

	vbits1 = 16-vbits;

	for (i=x1; i<=x2; i+=16) {
		int j;
		float uuf, vvf, uu2f, vv2f;
		Uint32 dui, dvi, uu, vv;

		dx = MIN(x2-i+1,16);
		/*if (dx==16) {*/
			u2 = u1 + du16;
			v2 = v1 + dv16;
			w2 = w1 + dw16;
		/*} else {
			u2 = u1 + du * dx;
			v2 = v1 + dv * dx;
			w2 = w1 + dw * dx;
		}*/

		invw = 65536.0f / w1;
		uuf = u1 * invw;
		vvf = v1 * invw;
		invw = 65536.0f / w2;
		uu2f = u2 * invw;
		vv2f = v2 * invw;

		/*if (dx==16) {*/
			dui = (uu2f-uuf)/16.0f;
			dvi = (vv2f-vvf)/16.0f;
		/*} else {
			dui = (uu2f-uuf)/dx;
			dvi = (vv2f-vvf)/dx;
		}*/

		uu = uuf;
		vv = vvf;

/*	XXxxYYyy	uv, duv
	XXxx--YY	lsr.w
	xx--YYXX	rol.l	*/

		/* Write first single pixel */
		if (dx & 1) {
			Uint8 c;
			Uint32 pu,pv;

			pu = uu>>16;		/* 0000XXXX */
			pu &= umask;		/* 0000---X */
			pv = vv>>(16-ubits);	/* 000YYYYy */
			pv &= vmask;		/* 000YYYY- */

#ifdef FORCE_OPAQUE
			*dst_col++ = palette[tex_pixels[pv|pu]];
#else
			c = tex_pixels[pv|pu];
			if (alpha_pal[c]) {
				*dst_col = palette[c];
			}
			dst_col++;
#endif
			uu += dui;
			vv += dvi;

			--dx;
		}

		if (dx) {
			Uint32 uv, duv;

			uv = (uu<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
			uv |= (vv>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */
			duv = (dui<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
			duv |= (dvi>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */

			/* for signed d0:w addressing */
			if (ubits+vbits>15) {
				uv ^= 0x8000;
			}

			dx >>= 1;

__asm__ __volatile__ (
	"movel	%5,d4\n\t"
	"lsrw	%7,d4\n\t"
	"roll	%6,d4\n\t"
	"moveql	#0,d5\n\t"
	"moveql	#0,d1\n"

"0:\n\t"
	"moveb	%2@(0,d4:w),d5\n\t"
	"movel	%5,d0\n\t"

	"moveb	%3@(3,d5:w*4),d4\n\t"
	"lsrw	%7,d0\n\t"

	"moveb	d4,%0@+\n\t"
	"roll	%6,d0\n\t"

	"addal	%4,%5\n\t"
	"moveb	%2@(0,d0:w),d1\n\t"

	"movel	%5,d4\n\t"
	"moveb	%3@(3,d1:w*4),d0\n\t"

	"lsrw	%7,d4\n\t"
	"moveb	d0,%0@+\n\t"

	"roll	%6,d4\n\t"
	"orw	d5,d5\n\t"

	"subqw	#1,%1\n\t"
	"addal	%4,%5\n\t"

	"bpls	0b\n"

	: /* output */
		"+a"(dst_col) /*%0*/
	: /* input */
		"d"(dx) /*%1*/, "a"(tex_pixels1) /*%2*/, "a"(palette) /*%3*/,
		"a"(duv) /*%4*/, "a"(uv) /*%5*/, "d"(ubits) /*%6*/, "d"(vbits1) /*%7*/
	: /* clobbered registers */
		"d0", "d1", "d2", "d4", "d5", "cc", "memory" 
);
		}

		u1 = u2;
		v1 = v2;
		w1 = w2;
	}
}

void draw_render_textured8_pc3opaquem68k(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, du,dv, u,v;
	float w1, w2, w, dw;
	int dxtotal, dx, i;
	Uint32 ubits, umask, vbits, vmask;
	render_texture_t *tex = segment->texture;
	Uint8 *dst_col = dst_line;
	Uint32 *palette;
	Uint8 *alpha_pal;
	Uint8 *tex_pixels;

	if (!tex->paletted)
		return;

	ubits = logbase2(tex->pitchw);
	umask = (1<<ubits)-1;
	vbits = logbase2(tex->pitchh);
	vmask = (1<<vbits)-1;
	vmask <<= ubits;

	if (ubits+vbits>16) {
		draw_render_textured8_pc3opaque(surf, dst_line, segment, x1, x2);
		return;
	}

	palette = tex->palettes[segment->tex_num_pal];
	alpha_pal = tex->alpha_palettes[segment->tex_num_pal];
	tex_pixels = tex->pixels;

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

	u = u1 + du * dx;
	v = v1 + dv * dx;
	w = w1 + dw * dx;

	for (i=x1; i<=x2; i++) {
		Uint8 c;
		Uint32 pu,pv;
		float invw;

		invw = 65536.0f / w;
		pu = u * invw;	/* XXXXxxxx */
		pv = v * invw;	/* YYYYyyyy */

		pu >>= 16;		/* 0000XXXX */
		pv >>= 16-ubits;	/* 000YYYYy */
		pu &= umask;		/* 0000---X */
		pv &= vmask;		/* 000YYYY- */

		c = tex_pixels[pv|pu];

#ifdef FORCE_OPAQUE
		*dst_col++ = palette[c];
#else
		if (alpha_pal[c]) {
			*dst_col = palette[c];
		}
		dst_col++;
#endif
		u += du;
		v += dv;
		w += dw;
	}
}
