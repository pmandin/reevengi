/*
	RE2
	PS1
	Demo, Game Leon, Game Claire

	Copyright (C) 2007	Patrice Mandin

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

#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#include "filesystem.h"
#include "state.h"
#include "re2_ps1.h"
#include "background_bss.h"
#include "parameters.h"

/*--- Defines ---*/

#define CHUNK_SIZE 65536

/*--- Types ---*/

typedef struct {
	Uint32	offset;
	Uint32	flag;	/* 0: ignore, 1: tim or emd */
} re2ps1_ems_t;

/*--- Constant ---*/

static const char *re2ps1_bg = "common/bss/room%d%02x.bss";
static const char *re2ps1_room = "pl%d/rdt/room%d%02x0.rdt";

static const char *re2ps1demo_movies[] = {
	"zmovie/capcom.str",
	"zmovie/info.str",
	"zmovie/r10b.str",
	NULL
};

static const re2ps1_ems_t re2ps1demo_ems[]={
	{0x00000000, 0},
	{0x00016000, 1},	/* tim */
	{0x00026800, 1},	/* emd */
	{0x0004a800, 1},	/* tim */
	{0x0006b800, 1},	/* emd */
	{0x0008f800, 1},	/* tim */
	{0x000a0000, 1},	/* emd */
	{0x000c4000, 1},	/* tim */
	{0x000e5000, 1},	/* emd */
	{0x00107800, 0},
	{0x00110800, 1},	/* tim */
	{0x00121000, 1},	/* emd */
	{0x0013f000, 0},
	{0x00149000, 1},	/* tim */
	{0x00151800, 1},	/* emd */
	{0x00159800, 0},
	{0x00166800, 1},	/* tim */
	{0x00177000, 1},	/* emd */
	{0x001a3000, 0},
	{0x001ad000, 1},	/* tim */
	{0x001bd800, 1},	/* emd */
	{0x001cb000, 0},
	{0x001cf000, 1},	/* tim */
	{0x001d7800, 1},	/* emd */
	{0x001de000, 1},	/* tim */
	{0x001ee800, 1},	/* emd */
	{0x00202000, 1},	/* tim */
	{0x00212800, 1},	/* emd */
	{0x00222800, 1},	/* tim */
	{0x0023b000, 1},	/* emd */
	{0x00242000, 1},	/* tim */
	{0x0025a800, 1},	/* emd */
	{0x00281800, 1},	/* tim */
	{0x00292000, 1},	/* emd */
	{0x002a5800, 0}		/* file length */
};

static const re2ps1_ems_t re2ps1gamel1_ems[]={
	{0x00000000, 0},
	{0x0001a000, 1},	/* tim */
	{0x0002a800, 1},	/* emd */
	{0x0004e800, 1},	/* tim */
	{0x0005f000, 1},	/* emd */
	{0x00083800, 1},	/* tim */
	{0x000a4800, 1},	/* emd */
	{0x000c8800, 1},	/* tim */
	{0x000d9000, 1},	/* emd */
	{0x000fd000, 1},	/* tim */
	{0x0010d800, 1},	/* emd */
	{0x00131800, 1},	/* tim */
	{0x00142000, 1},	/* emd */
	{0x00166000, 1},	/* tim */
	{0x00176800, 1},	/* emd */
	{0x0019b800, 1},	/* tim */
	{0x001ac000, 1},	/* emd */
	{0x001ce800, 1},	/* tim */
	{0x001ef800, 1},	/* emd */
	{0x00212000, 1},	/* tim */
	{0x00233000, 1},	/* emd */
	{0x00255800, 0},
	{0x00260800, 1},	/* tim */
	{0x00271000, 1},	/* emd */
	{0x0028f000, 0},
	{0x00299000, 1},	/* tim */
	{0x002a1800, 1},	/* emd */
	{0x002a9800, 0},
	{0x002b8800, 1},	/* tim */
	{0x002c9000, 1},	/* emd */
	{0x002f4000, 0},
	{0x002fd000, 1},	/* tim */
	{0x0031e000, 1},	/* emd */
	{0x0034c800, 0},
	{0x00358800, 1},	/* tim */
	{0x00369000, 1},	/* emd */
	{0x00389000, 0},
	{0x00396000, 1},	/* tim */
	{0x003a6800, 1},	/* emd */
	{0x003b4000, 0},
	{0x003b7000, 1},	/* tim */
	{0x003bf800, 1},	/* emd */
	{0x003c1800, 0},
	{0x003c8800, 1},	/* tim */
	{0x003d1000, 1},	/* emd */
	{0x003db000, 0},
	{0x003e8000, 1},	/* tim */
	{0x00409000, 1},	/* emd */
	{0x00437000, 0},
	{0x0043b000, 1},	/* tim */
	{0x00443800, 1},	/* emd */
	{0x00447000, 0},
	{0x00451000, 1},	/* tim */
	{0x00461800, 1},	/* emd */
	{0x00483800, 0},
	{0x0048d800, 1},	/* tim */
	{0x004a6000, 1},	/* emd */
	{0x004c8800, 0},
	{0x004c9800, 1},	/* tim */
	{0x004d2000, 1},	/* emd */
	{0x004d6000, 0},
	{0x004d9000, 1},	/* tim */
	{0x004e1800, 1},	/* emd */
	{0x004e8000, 0},
	{0x004f3000, 1},	/* tim */
	{0x00503800, 1},	/* emd */
	{0x00538800, 0},
	{0x0053d800, 1},	/* tim */
	{0x00546000, 1},	/* emd */
	{0x00547800, 0},
	{0x00556800, 1},	/* tim */
	{0x0056f000, 1},	/* emd */
	{0x00594800, 0},
	{0x005a4800, 1},	/* tim */
	{0x005bd000, 1},	/* emd */
	{0x005ec800, 0},
	{0x005f9800, 1},	/* tim */
	{0x00612000, 1},	/* emd */
	{0x00644800, 0},
	{0x0065a800, 1},	/* tim */
	{0x00673000, 1},	/* emd */
	{0x006b6000, 0},
	{0x006c2000, 1},	/* tim */
	{0x006e2800, 1},	/* emd */
	{0x006fe000, 0},
	{0x0070a000, 1},	/* tim */
	{0x00712800, 1},	/* emd */
	{0x00721000, 0},
	{0x0072c000, 1},	/* tim */
	{0x0073c800, 1},	/* emd */
	{0x00771800, 0},
	{0x0077b800, 1},	/* tim */
	{0x0078c000, 1},	/* emd */
	{0x00797000, 0},
	{0x0079a000, 1},	/* tim */
	{0x007a2800, 1},	/* emd */
	{0x007a3800, 0},
	{0x007a4800, 1},	/* tim */
	{0x007ad000, 1},	/* emd */
	{0x007bb000, 0},
	{0x007bc000, 1},	/* tim */
	{0x007c4800, 1},	/* emd */
	{0x007cb000, 1},	/* tim */
	{0x007e3800, 1},	/* emd */
	{0x007f1000, 1},	/* tim */
	{0x00801800, 1},	/* emd */
	{0x00814800, 1},	/* tim */
	{0x0082d000, 1},	/* emd */
	{0x00840000, 1},	/* tim */
	{0x00858800, 1},	/* emd */
	{0x0086b000, 1},	/* tim */
	{0x0087b800, 1},	/* emd */
	{0x0088e000, 1},	/* tim */
	{0x008a6800, 1},	/* emd */
	{0x008b9000, 1},	/* tim */
	{0x008d1800, 1},	/* emd */
	{0x008de800, 1},	/* tim */
	{0x008f7000, 1},	/* emd */
	{0x008ff000, 1},	/* tim */
	{0x00917800, 1},	/* emd */
	{0x00924800, 1},	/* tim */
	{0x0093d000, 1},	/* emd */
	{0x00964800, 1},	/* tim */
	{0x00975000, 1},	/* emd */
	{0x00987800, 1},	/* tim */
	{0x009a0000, 1},	/* emd */
	{0x009b2800, 1},	/* tim */
	{0x009c3000, 1},	/* emd */
	{0x009d6000, 1},	/* tim */
	{0x009e6800, 1},	/* emd */
	{0x009f9000, 1},	/* tim */
	{0x00a09800, 1},	/* emd */
	{0x00a1c800, 1},	/* tim */
	{0x00a2d000, 1},	/* emd */
	{0x00a3f800, 1},	/* tim */
	{0x00a58000, 1},	/* emd */
	{0x00a6b000, 1},	/* tim */
	{0x00a83800, 1},	/* emd */
	{0x00a97000, 0}		/* file length */
};

static const re2ps1_ems_t re2ps1gamel2_ems[]={
	{0x00000000, 1},	/* tim */
	{0x00010800, 1},	/* emd */
	{0x00034800, 1},	/* tim */
	{0x00055800, 1},	/* emd */
	{0x00079800, 1},	/* tim */
	{0x0008a000, 1},	/* emd */
	{0x000ae000, 1},	/* tim */
	{0x000cf000, 1},	/* emd */
	{0x000f1800, 1},	/* tim */
	{0x00102000, 1},	/* emd */
	{0x00120000, 1},	/* tim */
	{0x00130800, 1},	/* emd */
	{0x0013e000, 1},	/* tim */
	{0x00146800, 1},	/* emd */
	{0x00148800, 1},	/* tim */
	{0x00151000, 1},	/* emd */
	{0x00154000, 1},	/* tim */
	{0x00164800, 1},	/* emd */
	{0x00177000, 1},	/* tim */
	{0x0018f800, 1},	/* emd */
	{0x0019c800, 1},	/* tim */
	{0x001b5000, 1},	/* emd */
	{0x001c2000, 1},	/* tim */
	{0x001da800, 1},	/* emd */
	{0x001ed000, 1},	/* tim */
	{0x001fd800, 1},	/* emd */
	{0x00210000, 1},	/* tim */
	{0x00228800, 1},	/* emd */
	{0x0023b800, 1},	/* tim */
	{0x00254000, 1},	/* emd */
	{0x00267800, 0}		/* file length */
};

static const re2ps1_ems_t re2ps1gamec1_ems[]={
	{0x00000000, 0},
	{0x0001a000, 1},	/* tim */
	{0x0002a800, 1},	/* emd */
	{0x0004e800, 1},	/* tim */
	{0x0005f000, 1},	/* emd */
	{0x00084000, 1},	/* tim */
	{0x000a5000, 1},	/* emd */
	{0x000c9000, 1},	/* tim */
	{0x000d9800, 1},	/* emd */
	{0x000fd800, 1},	/* tim */
	{0x0010e000, 1},	/* emd */
	{0x00132000, 1},	/* tim */
	{0x00142800, 1},	/* emd */
	{0x00166800, 1},	/* tim */
	{0x00177000, 1},	/* emd */
	{0x0019c800, 1},	/* tim */
	{0x001ad000, 1},	/* emd */
	{0x001cf800, 1},	/* tim */
	{0x001f0800, 1},	/* emd */
	{0x00213000, 1},	/* tim */
	{0x00234000, 1},	/* emd */
	{0x00256800, 0},
	{0x00261800, 1},	/* tim */
	{0x00272000, 1},	/* emd */
	{0x00290000, 0},
	{0x0029a000, 1},	/* tim */
	{0x002a2800, 1},	/* emd */
	{0x002aa800, 0},
	{0x002b9800, 1},	/* tim */
	{0x002ca000, 1},	/* emd */
	{0x002f5000, 0},
	{0x002fe000, 1},	/* tim */
	{0x0031f000, 1},	/* emd */
	{0x0034d800, 0},
	{0x00359800, 1},	/* tim */
	{0x0036a000, 1},	/* emd */
	{0x0038a000, 0},
	{0x00397000, 1},	/* tim */
	{0x003a7800, 1},	/* emd */
	{0x003b5000, 0},
	{0x003b8000, 1},	/* tim */
	{0x003c0800, 1},	/* emd */
	{0x003c2800, 0},
	{0x003c9800, 1},	/* tim */
	{0x003d2000, 1},	/* emd */
	{0x003dc000, 0},
	{0x003e9000, 1},	/* tim */
	{0x0040a000, 1},	/* emd */
	{0x00438000, 0},
	{0x0043c000, 1},	/* tim */
	{0x00444800, 1},	/* emd */
	{0x00447800, 0},
	{0x00451800, 1},	/* tim */
	{0x00462000, 1},	/* emd */
	{0x00484000, 0},
	{0x0048e000, 1},	/* tim */
	{0x004a6800, 1},	/* emd */
	{0x004ca000, 0},
	{0x004cb000, 1},	/* tim */
	{0x004d3800, 1},	/* emd */
	{0x004d7800, 0},
	{0x004da800, 1},	/* tim */
	{0x004e3000, 1},	/* emd */
	{0x004e9800, 0},
	{0x004f4800, 1},	/* tim */
	{0x00505000, 1},	/* emd */
	{0x0053a000, 0},
	{0x0053f000, 1},	/* tim */
	{0x00547800, 1},	/* emd */
	{0x00549000, 0},
	{0x00558000, 1},	/* tim */
	{0x00570800, 1},	/* emd */
	{0x00596000, 0},
	{0x005a4800, 1},	/* tim */
	{0x005be800, 1},	/* emd */
	{0x005ee000, 0},
	{0x005fb000, 1},	/* tim */
	{0x00613800, 1},	/* emd */
	{0x00646000, 0},
	{0x0065c000, 1},	/* tim */
	{0x00674800, 1},	/* emd */
	{0x006b7800, 0},
	{0x006c3800, 1},	/* tim */
	{0x006e4000, 1},	/* emd */
	{0x006ff800, 0},
	{0x0070b800, 1},	/* tim */
	{0x00714000, 1},	/* emd */
	{0x00722800, 0},
	{0x0072d800, 1},	/* tim */
	{0x0073e000, 1},	/* emd */
	{0x00773000, 0},
	{0x0077d000, 1},	/* tim */
	{0x0078d800, 1},	/* emd */
	{0x00798800, 0},
	{0x0079b800, 1},	/* tim */
	{0x007a4000, 1},	/* emd */
	{0x007a5000, 0},
	{0x007a6000, 1},	/* tim */
	{0x007ae800, 1},	/* emd */
	{0x007bc800, 0},
	{0x007bd800, 1},	/* tim */
	{0x007c6000, 1},	/* emd */
	{0x007cc800, 1},	/* tim */
	{0x007e5000, 1},	/* emd */
	{0x007f2800, 1},	/* tim */
	{0x00803000, 1},	/* emd */
	{0x00816800, 1},	/* tim */
	{0x0082f000, 1},	/* emd */
	{0x0083b800, 1},	/* tim */
	{0x00854000, 1},	/* emd */
	{0x00867800, 1},	/* tim */
	{0x00880000, 1},	/* emd */
	{0x00892800, 1},	/* tim */
	{0x008a3000, 1},	/* emd */
	{0x008b5800, 1},	/* tim */
	{0x008ce000, 1},	/* emd */
	{0x008db000, 1},	/* tim */
	{0x008f3800, 1},	/* emd */
	{0x008fb800, 1},	/* tim */
	{0x00914000, 1},	/* emd */
	{0x00921000, 1},	/* tim */
	{0x00939800, 1},	/* emd */
	{0x00961000, 1},	/* tim */
	{0x00971800, 1},	/* emd */
	{0x00976000, 1},	/* tim */
	{0x00986800, 1},	/* emd */
	{0x00999000, 1},	/* tim */
	{0x009b1800, 1},	/* emd */
	{0x009c4000, 1},	/* tim */
	{0x009d4800, 1},	/* emd */
	{0x009e7800, 1},	/* tim */
	{0x009f8000, 1},	/* emd */
	{0x00a0a800, 1},	/* tim */
	{0x00a1b000, 1},	/* emd */
	{0x00a2e000, 1},	/* tim */
	{0x00a3e800, 1},	/* emd */
	{0x00a51000, 1},	/* tim */
	{0x00a69800, 1},	/* emd */
	{0x00a7c800, 1},	/* tim */
	{0x00a95000, 1},	/* emd */
	{0x00aa8800, 0}		/* file length */
};

static const re2ps1_ems_t re2ps1gamec2_ems[]={
	{0x00000000, 0},
	{0x0001a000, 1},	/* tim */
	{0x0002a800, 1},	/* emd */
	{0x0004e800, 1},	/* tim */
	{0x0005f000, 1},	/* emd */
	{0x00083800, 1},	/* tim */
	{0x000a4800, 1},	/* emd */
	{0x000c8800, 1},	/* tim */
	{0x000d9000, 1},	/* emd */
	{0x000fd000, 1},	/* tim */
	{0x0010d800, 1},	/* emd */
	{0x00131800, 1},	/* tim */
	{0x00142000, 1},	/* emd */
	{0x00166000, 1},	/* tim */
	{0x00176800, 1},	/* emd */
	{0x0019b800, 1},	/* tim */
	{0x001ac000, 1},	/* emd */
	{0x001ce800, 1},	/* tim */
	{0x001ef800, 1},	/* emd */
	{0x00212000, 1},	/* tim */
	{0x00233000, 1},	/* emd */
	{0x00255800, 0},
	{0x00260800, 1},	/* tim */
	{0x00271000, 1},	/* emd */
	{0x0028f000, 0},
	{0x00299000, 1},	/* tim */
	{0x002a1800, 1},	/* emd */
	{0x002a9800, 0},
	{0x002b8800, 1},	/* tim */
	{0x002c9000, 1},	/* emd */
	{0x002f4000, 0},
	{0x002fd000, 1},	/* tim */
	{0x0031e000, 1},	/* emd */
	{0x0034c800, 0},
	{0x00358800, 1},	/* tim */
	{0x00369000, 1},	/* emd */
	{0x00389000, 0},
	{0x00396000, 1},	/* tim */
	{0x003a6800, 1},	/* emd */
	{0x003b4000, 0},
	{0x003b7000, 1},	/* tim */
	{0x003bf800, 1},	/* emd */
	{0x003c1800, 0},
	{0x003c8800, 1},	/* tim */
	{0x003d1000, 1},	/* emd */
	{0x003db000, 0},
	{0x003e8000, 1},	/* tim */
	{0x00409000, 1},	/* emd */
	{0x00437000, 0},
	{0x0043b000, 1},	/* tim */
	{0x00443800, 1},	/* emd */
	{0x00447000, 0},
	{0x00451000, 1},	/* tim */
	{0x00461800, 1},	/* emd */
	{0x00483800, 0},
	{0x0048d800, 1},	/* tim */
	{0x004a6000, 1},	/* emd */
	{0x004c8800, 0},
	{0x004c9800, 1},	/* tim */
	{0x004d2000, 1},	/* emd */
	{0x004d6000, 0},
	{0x004d9000, 1},	/* tim */
	{0x004e1800, 1},	/* emd */
	{0x004e8000, 0},
	{0x004f3000, 1},	/* tim */
	{0x00503800, 1},	/* emd */
	{0x00538800, 0},
	{0x0053d800, 1},	/* tim */
	{0x00546000, 1},	/* emd */
	{0x00547800, 0},
	{0x00556800, 1},	/* tim */
	{0x0056f000, 1},	/* emd */
	{0x00594800, 0},
	{0x005a4800, 1},	/* tim */
	{0x005bd000, 1},	/* emd */
	{0x005ec800, 0},
	{0x005f9800, 1},	/* tim */
	{0x00612000, 1},	/* emd */
	{0x00644800, 0},
	{0x0065a800, 1},	/* tim */
	{0x00673000, 1},	/* emd */
	{0x006b6000, 0},
	{0x006c2000, 1},	/* tim */
	{0x006e2800, 1},	/* emd */
	{0x006fe000, 0},
	{0x0070a000, 1},	/* tim */
	{0x00712800, 1},	/* emd */
	{0x00721000, 0},
	{0x0072c000, 1},	/* tim */
	{0x0073c800, 1},	/* emd */
	{0x00771800, 0},
	{0x0077b800, 1},	/* tim */
	{0x0078c000, 1},	/* emd */
	{0x00797000, 0},
	{0x0079a000, 1},	/* tim */
	{0x007a2800, 1},	/* emd */
	{0x007a3800, 0}		/* file length */
};

static const char *re2ps1game_leon_movies[] = {
	"pl0/zmovie/opn1stl.str",
	"pl0/zmovie/opn2ndl.str",
	"pl0/zmovie/opn2ndrl.str",
	"pl0/zmovie/r108l.str",
	"pl0/zmovie/r204l.str",
	"pl0/zmovie/r409.str",
	"pl0/zmovie/r700l.str",
	"pl0/zmovie/r703l.str",
	"pl0/zmovie/r704l.str",
	"pl0/zmovie/titlel.str",
	"zmovie/r109.str",
	"zmovie/r10b.str",
	"zmovie/r200.str",
	"zmovie/r505.str",
	"zmovie/virgin.str",
	NULL
};

static const char *re2ps1game_claire_movies[] = {
	"pl1/zmovie/opn1stc.str",
	"pl1/zmovie/opn2ndc.str",
	"pl1/zmovie/opn2ndrc.str",
	"pl1/zmovie/r108c.str",
	"pl1/zmovie/r204c.str",
	"pl1/zmovie/r408.str",
	"pl1/zmovie/r700c.str",
	"pl1/zmovie/r703c.str",
	"pl1/zmovie/r704c.str",
	"pl1/zmovie/titlec.str",
	"zmovie/r109.str",
	"zmovie/r10b.str",
	"zmovie/r200.str",
	"zmovie/r505.str",
	"zmovie/virgin.str",
	NULL
};

/*--- Variables ---*/

static int game_player = 0;

/*--- Functions prototypes ---*/

static void re2ps1_shutdown(void);

static void re2ps1_loadbackground(void);

static void re2ps1_loadroom(void);
static int re2ps1_loadroom_rdt(const char *filename);

/*--- Functions ---*/

void re2ps1_init(state_t *game_state)
{
	game_state->load_background = re2ps1_loadbackground;
	game_state->load_room = re2ps1_loadroom;
	game_state->shutdown = re2ps1_shutdown;

	switch(game_state->version) {
		case GAME_RE2_PS1_DEMO:
			game_state->movies_list = (char **) re2ps1demo_movies;
			break;
		case GAME_RE2_PS1_GAME_LEON:
			game_state->movies_list = (char **) re2ps1game_leon_movies;
			break;
		case GAME_RE2_PS1_GAME_CLAIRE:
			game_state->movies_list = (char **) re2ps1game_claire_movies;
			game_player = 1;
			break;
	}
}

static void re2ps1_shutdown(void)
{
}

static void re2ps1_loadbackground(void)
{
	char *filepath;

	filepath = malloc(strlen(re2ps1_bg)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re2ps1_bg, game_state.stage, game_state.room);

	logMsg(1, "bss: Loading %s ... ", filepath);
	logMsg(1, "%s\n", background_bss_load(filepath, CHUNK_SIZE) ? "done" : "failed");

	free(filepath);
}

static void re2ps1_loadroom(void)
{
	char *filepath;

	filepath = malloc(strlen(re2ps1_room)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re2ps1_room, game_player, game_state.stage, game_state.room);

	logMsg(1, "rdt: Loading %s ... ", filepath);
	logMsg(1, "%s\n", re2ps1_loadroom_rdt(filepath) ? "done" : "failed");

	free(filepath);
}

static int re2ps1_loadroom_rdt(const char *filename)
{
	SDL_RWops *src;
	int retval = 0;
	PHYSFS_sint64 length;
	Uint8 *rdt_header;
	
	game_state.num_cameras = 16;

	game_state.room_file = FS_Load(filename, &length);
	if (!game_state.room_file) {
		return retval;
	}

	rdt_header = (Uint8 *) game_state.room_file;
	game_state.num_cameras = rdt_header[1];

	retval = 1;
	return retval;
}

typedef struct {
	unsigned short unk0;
	unsigned short const0; /* 0x683c, or 0x73b7 */
	/* const0>>7 used for engine */
	long camera_from_x;
	long camera_from_y;
	long camera_from_z;
	long camera_to_x;
	long camera_to_y;
	long camera_to_z;
	unsigned long offset;
} rdt_camera_pos_t;

void re2ps1_get_camera(long *camera_pos)
{
	Uint32 *cams_offset, offset;
	rdt_camera_pos_t *cam_array;
	
	cams_offset = (Uint32 *) ( &((Uint8 *)game_state.room_file)[8+7*4]);
	offset = SDL_SwapLE32(*cams_offset);
	cam_array = (rdt_camera_pos_t *) &((Uint8 *)game_state.room_file)[offset];

	camera_pos[0] = SDL_SwapLE32(cam_array[game_state.camera].camera_from_x);
	camera_pos[1] = SDL_SwapLE32(cam_array[game_state.camera].camera_from_y);
	camera_pos[2] = SDL_SwapLE32(cam_array[game_state.camera].camera_from_z);
	camera_pos[3] = SDL_SwapLE32(cam_array[game_state.camera].camera_to_x);
	camera_pos[4] = SDL_SwapLE32(cam_array[game_state.camera].camera_to_y);
	camera_pos[5] = SDL_SwapLE32(cam_array[game_state.camera].camera_to_z);
}
