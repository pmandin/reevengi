/*	XXxxYYyy	uv, duv
	XXxx--YY	lsr.w
	xx--YYXX	rol.l	*/

void draw_render_textured8_pc0opaquem68k(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
	int dxtotal, dx, i;
	render_texture_t *tex = segment->texture;
	Uint32 ui,vi,dui,dvi;
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

__asm__ __volatile(
	"fmove.s &0f65536,fp0\n\t"
	"fmove.l %14,fp3\n\t"

	"fmove.s %10,fp1\n\t"
	"fsglmul.x fp0,fp1\n\t"
	"fmove.s fp1,%0\n\t"	/* u1 = segment->start.u * 65536.0 */

	"fmove.s %12,fp2\n\t"
	"fsglmul.x fp0,fp2\n\t"
	"fmove.s fp2,%2\n\t"	/* u2 = segment->end.u * 65536.0 */

	"fsub.x fp1,fp2\n\t"
	"fsgldiv.x fp3,fp2\n\t"
/*	"fmove.s fp2,%4\n\t"*/	/* du = (u2-u1)/dxtotal */
	"fmove.l fp2,%8\n\t"	/* dui = du */

	"fmove.s %11,fp1\n\t"
	"fmul.x fp0,fp1\n\t"
	"fmove.s fp1,%1\n\t"	/* v1 = segment->start.v * 65536.0 */

	"fmove.s %13,fp4\n\t"
	"fmul.x fp0,fp4\n\t"
	"fmove.s fp4,%3\n\t"	/* v2 = segment->end.v * 65536.0 */

	"fsub.x fp1,fp4\n\t"
	"fsgldiv.x fp3,fp4\n\t"
/*	"fmove.s fp4,%5\n\t"*/	/* dv = (v2-v1)/dxtotal */
	"fmove.l fp4,%9\n\t"	/* dvi = dv */

	"fmove.l %15,fp0\n\t"
	"fmove.x fp0,fp1\n\t"

	"fsglmul.x fp3,fp0\n\t"
	"fadd.s %0,fp0\n\t"
	"fmove.l fp0,%6\n\t"	/* ui = u1 + du * dx */

	"fsglmul.x fp4,fp1\n\t"
	"fadd.s %1,fp1\n\t"
	"fmove.l fp1,%7\n"	/* vi = v1 + dv * dx */

	: /* output */
		"=m"(u1) /*%0*/, "=m"(v1) /*%1*/,
		"=m"(u2) /*%2*/, "=m"(v2) /*%3*/,
		"=m"(du) /*%4*/, "=m"(dv) /*%5*/,
		"=m"(ui) /*%6*/, "=m"(vi) /*%7*/,
		"=m"(dui) /*%8*/, "=m"(dvi) /*%9*/
	: /* input */
		"m"(segment->start.u) /*%10*/, "m"(segment->start.v) /*%11*/,
		"m"(segment->end.u) /*%12*/, "m"(segment->end.v) /*%13*/,
		"d"(dxtotal) /*%14*/, "d"(dx) /*%15*/
	: /* clobbered registers */
		"fp0", "fp1", "fp2", "fp3", "fp4", "cc", "memory" 
);

	dx = x2 - x1 + 1;

	{
		Uint32 uv, duv;
		int dx1, vbits1;
		Uint8 *tex_pixels1 = tex_pixels;

		uv = (ui<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		uv |= (vi>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */
		duv = (dui<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		duv |= (dvi>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */

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
	"addal	%4,%5\n\t"
	"roll	%6,d4\n\t"
	"moveql	#0,d5\n\t"
	"moveql	#0,d1\n\t"

	"movel	%0,d0\n\t"	/* read dst_col */
	"movel	%8,d2\n\t"	/* read dx */

	/* Align on even address */
	"btst	#0,d0\n\t"	/* if dst_col & 1 */
	"beqs	2f\n"		/* ==0, even adress, jump */
"\n\t"
	"moveb	%2@(0,d4:w),d1\n\t"/* else draw pixel */
	"movel	%5,d4\n\t"
	"moveb	%3@(3,d1:w*4),%0@+\n\t"
	"lsrw	%7,d4\n\t"
	"addal	%4,%5\n\t"
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
	"moveb	%2@(0,d4:w),d1\n\t"
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
	float invw;
	int dxtotal, dx, i;
	render_texture_t *tex = segment->texture;
	Uint32 ui,vi,dui,dvi;
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

__asm__ __volatile(
	"fmove.s &0f65536,fp0\n\t"
	"fsgldiv.s %16,fp0\n\t"	/* fp0 = 65536.0 / w1 = invw1 */

	"fmove.s &0f65536,fp6\n\t"
	"fsgldiv.s %17,fp6\n\t"	/* fp6 = 65536.0 / w2 = invw2 */

	"fmove.l %14,fp3\n\t"

	"fmove.s %10,fp1\n\t"
	"fsglmul.x fp0,fp1\n\t"
	"fmove.s fp1,%0\n\t"	/* u1 = segment->start.u * invw1 */

	"fmove.s %12,fp2\n\t"
	"fsglmul.x fp6,fp2\n\t"
	"fmove.s fp2,%2\n\t"	/* u2 = segment->end.u * invw2 */

	"fsub.x fp1,fp2\n\t"
	"fsgldiv.x fp3,fp2\n\t"
	"fmove.s fp2,%4\n\t"	/* du = (u2-u1)/dxtotal */
	"fmove.l fp2,%8\n\t"	/* dui = du */

	"fmove.s %11,fp1\n\t"
	"fmul.x fp0,fp1\n\t"
	"fmove.s fp1,%1\n\t"	/* v1 = segment->start.v * invw1 */

	"fmove.s %13,fp4\n\t"
	"fmul.x fp6,fp4\n\t"
	"fmove.s fp4,%3\n\t"	/* v2 = segment->end.v * invw2 */

	"fsub.x fp1,fp4\n\t"
	"fsgldiv.x fp3,fp4\n\t"
	"fmove.s fp4,%5\n\t"	/* dv = (v2-v1)/dxtotal */
	"fmove.l fp4,%9\n\t"	/* dvi = dv */

	"fmove.l %15,fp0\n\t"
	"fmove.x fp0,fp1\n\t"

	"fsglmul.x fp3,fp0\n\t"
	"fadd.s %0,fp0\n\t"
	"fmove.l fp0,%6\n\t"	/* ui = u1 + du * dx */

	"fsglmul.x fp4,fp1\n\t"
	"fadd.s %1,fp1\n\t"
	"fmove.l fp1,%7\n"	/* vi = v1 + dv * dx */

	: /* output */
		"=m"(u1) /*%0*/, "=m"(v1) /*%1*/,
		"=m"(u2) /*%2*/, "=m"(v2) /*%3*/,
		"=m"(du) /*%4*/, "=m"(dv) /*%5*/,
		"=m"(ui) /*%6*/, "=m"(vi) /*%7*/,
		"=m"(dui) /*%8*/, "=m"(dvi) /*%9*/
	: /* input */
		"m"(segment->start.u) /*%10*/, "m"(segment->start.v) /*%11*/,
		"m"(segment->end.u) /*%12*/, "m"(segment->end.v) /*%13*/,
		"d"(dxtotal) /*%14*/, "d"(dx) /*%15*/,
		"m"(segment->start.w) /*%16*/, "m"(segment->end.w) /*%17*/
	: /* clobbered registers */
		"fp0", "fp1", "fp2", "fp3", "fp4", "fp5", "fp6", "cc", "memory" 
);

	dx = x2 - x1 + 1;

	{
		Uint32 uv, duv;
		int dx1, vbits1;
		Uint8 *tex_pixels1 = tex_pixels;

		uv = (ui<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		uv |= (vi>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */
		duv = (dui<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		duv |= (dvi>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */

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
	"addal	%4,%5\n\t"
	"roll	%6,d4\n\t"
	"moveql	#0,d5\n\t"
	"moveql	#0,d1\n\t"

	"movel	%0,d0\n\t"	/* read dst_col */
	"movel	%8,d2\n\t"	/* read dx */

	/* Align on even address */
	"btst	#0,d0\n\t"	/* if dst_col & 1 */
	"beqs	2f\n"		/* ==0, even adress, jump */
"\n\t"
	"moveb	%2@(0,d4:w),d1\n\t"/* else draw pixel */
	"movel	%5,d4\n\t"
	"moveb	%3@(3,d1:w*4),%0@+\n\t"
	"lsrw	%7,d4\n\t"
	"addal	%4,%5\n\t"
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
	"moveb	%2@(0,d4:w),d1\n\t"
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
	float invw1, invw2;
	float du16,dv16,dw16;
	int dxtotal, dx, i;
	Uint32 ubits, umask, vbits, vmask;
	render_texture_t *tex = segment->texture;
	Uint8 *dst_col = dst_line;
	Uint32 uv, duv;
	Uint32 dui, dvi, ui, vi;
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

__asm__ __volatile__ (
	"fmove.s	&0f65536,fp0\n\t"
	"fsgldiv.x %6,fp0\n\t"	/* invw1 = 65536.0 / w1 */

	"fsglmul.x fp0,%4\n\t"
	"fmove.s %4,%0\n\t"	/* uuf = u1 * invw1 */

	"fmove.l %4,%2\n\t"	/* ui = (int) uuf */

	"fsglmul.x fp0,%5\n\t"
	"fmove.s %5,%1\n\t"	/* vvf = v1 * invw1 */

	"fmove.l %5,%3\n"	/* vi = (int) vvf */

	: /* output */
		"=m"(uuf) /*%0*/, "=m"(vvf) /*%1*/,
		"=m"(ui) /*%2*/, "=m"(vi) /*%3*/
	: /* input */
		"f"(u1) /*%4*/, "f"(v1) /*%5*/, "f"(w1) /*%6*/
	: /* clobbered registers */
		"fp0", "cc", "memory" 
);

	/* Align dest on multiple of 4 */
	if (((Uint32) dst_col) & 3) {
		int num_pix, numpix_max;

		uv = (ui<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		uv |= (vi>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */
		duv = (dui<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		duv |= (dvi>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */

		/* for signed d0:w addressing */
		if (ubits+vbits>15) {
			uv ^= 0x8000;
		}

		num_pix = 4-(((Uint32) dst_col) & 3);
		numpix_max = x2-x1+1;
		if (num_pix>numpix_max) {
			num_pix = numpix_max;
		}

__asm__ __volatile__ (
	"movel	%3,d4\n\t"
	"movel	%6,d7\n\t"
	"lsrw	%5,d4\n\t"
	"roll	%4,d4\n\t"
	"subql	#1,d7\n\t"
	"moveql	#0,d5\n"

"0:\n\t"
	"moveb	%1@(0,d4:w),d5\n\t"
	"addal	%7,%3\n\t"

	"moveb	%2@(3,d5:w*4),%0@+\n\t"
	"movel	%3,d4\n\t"

	"lsrw	%5,d4\n\t"

	"roll	%4,d4\n\t"

	"dbra	d7,0b\n"

	: /* output */
		"+a"(dst_col) /*%0*/
	: /* input */
		"a"(tex_pixels1) /*%1*/, "a"(palette) /*%2*/,
		"a"(uv) /*%3*/, "d"(ubits) /*%4*/, "d"(vbits1) /*%5*/,
		"d"(num_pix) /*%6*/, "a"(duv) /*%7*/
	: /* clobbered registers */
		"d4", "d5", "d7", "cc", "memory" 
);

		u1 += du * num_pix;
		v1 += dv * num_pix;
		w1 += dw * num_pix;

		x1 += num_pix;

__asm__ __volatile__ (
	"fmove.s	&0f65536,fp0\n\t"
	"fsgldiv.x %4,fp0\n\t"	/* invw1 = 65536.0 / w1 */

	"fsglmul.x fp0,%2\n\t"
	"fmove.s %2,%0\n\t"	/* uuf = u1 * invw1 */

	"fsglmul.x fp0,%3\n\t"
	"fmove.s %3,%1\n\t"	/* vvf = v1 * invw1 */

	: /* output */
		"=m"(uuf) /*%0*/, "=m"(vvf) /*%1*/
	: /* input */
		"f"(u1) /*%2*/, "f"(v1) /*%3*/, "f"(w1) /*%4*/
	: /* clobbered registers */
		"fp0", "cc", "memory" 
);
	}

	dx = x2-16 - x1 + 1;
	i = x1;

	u2 = u1 + du16;
	v2 = v1 + dv16;
	w2 = w1 + dw16;

__asm__ __volatile__ (
	"fmove.s	&0f65536,fp0\n\t"
	"fsgldiv.x %8,fp0\n\t"	/* invw2 = 65536.0 / w2 */

	"fsglmul.x fp0,%6\n\t"
	"fmove.s %6,%4\n\t"	/* uu2f = u2 * invw2 */

	"fmove.s %9,fp1\n\t"	/* uuf */
	"fmove.l fp1,%0\n\t"

	"fsub.x fp1,%6\n\t"	/* uu2f-uuf */
	"fmove.l %6,d0\n\t"
	"asrl	#4,d0\n\t"
	"movel	d0,%2\n\t"	/* dui = (uu2f-uuf)/16.0f */

	"fsglmul.x fp0,%7\n\t"
	"fmove.s %7,%5\n\t"	/* vv2f = v2 * invw2 */

	"fmove.s %10,fp1\n\t"	/* vvf */
	"fmove.l fp1,%1\n\t"

	"fsub.x fp1,%7\n\t"	/* vv2f-vvf */
	"fmove.l %7,d0\n\t"
	"asrl	#4,d0\n\t"
	"movel	d0,%3\n\t"	/* dvi = (vv2f-vvf)/16.0f */


	: /* output */
		"=m"(ui) /*%0*/, "=m"(vi) /*%1*/,
		"=m"(dui) /*%2*/, "=m"(dvi) /*%3*/,
		"=m"(uu2f) /*%4*/, "=m"(vv2f) /*%5*/
	: /* input */
		"f"(u2) /*%6*/, "f"(v2) /*%7*/, "f"(w2) /*%8*/,
		"m"(uuf) /*%9*/, "m"(vvf) /*%10*/
	: /* clobbered registers */
		"d0", "fp0", "fp1", "cc", "memory" 
);

	if (dx>0) {

		for (i=x1; i<=x2-16; i+=16) {

			uv = (ui<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
			uv |= (vi>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */
			duv = (dui<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
			duv |= (dvi>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */

			/* for signed d0:w addressing */
			if (ubits+vbits>15) {
				uv ^= 0x8000;
			}

			/* Render 16 pixels, calculate for next 16 */

/* Splitted because inline asm with gcc allows max 30 operands */

#define ASM_dst_col	"%0"	/*rw*/
#define ASM_texpixels1	"%12"	/*r*/
#define ASM_palette "%10"	/*r*/
#define ASM_duv	"%11"		/*r*/
#define ASM_uv	"%9"		/*w*/
#define ASM_ubits "%13"		/*r*/
#define ASM_vbits1 "%14"	/*r*/
#define ASM_u1	"%3"		/*w*/
#define ASM_v1	"%4"		/*w*/
#define ASM_w1	"%5"		/*w*/
#define ASM_u2	"%6"		/*rw*/
#define ASM_v2	"%7"		/*rw*/
#define ASM_w2	"%8"		/*rw*/
#define ASM_du16	"%17"	/*r*/
#define ASM_dv16	"%18"	/*r*/
#define ASM_dw16	"%19"	/*r*/
#define ASM_uuf	"%1"		/*w*/
#define ASM_vvf	"%2"		/*w*/
#define ASM_uu2f	"%15"	/*r*/
#define ASM_vv2f	"%16"	/*r*/

__asm__ __volatile__ (
	/* Init rendering */
	"movel	" ASM_uv ",d4\n\t"
	"lsrw	" ASM_vbits1 ",d4\n\t"
	"addal	" ASM_duv "," ASM_uv "\n\t"
	"roll	" ASM_ubits ",d4\n\t"
	"moveql	#0,d5\n\t"
	"moveql	#0,d1\n\t"

	/* Pixels 0,1 */
	"fmove.s	" ASM_u2 ",fp0\n\t"
	"moveb	" ASM_texpixels1 "@(0,d4:w),d5\n\t"
	"movel	" ASM_uv ",d0\n\t"

	"fmove.s	fp0," ASM_u1 "\n\t"	/* u1 = u2, 2 cycles */
	"moveb	" ASM_palette "@(3,d5:w*4),d2\n\t"
	"lsrw	" ASM_vbits1 ",d0\n\t"

	"fmove.s	" ASM_v2 ",fp1\n\t"
	"addal	" ASM_duv "," ASM_uv "\n\t"
	"roll	" ASM_ubits ",d0\n\t"

	"lsll	#8,d2\n\t"
	"movel	" ASM_uv ",d4\n\t"

	"fmove.s	fp1," ASM_v1 "\n\t"	/* v1 = v2, 2 cycles */
	"moveb	" ASM_texpixels1 "@(0,d0:w),d1\n\t"
	"lsrw	" ASM_vbits1 ",d4\n\t"

	"fmove.s	" ASM_w2 ",fp2\n\t"
	"moveb	" ASM_palette "@(3,d1:w*4),d2\n\t"
	"roll	" ASM_ubits ",d4\n\t"

	"fmove.s	fp2," ASM_w1 "\n\t"	/* w1 = w2, 2 cycles */
	"lsll	#8,d2\n\t"
	"addal	" ASM_duv "," ASM_uv "\n\t"

	/* Pixels 2,3 */
	"fadd.x	" ASM_du16 ",fp0\n\t"		/* fp0 += du16, 3 cycles */
	"moveb	" ASM_texpixels1 "@(0,d4:w),d5\n\t"
	"movel	" ASM_uv ",d0\n\t"

	"moveb	" ASM_palette "@(3,d5:w*4),d2\n\t"
	"lsrw	" ASM_vbits1 ",d0\n\t"

	"fmove.s	fp0," ASM_u2 "\n\t"	/* u2 = u1+du16, 2 cycles */
	"addal	" ASM_duv "," ASM_uv "\n\t"
	"roll	" ASM_ubits ",d0\n\t"

	"lsll	#8,d2\n\t"
	"movel	" ASM_uv ",d4\n\t"

	"fadd.x	" ASM_dv16 ",fp1\n\t"		/* fp1 += dv16, 3 cycles */
	"moveb	" ASM_texpixels1 "@(0,d0:w),d1\n\t"
	"lsrw	" ASM_vbits1 ",d4\n\t"

	"moveb	" ASM_palette "@(3,d1:w*4),d2\n\t"
	"roll	" ASM_ubits ",d4\n\t"

	"fmove.s	fp1," ASM_v2 "\n\t"	/* v2 = v1+dv16, 2 cycles */
	"movel	d2," ASM_dst_col "@+\n\t"
	"addal	" ASM_duv "," ASM_uv "\n\t"

	"move.l	" ASM_uu2f "," ASM_uuf "\n\t"	/* uuf = uu2f */
	"move.l	" ASM_vv2f "," ASM_vvf "\n\t"	/* vvf = vv2f */

	/* Pixels 4,5 */
	"fadd.x	" ASM_dw16 ",fp2\n\t"		/* fp2 += dw16, 3 cycles */
	"moveb	" ASM_texpixels1 "@(0,d4:w),d5\n\t"
	"movel	" ASM_uv ",d0\n\t"

	"moveb	" ASM_palette "@(3,d5:w*4),d2\n\t"
	"lsrw	" ASM_vbits1 ",d0\n\t"

	"fmove.s	fp2," ASM_w2 "\n\t"	/* w2 = w1+dw16, 2 cycles */
	"addal	" ASM_duv "," ASM_uv "\n\t"
	"roll	" ASM_ubits ",d0\n\t"

	"lslw	#8,d2\n\t"
	"movel	" ASM_uv ",d4\n\t"

	"fmove.s	&0f65536,fp3\n\t"
	"moveb	" ASM_texpixels1 "@(0,d0:w),d1\n\t"
	"lsrw	" ASM_vbits1 ",d4\n\t"

	"fsgldiv.x fp2,fp3\n\t"	/* invw2 = 65536.0 / w2, 37 cycles */
	"moveb	" ASM_palette "@(3,d1:w*4),d2\n\t"
	"roll	" ASM_ubits ",d4\n\t"

	"move	d2," ASM_dst_col "@+\n\t"
	"addal	" ASM_duv "," ASM_uv "\n"

	: /* output */
		"+a"(dst_col) /*%0*/,
		"=m"(uuf) /*%1*/, "=m"(vvf) /*%2*/,
		"=m"(u1) /*%3*/, "=m"(v1) /*%4*/, "=m"(w1) /*%5*/,
		"+m"(u2) /*%6*/, "+m"(v2) /*%7*/, "+m"(w2) /*%8*/,
		"+a"(uv) /*%9*/
	: /* input */
		"a"(palette) /*%10*/,
		"a"(duv) /*%11*/, "a"(tex_pixels1) /*%12*/, "d"(ubits) /*%13*/, "d"(vbits1) /*%14*/,
		"m"(uu2f) /*%15*/, "m"(vv2f) /*%16*/,
		"f"(du16) /*%17*/, "f"(dv16) /*%18*/, "f"(dw16) /*%19*/
	: /* clobbered registers */
		"d0", "d1", "d2", "d4", "d5", "d7",
		"fp0", "fp1", "fp2", "fp3",
		"cc", "memory" 
);

#undef ASM_dst_col
#undef ASM_texpixels1
#undef ASM_palette
#undef ASM_duv
#undef ASM_uv
#undef ASM_ubits
#undef ASM_vbits1
#undef ASM_u1
#undef ASM_v1
#undef ASM_w1
#undef ASM_u2
#undef ASM_v2
#undef ASM_w2
#undef ASM_du16
#undef ASM_dv16
#undef ASM_dw16
#undef ASM_uuf
#undef ASM_vvf
#undef ASM_uu2f
#undef ASM_vv2f


#define ASM_dst_col	"%0"	/*rw*/
#define ASM_texpixels1	"%2"	/*r*/
#define ASM_palette "%3"	/*r*/
#define ASM_duv	"%4"		/*r*/
#define ASM_uv	"%1"		/*w*/
#define ASM_ubits "%5"		/*r*/
#define ASM_vbits1 "%6"		/*r*/

__asm__ __volatile__ (

	/* Pixels 6,7 8,9, 10,11, 12,13 */
	"moveq	#1,d7\n"
"0:\n\t"
	"moveb	" ASM_texpixels1 "@(0,d4:w),d5\n\t"
	"movel	" ASM_uv ",d0\n\t"

	"moveb	" ASM_palette "@(3,d5:w*4),d2\n\t"
	"lsrw	" ASM_vbits1 ",d0\n\t"

	"addal	" ASM_duv "," ASM_uv "\n\t"
	"roll	" ASM_ubits ",d0\n\t"

	"lsll	#8,d2\n\t"
	"movel	" ASM_uv ",d4\n\t"

	"moveb	" ASM_texpixels1 "@(0,d0:w),d1\n\t"
	"lsrw	" ASM_vbits1 ",d4\n\t"

	"moveb	" ASM_palette "@(3,d1:w*4),d2\n\t"
	"roll	" ASM_ubits ",d4\n\t"

	"lsll	#8,d2\n\t"
	"addal	" ASM_duv "," ASM_uv "\n\t"

	"moveb	" ASM_texpixels1 "@(0,d4:w),d5\n\t"
	"movel	" ASM_uv ",d0\n\t"

	"moveb	" ASM_palette "@(3,d5:w*4),d2\n\t"
	"lsrw	" ASM_vbits1 ",d0\n\t"

	"addal	" ASM_duv "," ASM_uv "\n\t"
	"roll	" ASM_ubits ",d0\n\t"

	"lsll	#8,d2\n\t"
	"movel	" ASM_uv ",d4\n\t"

	"moveb	" ASM_texpixels1 "@(0,d0:w),d1\n\t"
	"lsrw	" ASM_vbits1 ",d4\n\t"

	"moveb	" ASM_palette "@(3,d1:w*4),d2\n\t"
	"roll	" ASM_ubits ",d4\n\t"

	"movel	d2," ASM_dst_col "@+\n\t"
	"addal	" ASM_duv "," ASM_uv "\n\t"

	"dbra	d7,0b\n\t"

	: /* output */
		"+a"(dst_col) /*%0*/,
		"+a"(uv) /*%1*/
	: /* input */
		"a"(tex_pixels1) /*%2*/, "a"(palette) /*%3*/,
		"a"(duv) /*%4*/, "d"(ubits) /*%5*/, "d"(vbits1) /*%6*/
	: /* clobbered registers */
		"d0", "d1", "d2", "d4", "d5", "d7",
		"fp0", "fp1", "fp2", "fp3",
		"cc", "memory" 
);

#undef ASM_dst_col
#undef ASM_texpixels1
#undef ASM_palette
#undef ASM_duv
#undef ASM_uv
#undef ASM_ubits
#undef ASM_vbits1


#define ASM_dst_col	"%0"	/*rw*/
#define ASM_texpixels1	"%2"	/*r*/
#define ASM_palette "%3"	/*r*/
#define ASM_duv	"%4"		/*r*/
#define ASM_uv	"%1"		/*w*/
#define ASM_ubits "%5"		/*r*/
#define ASM_vbits1 "%6"		/*r*/

__asm__ __volatile__ (

	/* Pixels 14,15 */

	"fsglmul.x fp3,fp0\n\t"	/* uu2f = u2 * invw2, 3 cycles */
	"moveb	" ASM_texpixels1 "@(0,d4:w),d5\n\t"
	"movel	" ASM_uv ",d0\n\t"

	"moveb	" ASM_palette "@(3,d5:w*4),d2\n\t"
	"lsrw	" ASM_vbits1 ",d0\n\t"

	"fsglmul.x fp3,fp1\n\t"	/* vv2f = v2 * invw2, 3 cycles */
	"addal	" ASM_duv "," ASM_uv "\n\t"
	"roll	" ASM_ubits ",d0\n\t"

	"lslw	#8,d2\n\t"
	"movel	" ASM_uv ",d4\n\t"

	"moveb	" ASM_texpixels1 "@(0,d0:w),d1\n\t"
	"lsrw	" ASM_vbits1 ",d4\n\t"

	"moveb	" ASM_palette "@(3,d1:w*4),d2\n\t"
	"roll	" ASM_ubits ",d4\n\t"

	"move	d2," ASM_dst_col "@+\n\t"
	"addal	" ASM_duv "," ASM_uv "\n\t"

	: /* output */
		"+a"(dst_col) /*%0*/,
		"+a"(uv) /*%1*/
	: /* input */
		"a"(tex_pixels1) /*%2*/, "a"(palette) /*%3*/,
		"a"(duv) /*%4*/, "d"(ubits) /*%5*/, "d"(vbits1) /*%6*/
	: /* clobbered registers */
		"d0", "d1", "d2", "d4", "d5",
		"fp0", "fp1", "fp2", "fp3",
		"cc", "memory" 
);

#undef ASM_dst_col
#undef ASM_texpixels1
#undef ASM_palette
#undef ASM_duv
#undef ASM_uv
#undef ASM_ubits
#undef ASM_vbits1

#define ASM_uu2f	"%0"	/*w*/
#define ASM_vv2f	"%1"	/*w*/
#define ASM_ui	"%2"		/*w*/
#define ASM_vi	"%3"		/*w*/
#define ASM_dui		"%4"	/*w*/
#define ASM_dvi		"%5"	/*w*/
#define ASM_uuf		"%6"	/*r*/
#define ASM_vvf		"%7"	/*r*/

__asm__ __volatile__ (

	/* Finish fpu math for next 16 pixels */
	"fmove.s fp0," ASM_uu2f "\n\t"	/* uu2f = u2 * invw2 */

	"fmove.s " ASM_uuf ",fp4\n\t"	/* uuf */
	"fmove.l fp4," ASM_ui "\n\t"	/* ui */

	"fsub.x fp4,fp0\n\t"	/* uu2f-uuf */
	"fmove.l fp0,d0\n\t"
	"asrl	#4,d0\n\t"
	"movel	d0," ASM_dui "\n\t"	/* dui = (uu2f-uuf)/16.0f */

	"fmove.s fp1," ASM_vv2f "\n\t"	/* vv2f = v2 * invw2 */

	"fmove.s " ASM_vvf ",fp3\n\t"	/* vvf */
	"fmove.l fp3," ASM_vi "\n\t"	/* vi */

	"fsub.x fp3,fp1\n\t"	/* vv2f-vvf */
	"fmove.l fp1,d0\n\t"
	"asrl	#4,d0\n\t"
	"movel	d0," ASM_dvi "\n"	/* dvi = (vv2f-vvf)/16.0f */

	: /* output */
		"=m"(uu2f) /*%0*/, "=m"(vv2f) /*%1*/,
		"=m"(ui) /*%2*/, "=m"(vi) /*%3*/,
		"=m"(dui) /*%4*/, "=m"(dvi) /*%5*/
	: /* input */
		"m"(uuf) /*%6*/, "m"(vvf) /*%7*/
	: /* clobbered registers */
		"d0",
		"fp0", "fp1", "fp3", "fp4",
		"cc", "memory" 
);

#undef ASM_uu2f
#undef ASM_vv2f
#undef ASM_ui
#undef ASM_vi
#undef ASM_dui
#undef ASM_dvi
#undef ASM_uuf
#undef ASM_vvf

		}
	}

	/* Draw remaning pixels */
	dx = x2-i+1;

	if (dx>0) {
		int dx1;

		uv = (ui<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		uv |= (vi>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */
		duv = (dui<<(16-ubits)) & 0xffff0000UL;	/* Xxxx0000 */
		duv |= (dvi>>vbits) & 0x0000ffffUL;	/* XxxxYYYy */

		/* for signed d0:w addressing */
		if (ubits+vbits>15) {
			uv ^= 0x8000;
		}

		dx1 = (dx>>1)-1;

__asm__ __volatile__ (
	"movel	%5,d4\n\t"
	"lsrw	%7,d4\n\t"
	"addal	%4,%5\n\t"
	"roll	%6,d4\n\t"
	"moveql	#0,d5\n\t"
	"moveql	#0,d1\n\t"

	"movel	%8,d2\n\t"
	"cmpl	#2,d2\n\t"	/* if dx<2 draw remaining pixel */
	"bmis	4f\n"

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

	"dbra	%1,0b\n"
"\n\t"
	/* Remaining pixel, if any */
	"movel	%8,d0\n\t"
	"btst	#0,d0\n\t"
	"beqs	1f\n"
"4:\n\t"
	"moveb	%2@(0,d4:w),d1\n\t"
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

void draw_render_textured8_pc3opaquem68k(SDL_Surface *surf, Uint8 *dst_line, sbuffer_segment_t *segment, int x1,int x2)
{
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
#if 1
		Uint32 coefu = 1<<(15-ubits);
		Uint32 coefv = 1<<(15-vbits);

		u *= (float) coefu;
		du *= (float) coefu;
		v *= (float) coefv;
		dv *= (float) coefv;
#endif
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

	"fmove.s	&0f65536,fp0\n\t"
	"fsgldiv.x fp5,fp0\n\t"		/* fp0 = 65536.0 / w */

	"fmove.x	fp1,fp7\n\t"
	"fsglmul.x	fp0,fp7\n\t"
	"fmove.l	fp7,d1\n\t"	/* pu = (int) (u * invw), UUUUuuuu */

	"fmove.x	fp3,fp7\n\t"

	"moveql	#0,d2\n"

"0:\n\t"
	/*"fmove.s &0f65536,fp0\n\t"*/		/* 1c */
	/*"fsgldiv.x fp5,fp0\n\t"*/		/* 37c fp0 = 65536.0 / w */

	/*"fmove.x	fp1,fp7\n\t"*/		/* 1c */
	/*"fsglmul.x	fp0,fp7\n\t"*/		/* 3c */
	/*"fmove.l	fp7,d1\n\t"*/		/* 1c */ /* pu = (int) (u * invw), UUUUuuuu */

	/*"fmove.x	fp3,fp7\n\t"*/		/* 1c */
	"fsglmul.x	fp0,fp7\n\t"		/* 3c */

	"fmove.l	fp7,d0\n\t"		/* 1c */ /* pv = (int) (v * invw), VVVVvvvv */
/*	"lsll	%4,d1\n\t"*/
	"lsll	#1,d1\n\t"

	"fadd.x	fp2,fp1\n\t"			/* 3c */
/*	"lsll	%6,d0\n\t"*/
	"lsll	#1,d0\n\t"

	"fadd.x	fp4,fp3\n\t"			/* 3c */
	"swap	d0\n\t"		/* vvvvVVVV */

	"fadd.x	fp6,fp5\n\t"			/* 3c */
	"move	d0,d1\n\t"	/* UUUUVVVV */

	"fmove.s	&0f65536,fp0\n\t"
	"lsrw	%6,d1\n\t"	/* UUUU00VV */

	"fsgldiv.x fp5,fp0\n\t"		/* fp0 = 65536.0 / w */
	"roll	%5,d1\n\t"	/* UU00VVUU */

	"fmove.x	fp1,fp7\n\t"
	"addw	%7,d1\n\t"

	"fsglmul.x	fp0,fp7\n\t"
	"moveb	%2@(0,d1:w),d2\n\t"

	"fmove.l	fp7,d1\n\t"	/* pu = (int) (u * invw), UUUUuuuu */
	"moveb	%3@(3,d2:w*4),%0@+\n\t"

	"fmove.x	fp3,fp7\n\t"
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
