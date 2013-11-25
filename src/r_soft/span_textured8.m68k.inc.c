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

	dx = x2 - x1 + 1;

	{
		Uint32 uv, duv;
		int dx1, vbits1;
		Uint8 *tex_pixels1 = tex_pixels;

		uv = (u<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		uv |= (v>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */
		duv = (du<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		duv |= (dv>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */

		/* for signed d0:w addressing */
		if (ubits+vbits>15) {
			tex_pixels1 += 32768;
			uv ^= 0x8000;
		}

		vbits1 = 16-vbits;

		dx1 = (dx>>1)-1;

__asm__ __volatile__ (
	"movel	%5,d4\n\t"
	"lsrw	%7,d4\n\t"
	"roll	%6,d4\n\t"

	"moveql	#0,d5\n\t"
	"moveql	#0,d1\n\t"

	"movel	%8,d2\n\t"	/* read dx */

	/* Align on even address */
	"movel	%0,d0\n\t"
	"btst	#0,d0\n\t"	/* if dst_col & 1 */
	"beqs	2f\n\t"		/* ==0, even adress, jump */

	"addal	%4,%5\n\t"	/* else draw pixel */
	"moveb	%2@(0,d4:w),d1\n\t"
	"movel	%5,d4\n\t"
	"moveb	%3@(3,d1:w*4),%0@+\n\t"
	"lsrw	%7,d4\n\t"
	"roll	%6,d4\n\t"

	"subql	#1,d2\n\t"	/* --dx */
	"beqs	1f\n\t"		/* if dx==0 stop */
	"movel	d2,%8\n"
"2:\n\t"
	"cmpl	#2,d2\n\t"	/* if dx<2 draw remaining pixel */
	"bmis	4f\n\t"

	"lsrl	#1,d2\n\t"
	"subq	#1,d2\n\t"
	"movel	d2,%1\n"	/* dx1=(dx>>1)-1 */

	/* Loop */
"0:\n\t"
	"moveb	%2@(0,d4:w),d5\n\t"
	"movel	%5,d0\n\t"

	"moveb	%3@(3,d5:w*4),d2\n\t"
	"lsrw	%7,d0\n\t"

	"addal	%4,%5\n\t"
	"roll	%6,d0\n\t"

	"lslw	#8,d2\n\t"
	"movel	%5,d4\n\t"

	"moveb	%2@(0,d0:w),d1\n\t"
	"lsrw	%7,d4\n\t"

	"moveb	%3@(3,d1:w*4),d2\n\t"
	"roll	%6,d4\n\t"

	"move	d2,%0@+\n\t"
	"addal	%4,%5\n\t"

	"dbra	%1,0b\n\t"

	/* Remaining pixel, if any */
	"movel	%8,d0\n\t"
	"btst	#0,d0\n\t"
	"beqs	1f\n"
"4:\n\t"
	"movel	%5,d0\n\t"
	"lsrw	%7,d0\n\t"
	"roll	%6,d0\n\t"
	"moveb	%2@(0,d0:w),d1\n\t"
	"moveb	%3@(3,d1:w*4),%0@+\n"
"1:\n"

	: /* output */
		"+a"(dst_col) /*%0*/
	: /* input */
		"d"(dx1) /*%1*/, "a"(tex_pixels1) /*%2*/, "a"(palette) /*%3*/,
		"a"(duv) /*%4*/, "a"(uv) /*%5*/, "d"(ubits) /*%6*/, "d"(vbits1) /*%7*/,
		"g"(dx) /*%8*/
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

	dx = x2 - x1 + 1;

	{
		Uint32 uv, duv;
		int dx1, vbits1;
		Uint8 *tex_pixels1 = tex_pixels;

		uv = (u<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		uv |= (v>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */
		duv = (du<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		duv |= (dv>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */

		/* for signed d0:w addressing */
		if (ubits+vbits>15) {
			tex_pixels1 += 32768;
			uv ^= 0x8000;
		}

		vbits1 = 16-vbits;

		dx1 = (dx>>1)-1;

__asm__ __volatile__ (
	"movel	%5,d4\n\t"
	"lsrw	%7,d4\n\t"
	"roll	%6,d4\n\t"

	"moveql	#0,d5\n\t"
	"moveql	#0,d1\n\t"

	"movel	%8,d2\n\t"	/* read dx */

	/* Align on even address */
	"movel	%0,d0\n\t"
	"btst	#0,d0\n\t"	/* if dst_col & 1 */
	"beqs	2f\n\t"		/* ==0, even adress, jump */

	"addal	%4,%5\n\t"	/* else draw pixel */
	"moveb	%2@(0,d4:w),d1\n\t"
	"movel	%5,d4\n\t"
	"moveb	%3@(3,d1:w*4),%0@+\n\t"
	"lsrw	%7,d4\n\t"
	"roll	%6,d4\n\t"

	"subql	#1,d2\n\t"	/* --dx */
	"beqs	1f\n\t"		/* if dx==0 stop */
	"movel	d2,%8\n"
"2:\n\t"
	"cmpl	#2,d2\n\t"	/* if dx<2 draw remaining pixel */
	"bmis	4f\n\t"

	"lsrl	#1,d2\n\t"
	"subq	#1,d2\n\t"
	"movel	d2,%1\n"	/* dx1=(dx>>1)-1 */

	/* Loop */
"0:\n\t"
	"moveb	%2@(0,d4:w),d5\n\t"
	"movel	%5,d0\n\t"

	"moveb	%3@(3,d5:w*4),d2\n\t"
	"lsrw	%7,d0\n\t"

	"addal	%4,%5\n\t"
	"roll	%6,d0\n\t"

	"lslw	#8,d2\n\t"
	"movel	%5,d4\n\t"

	"moveb	%2@(0,d0:w),d1\n\t"
	"lsrw	%7,d4\n\t"

	"moveb	%3@(3,d1:w*4),d2\n\t"
	"roll	%6,d4\n\t"

	"move	d2,%0@+\n\t"
	"addal	%4,%5\n\t"

	"dbra	%1,0b\n\t"

	/* Remaining pixel, if any */
	"movel	%8,d0\n\t"
	"btst	#0,d0\n\t"
	"beqs	1f\n"
"4:\n\t"
	"movel	%5,d0\n\t"
	"lsrw	%7,d0\n\t"
	"roll	%6,d0\n\t"
	"moveb	%2@(0,d0:w),d1\n\t"
	"moveb	%3@(3,d1:w*4),%0@+\n"
"1:\n"

	: /* output */
		"+a"(dst_col) /*%0*/
	: /* input */
		"d"(dx1) /*%1*/, "a"(tex_pixels1) /*%2*/, "a"(palette) /*%3*/,
		"a"(duv) /*%4*/, "a"(uv) /*%5*/, "d"(ubits) /*%6*/, "d"(vbits1) /*%7*/,
		"g"(dx) /*%8*/
	: /* clobbered registers */
		"d0", "d1", "d2", "d4", "d5", "cc", "memory" 
);
	}
}

void draw_render_textured8_pc2opaquem68k(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	float u1,v1, u2,v2, du,dv, u,v;
	float w1, w2, dw, w, invw;
	float du16,dv16,dw16;
	int dxtotal, dx, i;
	Uint32 ubits, umask, vbits, vmask;
	render_texture_t *tex = segment->texture;
	Uint8 *dst_col = dst_line;
	Uint32 color;
	Uint32 dui, dvi, uu, vv;
	float uuf, vvf, uu2f, vv2f;
	int vbits1;
	Uint8 *tex_pixels1;
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
		draw_render_textured8_pc2opaque(surf, dst_line, segment, x1, x2);
		return;
	}

	palette = tex->palettes[segment->tex_num_pal];
	alpha_pal = tex->alpha_palettes[segment->tex_num_pal];
	tex_pixels = (Uint8 *) tex->pixels;

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

	vbits1 = 16-vbits;

	/* for signed d0:w addressing */
	tex_pixels1 = tex_pixels;
	if (ubits+vbits>15) {
		tex_pixels1 += 32768;
	}

	/* Align on even address */
	if (((Uint32) dst_col) & 1) {
		Uint8 c;
		Uint32 pu,pv;
		float invw;

		invw = 65536.0f / w1;
		pu = u1 * invw;	/* XXXXxxxx */
		pv = v1 * invw;	/* YYYYyyyy */

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
		u1 += du;
		v1 += dv;
		w1 += dw;

		++x1;
	}

	for (i=x1; x2-i>=16; i+=16) {
		int j;
		Uint32 uv, duv;

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
		uu = uuf;
		vv = vvf;

		uv = (uu<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		uv |= (vv>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */
		duv = (dui<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		duv |= (dvi>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */

		/* for signed d0:w addressing */
		if (ubits+vbits>15) {
			uv ^= 0x8000;
		}

		dx = 7;

__asm__ __volatile__ (
	"movel	%5,d4\n\t"
	"lsrw	%7,d4\n\t"
	"roll	%6,d4\n\t"
	"moveql	#0,d5\n\t"
	"moveql	#0,d1\n"

"0:\n\t"
#if 1
	"moveb	%2@(0,d4:w),d5\n\t"
	"movel	%5,d0\n\t"

	"moveb	%3@(3,d5:w*4),d2\n\t"
	"lsrw	%7,d0\n\t"

	"lslw	#8,d2\n\t"
	"roll	%6,d0\n\t"

	"orw	d5,d5\n\t"
	"addal	%4,%5\n\t"

	"moveb	%2@(0,d0:w),d1\n\t"
	"movel	%5,d4\n\t"

	"moveb	%3@(3,d1:w*4),d2\n\t"
	"lsrw	%7,d4\n\t"

	"move	d2,%0@+\n\t"
	"roll	%6,d4\n\t"

	"subqw	#1,%1\n\t"
	"addal	%4,%5\n\t"

	"bpls	0b\n"
#else
	"moveb	%2@(0,d4:w),d5\n\t"
	"movel	%5,d0\n\t"

	"moveb	%3@(3,d5:w*4),d2\n\t"
	"lsrw	%7,d0\n\t"

	"addal	%4,%5\n\t"
	"roll	%6,d0\n\t"

	"lslw	#8,d2\n\t"
	"movel	%5,d4\n\t"

	"moveb	%2@(0,d0:w),d1\n\t"
	"lsrw	%7,d4\n\t"

	"moveb	%3@(3,d1:w*4),d2\n\t"
	"roll	%6,d4\n\t"

	"move	d2,%0@+\n\t"
	"addal	%4,%5\n\t"

	"dbra	%1,0b\n"
#endif
	: /* output */
		"+a"(dst_col) /*%0*/
	: /* input */
		"d"(dx) /*%1*/, "a"(tex_pixels1) /*%2*/, "a"(palette) /*%3*/,
		"a"(duv) /*%4*/, "a"(uv) /*%5*/, "d"(ubits) /*%6*/, "d"(vbits1) /*%7*/
	: /* clobbered registers */
		"d0", "d1", "d2", "d4", "d5", "cc", "memory" 
);

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
	uu = uuf;
	vv = vvf;

	for ( ; i<=x2; i++) {
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

	{
		int vbits1, ubits1;
		Uint8 *tex_pixels1 = tex_pixels;
		Uint32 uvoffset=0;

		vbits1 = 16-vbits;
		ubits1 = 16-ubits;

		/* for signed d0:w addressing */
		if (ubits+vbits>15) {
			tex_pixels1 += 32768;
			uvoffset = 0x8000;
		}

		dx = x2-x1;

__asm__ __volatile__(
	"fmove.s	%8,fp1\n\t"	/* u */
	"fmove.s	%9,fp2\n\t"	/* du */
	"fmove.s	%10,fp3\n\t"	/* v */
	"fmove.s	%11,fp4\n\t"	/* dv */
	"fmove.s	%12,fp5\n\t"	/* w */
	"fmove.s	%13,fp6\n\t"	/* dw */
	"moveql	#0,d2\n"

"0:\n\t"
	"fmove.s &0f65536,fp0\n\t"
	"fsgldiv.x fp5,fp0\n\t"		/* fp0 = 65536.0 / w */

	"fmove.x	fp1,fp7\n\t"
	"fsglmul.x	fp0,fp7\n\t"
	"fmove.l	fp7,d1\n\t"	/* pu = (int) (u * invw), UUUUuuuu */

	"fmove.x	fp3,fp7\n\t"
	"fsglmul.x	fp0,fp7\n\t"
	"fmove.l	fp7,d0\n\t"	/* pv = (int) (v * invw), VVVVvvvv */

	"lsll	%4,d1\n\t"
	"lsll	%6,d0\n\t"

	"swap	d0\n\t"		/* vvvvVVVV */		/* 0x46005049 */
	"move	d0,d1\n\t"	/* UUUUVVVV */		/* 0xa4c65049 */
	"lsrw	%6,d1\n\t"	/* UUUU00VV */		/* 0xa4c60050 */
	"roll	%5,d1\n\t"	/* UU00VVUU */		/* 0xc60050a4 */

	"fadd.x	fp2,fp1\n\t"
	"addw	%7,d1\n\t"

	"fadd.x	fp4,fp3\n\t"
	"moveb	%2@(0,d1:w),d2\n\t"

	"fadd.x	fp6,fp5\n\t"
	"moveb	%3@(3,d2:w*4),%0@+\n\t"

	"dbra	%1,0b\n"

	: /* output */
		"+a"(dst_col) /*%0*/
	: /* input */
		"d"(dx) /*%1*/, "a"(tex_pixels1) /*%2*/, "a"(palette) /*%3*/,
		"d"(ubits1) /*%4*/, "d"(ubits) /*%5*/, "d"(vbits1) /*%6*/,
		"r"(uvoffset) /*%7*/,
		"m"(u) /*%8*/, "m"(du) /*%9*/, "m"(v) /*%10*/, "m"(dv) /*%11*/,
		"m"(w) /*%12*/, "m"(dw) /*%13*/
	: /* clobbered registers */
		"fp0", "fp1", "fp2", "fp3", "fp4", "fp5", "fp6", "fp7",
		"d0", "d1", "d2",
		"cc", "memory"
);

	}
}
