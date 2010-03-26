/*
	RE3
	PS1
	Game

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
#include "re3_ps1_game.h"
#include "background_bss.h"
#include "parameters.h"
#include "log.h"
#include "room_rdt2.h"
#include "render.h"

/*--- Defines ---*/

#define CHUNK_SIZE 65536

/*--- Types ---*/

typedef struct {
	Uint32 length;
	Uint32 count;
} ard_header_t;

typedef struct {
	Uint32 length;
	Uint32 unknown;
} ard_object_t;

typedef struct {
	Uint32	start;	/* Start sector */
	Uint16	count;	/* Number of sectors */
	const char *filename;	/* File stored there */
} re3_cdraw_t;

/*--- Constant ---*/

static const char *re3ps1game_bg = "cd_data/stage%d/r%d%02x.bss";
static const char *re3ps1game_room = "cd_data/stage%d/r%d%02x.ard";
static const char *re3ps1game_font = "cd_data/etc/sele_ob%c.tim";

static const char *re3ps1game_movies[] = {
	"cd_data/zmovie/enda.str",
	"cd_data/zmovie/endb.str",
	"cd_data/zmovie/ins01.str",
	"cd_data/zmovie/ins02.str",
	"cd_data/zmovie/ins03.str",
	"cd_data/zmovie/ins04.str",
	"cd_data/zmovie/ins05.str",
	"cd_data/zmovie/ins06.str",
	"cd_data/zmovie/ins07.str",
	"cd_data/zmovie/ins08.str",
	"cd_data/zmovie/ins09.str",
	"cd_data/zmovie/opn.str",
	"cd_data/zmovie/roopne.str",
	NULL
};

/*
	(0x10),	(0x11),	(0x12),	0x13,	(0x14),	(0x15),	(0x16),	(0x17),
	0x18,	0x19,	0x1a,	0x1b,	(0x1c),	0x1d,	(0x1e),	0x1f,
	(0x20),	(0x21),	0x22,	(0x23),	0x24,	0x25,	0x26,	0x27,
	(0x28),	0x2c,	(0x2d),	0x2e,	(0x2f),	0x30,	0x32,	0x33,
	(0x34),	0x35,	0x36,	0x37,	0x38,	0x39,	0x3a,	0x3b,
	0x3e,	0x3f,	0x40,	0x50,	0x51,	(0x52),	(0x53),	(0x54),
	(0x55),	0x56,	0x57,	(0x58),	(0x59),	0x5a,	0x5b,	0x5c,
	0x5d,	0x5e,	0x5f,	0x60,	0x61,	0x62,	0x63,	0x64,
	0x65,	0x66,	0x67,	0x70,	0x71
*/

static const re3cd_raw_t re3_sles_02530[]={
	{126,76,"capcom.tim"},
	{202,82,"continue.tim"},
	{284,32,"core00f.tim"},
	{316,25,"died00f.tim"},
	{341,86,"eidos.tim"},
	{427,76,"epi_sel.tim"},
	{503,17,"epi_none.tim"},
	{520,17,"insta_i.tim"},
	{537,12,"insta_t0.tim"},
	{549,12,"insta_t1.tim"},
	{561,12,"insta_t2.tim"},
	{573,12,"insta_t3.tim"},
	{585,12,"insta_t4.tim"},
	{597,12,"insta_t5.tim"},
	{609,12,"insta_t6.tim"},
	{621,12,"insta_t7.tim"},
	{633,12,"insta_t8.tim"},
	{645,17,"dario_i.tim"},
	{662,12,"dario_t0.tim"},
	{674,12,"dario_t1.tim"},
	{686,12,"dario_t2.tim"},
	{698,12,"dario_t3.tim"},
	{710,12,"dario_t4.tim"},
	{722,17,"dmerc_i.tim"},
	{739,12,"dmerc_t0.tim"},
	{751,12,"dmerc_t1.tim"},
	{763,12,"dmerc_t2.tim"},
	{775,12,"dmerc_t3.tim"},
	{787,12,"dmerc_t4.tim"},
	{799,12,"dmerc_t5.tim"},
	{811,12,"dmerc_t6.tim"},
	{823,17,"fax1_i.tim"},
	{840,12,"fax1_t0.tim"},
	{852,12,"fax1_t1.tim"},
	{864,17,"marvin_i.tim"},
	{881,12,"marvin_t0.tim"},
	{893,12,"marvin_t1.tim"},
	{905,12,"marvin_t2.tim"},
	{917,12,"marvin_t3.tim"},
	{929,12,"marvin_t4.tim"},
	{941,17,"david_i.tim"},
	{958,12,"david_t0.tim"},
	{970,12,"david_t1.tim"},
	{982,12,"david_t2.tim"},
	{994,12,"david_t3.tim"},
	{1006,12,"david_t4.tim"},
	{1018,17,"town_i.tim"},
	{1035,12,"town_t0.tim"},
	{1047,12,"town_t1.tim"},
	{1059,12,"town_t2.tim"},
	{1071,12,"town_t3.tim"},
	{1083,12,"town_t4.tim"},
	{1095,17,"report_i.tim"},
	{1112,12,"report_t0.tim"},
	{1124,12,"report_t1.tim"},
	{1136,12,"report_t2.tim"},
	{1148,12,"report_t3.tim"},
	{1160,12,"report_t4.tim"},
	{1172,12,"report_t5.tim"},
	{1184,17,"inst_i.tim"},
	{1201,12,"inst_t0.tim"},
	{1213,12,"inst_t1.tim"},
	{1225,12,"inst_t2.tim"},
	{1237,12,"inst_t3.tim"},
	{1249,12,"inst_t4.tim"},
	{1261,17,"nmerc_i.tim"},
	{1278,12,"nmerc_t0.tim"},
	{1290,12,"nmerc_t1.tim"},
	{1302,12,"nmerc_t2.tim"},
	{1314,12,"nmerc_t3.tim"},
	{1326,12,"nmerc_t4.tim"},
	{1338,12,"nmerc_t5.tim"},
	{1350,12,"nmerc_t6.tim"},
	{1362,17,"postcard_i.tim"},
	{1379,12,"postcard_t0.tim"},
	{1391,12,"postcard_t1.tim"},
	{1403,12,"postcard_t2.tim"},
	{1415,17,"super_i.tim"},
	{1432,12,"super_t0.tim"},
	{1444,12,"super_t1.tim"},
	{1456,12,"super_t2.tim"},
	{1468,12,"super_t3.tim"},
	{1480,12,"super_t4.tim"},
	{1492,12,"super_t5.tim"},
	{1504,17,"order_i.tim"},
	{1521,12,"order_t0.tim"},
	{1533,12,"order_t1.tim"},
	{1545,12,"order_t2.tim"},
	{1557,17,"director_i.tim"},
	{1574,12,"director_t0.tim"},
	{1586,12,"director_t1.tim"},
	{1598,12,"director_t2.tim"},
	{1610,12,"director_t3.tim"},
	{1622,12,"director_t4.tim"},
	{1634,12,"director_t5.tim"},
	{1646,12,"director_t6.tim"},
	{1658,17,"manager1_i.tim"},
	{1675,12,"manager1_t0.tim"},
	{1687,12,"manager1_t1.tim"},
	{1699,12,"manager1_t2.tim"},
	{1711,12,"manager1_t3.tim"},
	{1723,12,"manager1_t4.tim"},
	{1735,12,"manager1_t5.tim"},
	{1747,12,"manager1_t6.tim"},
	{1759,12,"manager1_t7.tim"},
	{1771,17,"security_i.tim"},
	{1788,12,"security_t0.tim"},
	{1800,12,"security_t1.tim"},
	{1812,12,"security_t2.tim"},
	{1824,12,"security_t3.tim"},
	{1836,17,"meca_i.tim"},
	{1853,12,"meca_t0.tim"},
	{1865,12,"meca_t1.tim"},
	{1877,12,"meca_t2.tim"},
	{1889,12,"meca_t3.tim"},
	{1901,12,"meca_t4.tim"},
	{1913,17,"kendo_i.tim"},
	{1930,12,"kendo_t0.tim"},
	{1942,12,"kendo_t1.tim"},
	{1954,12,"kendo_t2.tim"},
	{1966,12,"kendo_t3.tim"},
	{1978,12,"kendo_t4.tim"},
	{1990,12,"kendo_t5.tim"},
	{2002,17,"manager2_i.tim"},
	{2019,12,"manager2_t0.tim"},
	{2031,12,"manager2_t1.tim"},
	{2043,12,"manager2_t2.tim"},
	{2055,12,"manager2_t3.tim"},
	{2067,12,"manager2_t4.tim"},
	{2079,12,"manager2_t5.tim"},
	{2091,17,"medic_i.tim"},
	{2108,12,"medic_t0.tim"},
	{2120,12,"medic_t1.tim"},
	{2132,12,"medic_t2.tim"},
	{2144,12,"medic_t3.tim"},
	{2156,17,"fax2_i.tim"},
	{2173,12,"fax2_t0.tim"},
	{2185,12,"fax2_t1.tim"},
	{2197,17,"incinerator_i.tim"},
	{2214,12,"incinerator_t0.tim"},
	{2226,12,"incinerator_t1.tim"},
	{2238,12,"incinerator_t2.tim"},
	{2250,12,"incinerator_t3.tim"},
	{2262,17,"photoa_i.tim"},
	{2279,12,"photoa_t0.tim"},
	{2291,12,"photoa_t1.tim"},
	{2303,12,"photoa_t2.tim"},
	{2315,12,"photoa_t3.tim"},
	{2327,17,"photob_i.tim"},
	{2344,12,"photob_t0.tim"},
	{2356,12,"photob_t1.tim"},
	{2368,12,"photob_t2.tim"},
	{2380,12,"photob_t3.tim"},
	{2392,17,"photoc_i.tim"},
	{2409,12,"photoc_t0.tim"},
	{2421,12,"photoc_t1.tim"},
	{2433,12,"photoc_t2.tim"},
	{2445,17,"photoe_i.tim"},
	{2462,12,"photoe_t0.tim"},
	{2474,12,"photoe_t1.tim"},
	{2486,12,"photoe_t2.tim"},
	{2498,17,"photod_i.tim"},
	{2515,12,"photod_t0.tim"},
	{2527,12,"photod_t1.tim"},
	{2539,12,"photod_t2.tim"},
	{2551,12,"photod_t3.tim"},
	{2563,17,"belfry_i.tim"},
	{2580,12,"belfry_t0.tim"},
	{2592,12,"belfry_t1.tim"},
	{2604,12,"belfry_t2.tim"},
	{2616,12,"belfry_t3.tim"},
	{2628,17,"instb_i.tim"},
	{2645,12,"instb_t0.tim"},
	{2657,12,"instb_t1.tim"},
	{2669,12,"instb_t2.tim"},
	{2681,12,"instb_t3.tim"},
	{2693,12,"instb_t4.tim"},
	{2705,12,"instb_t5.tim"},
	{2717,12,"instb_t6.tim"},
	{2729,12,"instb_t7.tim"},
	{2741,17,"secret_i.tim"},
	{2758,12,"secret_t0.tim"},
	{2770,12,"secret_t1.tim"},
	{2782,12,"secret_t2.tim"},
	{2794,17,"djill_i.tim"},
	{2811,12,"djill_t0.tim"},
	{2823,12,"djill_t1.tim"},
	{2835,12,"djill_t2.tim"},
	{2847,12,"djill_t3.tim"},
	{2859,12,"djill_t4.tim"},
	{2871,12,"djill_t5.tim"},
	{2883,12,"djill_t6.tim"},
	{2895,12,"djill_t7.tim"},
	{2907,17,"fileif.tim"},
	{2924,17,"filei.tim"},
	{2941,65,"fontst0f.tim"},
	{3006,65,"fontst1f.tim"},
	{3071,65,"fontst2f.tim"},
	{3136,65,"fontst3f.tim"},
	{3201,65,"fontst4f.tim"},
	{3266,65,"fontst5f.tim"},
	{3331,65,"fontst6f.tim"},
	{3396,65,"fontst7f.tim"},
	{3461,44,"nemesis_icon.tim"},
	{3505,5,"colt1.tim"},
	{3510,5,"knife.tim"},
	{3515,5,"colt2.tim"},
	{3520,5,"colt3.tim"},
	{3525,5,"shotgun1.tim"},
	{3530,5,"magnum.tim"},
	{3535,5,"grenade_launcher.tim"},
	/*{3540,5,"grenade_launcher.tim"},*/ /*copy*/
	/*{3545,5,"grenade_launcher.tim"},*/ /*copy*/
	/*{3550,5,"grenade_launcher.tim"},*/ /*copy*/
	{3555,5,"rocket_launcher.tim"},
	{3560,5,"gattling.tim"},
	{3565,5,"supercolt.tim"},
	{3570,5,"lasercolt.tim"},
	{3575,5,"m16.tim"},
	/*{3580,5,"m16.tim"},*/ /*copy*/
	{3585,5,"shotgun2.tim"},
	/*{3590,5,"colt2.tim"},*/ /*copy*/
	/*{3595,5,"colt3.tim"},*/ /*copy*/
	/*{3600,5,"shotgun.tim"},*/ /*copy*/
	/*{3605,5,"supercolt.tim"},*/ /*copy*/
	{3610,5,"ammo_colt.tim"},
	{3615,5,"ammo_magnum.tim"},
	{3620,5,"ammo_shotgun.tim"},
	{3625,5,"ammo_grenade.tim"},
	{3630,5,"ammo_grenade_fire.tim"},
	{3635,5,"ammo_grenade_acid.tim"},
	{3640,5,"ammo_grenade_blue.tim"},
	{3645,5,"ammo_elec.tim"},
	{3650,5,"ammo_m16.tim"},
	{3655,5,"ammo_supercolt.tim"},
	{3660,5,"ammo_shotgun_blue.tim"},
	{3665,5,"spray.tim"},
	{3670,5,"herb_g.tim"},
	{3675,5,"herb_b.tim"},
	{3680,5,"herb_r.tim"},
	{3685,5,"herb_gg.tim"},
	{3690,5,"herb_gb.tim"},
	{3695,5,"herb_gr.tim"},
	{3700,5,"herb_ggg.tim"},
	{3705,5,"herb_ggb.tim"},
	{3710,5,"herb_gbr.tim"},
	{3715,5,"spray_box.tim"},
	{3720,5,"lever.tim"},
	{3725,5,"medal_r.tim"},
	{3730,5,"medal_b.tim"},
	{3735,5,"medal_y.tim"},
	{3740,5,"police_card1.tim"},
	{3745,5,"oil.tim"},
	{3750,5,"battery.tim"},
	{3755,5,"crowbar.tim"},
	{3760,5,"elec_cord.tim"},
	{3765,5,"xxx.tim"},
	{3770,5,"tuyau_arrose.tim"},
	{3775,5,"bottle.tim"},
	{3780,5,"etui.tim"},
	{3785,5,"police_card2.tim"},
	{3790,5,"gas_empty.tim"},
	{3795,5,"gas_filled.tim"},
	{3800,5,"chain.tim"},
	{3805,5,"spanner.tim"},
	{3810,5,"pipe.tim"},
	{3815,5,"plug.tim"},
	/*{3820,5,"tuyau_arrose.tim"},*/ /*copy*/
	{3825,5,"dictaphone.tim"},
	{3830,5,"lighter_gas.tim"},
	{3835,5,"lighter_closed.tim"},
	{3840,5,"lighter_opened.tim"},
	{3845,5,"gem_g.tim"},
	{3850,5,"gem_b.tim"},
	{3855,5,"ovalgem_y.tim"},
	{3860,5,"ovalgem_o.tim"},
	{3865,5,"ovalgem_w.tim"},
	{3870,5,"remote_opened.tim"},
	{3875,5,"remote_closed.tim"},
	{3880,5,"remote_battery.tim"},
	{3885,5,"gear_y.tim"},
	{3890,5,"gear_w.tim"},
	{3895,5,"gear_yw.tim"},
	{3900,5,"book.tim"},
	{3905,5,"sundial.tim"},
	{3910,5,"serum_r.tim"},
	{3915,5,"serum_b.tim"},
	/*{3920,5,"colt.tim"},*/ /*copy*/
	/*{3925,5,"colt.tim"},*/ /*copy*/
	{3930,5,"serum_p1.tim"},
	/*{3935,5,"colt.tim"},*/ /*copy*/
	/*{3940,5,"colt.tim"},*/ /*copy*/
	{3945,5,"storage.tim"},
	{3950,5,"colt_pieces1.tim"},
	{3955,5,"colt_pieces2.tim"},
	{3960,5,"shotgun_pieces1.tim"},
	{3965,5,"shotgun_pieces2.tim"},
	/*{3970,5,"colt.tim"},*/ /*copy*/
	{3975,5,"key_gem.tim"},
	{3980,5,"lever2.tim"},
	{3985,5,"umbrella_card.tim"},
	{3990,5,"ammo_r.tim"},
	{3995,5,"ammo_y.tim"},
	{4000,5,"ammo_b.tim"},
	{4005,5,"ammo_rr.tim"},
	{4010,5,"ammo_yy.tim"},
	{4015,5,"ammo_rb.tim"},
	{4020,5,"ammo_yb.tim"},
	{4025,5,"ammo_bb.tim"},
	{4030,5,"ammo_rrr.tim"},
	{4035,5,"ammo_rry.tim"},
	{4040,5,"ammo_ryy.tim"},
	{4045,5,"ammo_yyy.tim"},
	{4050,5,"ammo_bbb.tim"},
	{4055,5,"ammo_inf.tim"},
	{4060,5,"water.tim"},
	{4065,5,"modisc.tim"},
	{4070,5,"key_spade.tim"},
	{4075,5,"paperclip.tim"},
	{4080,5,"key_xxx.tim"},
	{4085,5,"key_paper_y.tim"},
	{4090,5,"key_xxx.tim"},
	{4095,5,"keychain.tim"},
	{4100,5,"key_hole1.tim"},
	{4105,5,"key_hole2.tim"},
	{4110,5,"key_hole.tim"},
	/*{4115,5,"colt.tim"},*/ /*copy */
	{4120,5,"key_wood.tim"},
	{4125,5,"key_xxx.tim"},
	{4130,5,"key_small.tim"},
	{4135,5,"key_card1.tim"},
	{4140,5,"key_card2.tim"},
	{4145,5,"key_gem.tim"},
	{4150,5,"ink_ribbon.tim"},
	{4155,5,"ammo_build.tim"},
	/*{4160,5,"insta_i.tim"},*/ /*copy*/
	/*{4165,5,"instb_i.tim"},*/ /*copy*/
	{4170,5,"syrup.tim"},
	{4309,76,"jillbgf.tim"},
	{4385,231,"jill_obf.tim"},
	{4728,76,"epif0.tim"},
	{4804,76,"epif1.tim"},
	{4880,76,"epif2.tim"},
	{4956,76,"epif3.tim"},
	{5032,76,"epif4.tim"},
	{5108,76,"epif5.tim"},
	{5184,76,"epif6.tim"},
	{5260,76,"epif7.tim"},
	{5336,76,"endf.tim"},
	{5412,76,"epij0.tim"},
	{5488,223,"start0.tim"},
	{5712,151,"start1.tim"},
	/*{5863,223,"start0.tim"},*/ /*copy*/
	/*{6087,151,"start1.tim"},*/ /*copy*/
	{6238,33,"config0.tim"},
	{6271,76,"config1.tim"},
	/*{6347,76,"config1.tim"},*/ /*copy*/
	{6423,82,"config2.tim"},
	{6505,3,"radar.tim"},
	{6508,76,"res0_bgf.tim"},
	{6584,49,"res0_obf.tim"},
	{6633,76,"res3_bgf.tim"},
	{6709,76,"res4_bgf.tim"},
	{6785,76,"res5_bgf.tim"},
	{6861,76,"sele_bgf.tim"},
	{6937,49,"sele_obf.tim"},
	{6986,199,"staff0.tim"},
	{7185,36,"stmain0f.tim"},
	{7221,35,"stmain1f.tim"},
	{7256,35,"stmain2f.tim"},
	{7291,35,"stmain3f.tim"},
	{7326,5,"xxx.tim"},
	{7331,223,"stmojif.tim"},
	{7607,65,""},
	{7672,223,"texf.tim"},
	{7914,223,"warnf.tim"},
	{16181,49,"em10.tim"},	/* zombi (3 tx pages) */
	{16230,71,""},
	{16301,33,"em54.tim"},	/* reporter */
	{16334,36,""},
	{16370,33,"em59.tim"},	/* zombi reporter */
	{16403,223,""},
	/*{17567,49,"em10.tim"},*/ /*copy*/
	/*{17616,123,""},*/
	{17739,49,"em1c.tim"},	/* girl */
	{17788,223,""},
	/*{18405,49,""},
	{18454,111,""},*/
	{18565,66,"em1e.tim"},	/* zombi (4 tx pages) */
	{18631,225,""},
	/*{19583,66,"em1e.tim"},*/	/* copy*/
	/*{19649,65,""},*/
	{19714,33,"em53.tim"},	/* brad */
	{19747,223,""},
	/*{20673,98,""},
	{20771,68,""},*/
	/*{20839,33,"em53.tim"},*/	/*copy*/
	/*{20872,229,""},*/
	/*{21846,66,"em1e.tim"},*/	/*copy*/
	/*{21912,225,""},*/
	{23001,33,"em11.tim"},	/* zombi girl */
	{23034,121,""},
	{23155,33,"em14.tim"},	/* zombi (2 tx pages) */
	{23188,121,""},
	{23309,33,"em15.tim"},	/* black zombi */
	{23342,119,""},
	{23461,49,"em34.tim"},	/* nemesis 1 */
	{23510,71,""},
	/*{23581,33,"em53.tim"},*/	/*copy*/
	/*{23614,229,""},
	{24396,82,""},
	{24478,223,""},*/
	{25141,33,"em20.tim"},	/* dog */
	{25174,223,""},
	/*{25943,49,"em34.tim"},*/	/*copy*/
	/*{25992,223,""},*/
	{26770,49,"em23.tim"},	/* tick */
	{26819,149,""},
	{26968,33,"em28.tim"},
	{27001,149,""},
	/*{27150,49,"em34.tim"},*/	/*copy*/
	/*{27199,231,""},
	{28058,66,"em1e.tim"},*/	/*copy*/
	/*{28124,71,""},*/
	{28195,17,"em2d.tim"},
	{28212,234,""},
	/*{28981,33,"em14.tim"},*/	/*copy*/
	/*{29014,119,""},*/
	/*{29133,49,"em34.tim"},*/	/*copy*/
	/*{29182,235,""},*/
	/*{29643,49,"em23.tim"},*/	/*copy*/
	/*{29692,147,""},*/
	/*{29839,49,"em34.tim"},*/	/*copy*/
	/*{29888,231,""},*/
	/*{30551,49,"em34.tim"},*/	/*copy*/
	/*{30600,71,""},*/
	{30671,33,"em58.tim"},	/* zombi brad */
	{30704,228,""},
	/*{32155,33,"em14.tim"},*/	/*copy*/
	/*{32188,121,""},*/
	{32309,49,"em2f.tim"},
	{32358,120,""},
	/*{32478,49,"em34.tim"},*/	/*copy*/
	/*{32527,231,""},*/
	/*{33292,49,"em34.tim"},*/	/*copy*/
	/*{33341,231,""},*/
	/*{34032,49,"em10.tim"},*/	/*copy*/
	/*{34081,123,""},*/
	/*{34204,33,"em11.tim"},*/	/*copy*/
	/*{34237,119,""},*/
	/*{34356,49,"em34.tim"},*/	/*copy*/
	/*{34405,231,""},*/
	/*{35101,82,"em20.tim"},*/	/*copy*/
	/*{35183,87,""},*/
	{35270,33,""},
	{35303,103,""},
	/*{35406,49,"em34.tim"},*/	/*copy*/
	/*{35455,231,""},*/
	{37498,33,"em12.tim"},	/* zombi */
	{37531,121,""},
	{37652,49,"em17.tim"},	/* black zombi */
	{37701,123,""},
	/*{37824,49,"em34.tim"},*/	/*copy*/
	/*{37873,231,""},*/
	/*{38638,49,"em34.tim"},*/	/*copy*/
	/*{38687,231,""},*/
	{40144,98,""},
	{40242,68,""},
	{40310,33,"em52.tim"},	/* michail? nicholai? */
	{40343,37,""},
	{40380,33,"em55.tim"},
	{40413,36,""},
	{40449,33,"em5a.tim"},	/* jill costume 0 */
	{40482,36,""},
	{40518,33,"em5c.tim"},	/* (or em5d?) carlos */
	{40551,223,""},
	{41605,98,""},
	{41703,228,""},
	/*{42219,49,"em10.tim"},*/	/*copy*/
	/*{42268,123,""},*/
	/*{42391,49,"em1c.tim"},*/	/*copy*/
	/*{42440,223,""},*/
	/*{43042,49,"em1c.tim"},*/	/*copy*/
	/*{43091,111,""},*/
	/*{43202,66,"em1e.tim"},*/	/*copy*/
	/*{43268,225,""},*/
	{44229,66,"em16.tim"},	/* zombie (4 tx pages) */
	{44295,225,""},
	/*{45374,66,"em16.tim"},*/	/*copy*/
	/*{45440,87,""},*/
	{45527,17,"em21.tim"},	/* crow */
	{45544,62,""},
	/*{45606,49,"em34.tim"},*/	/*copy*/
	/*{45655,223,""},*/
	/*{46785,66,"em16.tim"},*/	/*copy*/
	/*{46851,89,""},*/
	/*{46940,33,"em20.tim"},*/	/*copy*/
	/*{46973,103,""},*/
	/*{47076,49,"em34.tim"},*/	/*copy*/
	/*{47125,223,""},*/
	/*{48176,49,"em10.tim"},*/	/*copy*/
	/*{48225,121,""},*/
	/*{48346,49,"em34.tim"},*/	/*copy*/
	/*{48395,231,""},*/
	/*{49084,33,"em20.tim"},*/	/*copy*/
	/*{49117,103,""},*/
	/*{49220,49,"em34.tim"},*/	/*copy*/
	/*{49269,231,""},*/
	/*{50539,33,"em14.tim"},*/	/*copy*/	
	/*{50572,121,""},*/
	{50693,82,""},
	{50775,87,""},
	/*{50862,33,"em20.tim"},*/	/*copy*/
	/*{50895,103,""},*/
	/*{50998,49,"em34.tim"},*/	/*copy*/
	/*{51047,231,""},*/
	/*{51633,33,"em58.tim"},*/	/*copy*/
	/*{51666,223,""},*/
	/*{56042,33,"em20.tim"},*/	/*copy*/
	/*{56075,75,""},*/
	/*{56150,17,"em21.tim"},*/	/*copy*/
	/*{56167,14,""},*/
	{56181,33,""},
	{56214,233,""},
	{57073,33,""},
	{57106,71,""},
	{57177,17,""},
	{57194,223,""},
	{58700,82,""},
	{58782,122,""},
	{58904,66,""},
	{58970,89,""},
	{59059,33,""},
	{59092,105,""},
	{59197,49,""},
	{59246,105,""},
	{59351,17,""},
	{59368,58,""},
	{59426,33,""},
	{59459,149,""},
	{59608,49,""},
	{59657,231,""},
	{60381,49,""},
	{60430,149,""},
	{60579,33,""},
	{60612,101,""},
	{60713,33,""},
	{60746,59,""},
	{60805,49,""},
	{60854,231,""},
	{61794,82,""},
	{61876,122,""},
	{61998,66,""},
	{62064,115,""},
	{62179,49,""},
	{62228,223,""},
	{63193,49,""},
	{63242,121,""},
	{63363,49,""},
	{63412,71,""},
	{63483,33,""},
	{63516,40,""},
	{63556,33,""},
	{63589,232,""},
	{64564,66,""},
	{64630,117,""},
	{64747,98,""},
	{64845,118,""},
	{64963,49,""},
	{65012,71,""},
	{65083,33,""},
	{65116,232,""},
	{66113,33,""},
	{66146,75,""},
	{66221,17,""},
	{66238,62,""},
	{66300,49,""},
	{66349,71,""},
	{66420,33,""},
	{66453,223,""},
	{67825,82,""},
	{67907,118,""},
	{68025,49,""},
	{68074,231,""},
	{69047,66,""},
	{69113,225,""},
	{69738,66,""},
	{69804,223,""},
	{71202,82,""},
	{71284,87,""},
	{71371,33,""},
	{71404,53,""},
	{71457,33,""},
	{71490,225,""},
	{72545,33,""},
	{72578,40,""},
	{72618,33,""},
	{72651,33,""},
	{72684,33,""},
	{72717,37,""},
	{72754,33,""},
	{72787,37,""},
	{72824,33,""},
	{72857,37,""},
	{72894,33,""},
	{72927,37,""},
	{72964,33,""},
	{72997,37,""},
	{73034,33,""},
	{73067,37,""},
	{73104,33,""},
	{73137,229,""},
	{74626,66,""},
	{74692,65,""},
	{74757,33,""},
	{74790,37,""},
	{74827,33,""},
	{74860,232,""},
	{75863,66,""},
	{75929,65,""},
	{75994,33,""},
	{76027,223,""},
	{76957,33,""},
	{76990,121,""},
	{77111,33,""},
	{77144,119,""},
	{77263,49,""},
	{77312,71,""},
	{77383,33,""},
	{77416,223,""},
	{78396,223,""},
	{78686,49,""},
	{78735,71,""},
	{78806,33,""},
	{78839,232,""},
	{79816,49,""},
	{79865,121,""},
	{79986,49,""},
	{80035,71,""},
	{80106,33,""},
	{80139,232,""},
	{81585,49,""},
	{81634,123,""},
	{81757,49,""},
	{81806,24,""},
	{81830,33,""},
	{81863,232,""},
	{82348,49,""},
	{82397,223,""},
	{83035,49,""},
	{83084,123,""},
	{83207,49,""},
	{83256,223,""},
	{84411,49,""},
	{84460,71,""},
	{84531,33,""},
	{84564,40,""},
	{84604,33,""},
	{84637,225,""},
	{86473,66,""},
	{86539,65,""},
	{86604,33,""},
	{86637,223,""},
	{87352,49,""},
	{87401,231,""},
	{88091,49,""},
	{88140,71,""},
	{88211,33,""},
	{88244,232,""},
	{89108,49,""},
	{89157,105,""},
	{89262,17,""},
	{89279,58,""},
	{89337,33,""},
	{89370,149,""},
	{89519,49,""},
	{89568,223,""},
	{96738,33,""},
	{96771,232,""},
	{97585,66,""},
	{97651,115,""},
	{97766,49,""},
	{97815,231,""},
	{98786,49,""},
	{98835,123,""},
	{98958,49,""},
	{99007,105,""},
	{99112,17,""},
	{99129,58,""},
	{99187,33,""},
	{99220,149,""},
	{99369,49,""},
	{99418,71,""},
	{99489,33,""},
	{99522,232,""},
	{100327,33,""},
	{100360,75,""},
	{100435,17,""},
	{100452,62,""},
	{100514,49,""},
	{100563,223,""},
	{101434,49,""},
	{101483,231,""},
	{102228,49,""},
	{102277,95,""},
	{102372,33,""},
	{102405,223,""},
	{103818,33,""},
	{103851,121,""},
	{103972,33,""},
	{104005,75,""},
	{104080,17,""},
	{104097,51,""},
	{104148,49,""},
	{104197,123,""},
	{104320,49,""},
	{104369,95,""},
	{104464,17,""},
	{104481,17,""},
	{104498,33,""},
	{104531,232,""},
	{105214,33,""},
	{105247,28,""},
	{105275,17,""},
	{105292,53,""},
	{105345,49,""},
	{105394,95,""},
	{105489,17,""},
	{105506,244,""},
	{106004,49,""},
	{106053,95,""},
	{106148,17,""},
	{106165,223,""},
	{106931,33,""},
	{106964,28,""},
	{106992,17,""},
	{107009,51,""},
	{107060,49,""},
	{107109,223,""},
	{107746,49,""},
	{107795,73,""},
	{107868,17,""},
	{107885,229,""},
	{109852,82,""},
	{109934,83,""},
	{110017,17,""},
	{110034,7,""},
	{110041,33,""},
	{110074,223,""},
	{111302,49,""},
	{111351,95,""},
	{111446,17,""},
	{111463,224,""},
	{112563,49,""},
	{112612,95,""},
	{112707,17,""},
	{112724,223,""},
	{113521,33,""},
	{113554,37,""},
	{113591,33,""},
	{113624,37,""},
	{113661,33,""},
	{113694,37,""},
	{113731,33,""},
	{113764,37,""},
	{113801,33,""},
	{113834,37,""},
	{113871,33,""},
	{113904,229,""},
	{114511,49,""},
	{114560,95,""},
	{114655,33,""},
	{114688,223,""},
	{115444,49,""},
	{115493,95,""},
	{115588,17,""},
	{115605,17,""},
	{115622,33,""},
	{115655,37,""},
	{115692,33,""},
	{115725,37,""},
	{115762,33,""},
	{115795,37,""},
	{115832,33,""},
	{115865,37,""},
	{115902,33,""},
	{115935,37,""},
	{115972,33,""},
	{116005,225,""},
	{116593,33,""},
	{116626,121,""},
	{116747,33,""},
	{116780,75,""},
	{116855,17,""},
	{116872,53,""},
	{116925,49,""},
	{116974,95,""},
	{117069,17,""},
	{117086,69,""},
	{117155,49,""},
	{117204,24,""},
	{117228,33,""},
	{117261,37,""},
	{117298,33,""},
	{117331,37,""},
	{117368,33,""},
	{117401,37,""},
	{117438,33,""},
	{117471,37,""},
	{117508,33,""},
	{117541,37,""},
	{117578,33,""},
	{117611,229,""},
	{118299,49,""},
	{118348,125,""},
	{118473,33,""},
	{118506,74,""},
	{118580,33,""},
	{118613,151,""},
	{118764,49,""},
	{118813,95,""},
	{118908,17,""},
	{118925,69,""},
	{118994,49,""},
	{119043,24,""},
	{119067,33,""},
	{119100,37,""},
	{119137,33,""},
	{119170,37,""},
	{119207,33,""},
	{119240,37,""},
	{119277,33,""},
	{119310,37,""},
	{119347,33,""},
	{119380,37,""},
	{119417,33,""},
	{119450,229,""},
	{120023,49,""},
	{120072,93,""},
	{120165,17,""},
	{120182,75,""},
	{120257,49,""},
	{120306,223,""},
	{121131,49,""},
	{121180,95,""},
	{121275,17,""},
	{121292,17,""},
	{121309,33,""},
	{121342,37,""},
	{121379,33,""},
	{121412,37,""},
	{121449,33,""},
	{121482,37,""},
	{121519,33,""},
	{121552,37,""},
	{121589,33,""},
	{121622,37,""},
	{121659,33,""},
	{121692,229,""},
	{122870,66,""},
	{122936,116,""},
	{123052,49,""},
	{123101,95,""},
	{123196,17,""},
	{123213,17,""},
	{123230,33,""},
	{123263,37,""},
	{123300,33,""},
	{123333,37,""},
	{123370,33,""},
	{123403,37,""},
	{123440,33,""},
	{123473,37,""},
	{123510,33,""},
	{123543,37,""},
	{123580,33,""},
	{123613,223,""},
	{128637,98,""},
	{128735,228,""},
	{129346,33,""},
	{129379,223,""},
	{129814,49,""},
	{129863,109,""},
	{129972,33,""},
	{130005,231,""},
	{130978,98,""},
	{131076,106,""},
	{131182,33,""},
	{131215,231,""},
	{131960,98,""},
	{132058,106,""},
	{132164,33,""},
	{132197,231,""},
	{133188,98,""},
	{133286,86,""},
	{133372,17,""},
	{133389,223,""},
	{133759,33,""},
	{133792,39,""},
	{133831,33,""},
	{133864,37,""},
	{133901,33,""},
	{133934,229,""},
	{134741,98,""},
	{134839,106,""},
	{134945,33,""},
	{134978,237,""},
	{135539,33,""},
	{135572,39,""},
	{135611,33,""},
	{135644,229,""},
	{136504,33,""},
	{136537,223,""},
	{137679,98,""},
	{137777,106,""},
	{137883,33,""},
	{137916,57,""},
	{137973,17,""},
	{137990,223,""},
	{139498,33,""},
	{139531,107,""},
	{139638,33,""},
	{139671,223,""},
	{140287,33,""},
	{140320,91,""},
	{140411,33,""},
	{140444,223,""},
	{141056,33,""},
	{141089,40,""},
	{141129,17,""},
	{141146,223,""},
	{141627,98,""},
	{141725,228,""},
	{142320,33,""},
	{142353,229,""},
	{143159,49,""},
	{143208,231,""},
	{143801,66,""},
	{143867,223,""},
	{144772,98,""},
	{144870,106,""},
	{144976,33,""},
	{145009,223,""},
	{149490,66,""},
	{149556,114,""},
	{149670,49,""},
	{149719,105,""},
	{149824,17,""},
	{149841,58,""},
	{149899,33,""},
	{149932,99,""},
	{150031,33,""},
	{150064,229,""},
	{150904,33,""},
	{150937,223,""},
	{152168,66,""},
	{152234,114,""},
	{152348,98,""},
	{152446,228,""},
	{153253,49,""},
	{153302,109,""},
	{153411,33,""},
	{153444,57,""},
	{153501,17,""},
	{153518,23,""},
	{153541,33,""},
	{153574,232,""},
	{155652,66,""},
	{155718,100,""},
	{155818,33,""},
	{155851,91,""},
	{155942,49,""},
	{155991,149,""},
	{156140,33,""},
	{156173,227,""},
	{156771,33,""},
	{156804,37,""},
	{156841,33,""},
	{156874,228,""},
	{157642,49,""},
	{157691,95,""},
	{157786,17,""},
	{157803,223,""},
	{158880,33,""},
	{158913,9,""},
	{158922,33,""},
	{158955,223,""},
	{160063,66,""},
	{160129,84,""},
	{160213,17,""},
	{160230,223,""},
	{160684,17,""},
	{160701,223,""},
	{161247,66,""},
	{161313,135,""},
	{161448,17,""},
	{161465,229,""},
	{162713,33,""},
	{162746,5,""},
	{162751,33,""},
	{162784,236,""},
	{163692,49,""},
	{163741,95,""},
	{163836,17,""},
	{163853,223,""},
	{170554,33,""},
	{170587,35,""},
	{170622,33,""},
	{170655,223,""},
	{170934,171,""},
	{171105,66,""},
	{171171,224,""},
	{172404,82,""},
	{172486,68,""},
	{172554,33,""},
	{172587,40,""},
	{172627,33,""},
	{172660,225,""},
	{173216,17,""},
	{173233,58,""},
	{173291,33,""},
	{173324,227,""},
	{173776,17,""},
	{173793,223,""},
	{174247,98,""},
	{174345,228,""},
	{175383,33,""},
	{175416,51,""},
	{175467,33,""},
	{175500,40,""},
	{175540,33,""},
	{175573,229,""},
	{176036,17,""},
	{176053,223,""},
	{176462,33,""},
	{176495,223,""},
	{177321,98,""},
	{177419,228,""},
	{178280,49,""},
	{178329,223,""},
	{179190,33,""},
	{179223,223,""},
	{180058,33,""},
	{180091,223,""},
	{180711,82,""},
	{180793,230,""},
	{181832,17,""},
	{181849,236,""},
	{184551,49,""},
	{184600,95,""},
	{184695,17,""},
	{184712,223,""},
	{185407,66,""},
	{185473,224,""},
	{186651,33,""},
	{186684,223,""},
	{187164,49,""},
	{187213,231,""},
	{187915,33,""},
	{187948,28,""},
	{187976,17,""},
	{187993,225,""},
	{188698,98,""},
	{188796,228,""},
	{189611,33,""},
	{189644,223,""},
	{190449,82,""},
	{190531,223,""},
	{191743,82,""},
	{191825,223,""},
	{192638,17,""},
	{192655,223,""},
	{193090,82,""},
	{193172,228,""},
	{194359,98,""},
	{194457,228,""},
	{196531,49,""},
	{196580,72,""},
	{196652,33,""},
	{196685,228,""},
	{197477,33,""},
	{197510,223,""},
	{198132,49,""},
	{198181,75,""},
	{198256,33,""},
	{198289,228,""},
	{199392,33,""},
	{199425,231,""},
	{200097,33,""},
	{200130,231,""},
	{201501,82,""},
	{201583,63,""},
	{201646,33,""},
	{201679,229,""},
	{202193,49,""},
	{202242,105,""},
	{202347,17,""},
	{202364,223,""},
	{202753,66,""},
	{202819,62,""},
	{202881,49,""},
	{202930,223,""},
	{203889,66,""},
	{203955,114,""},
	{204069,49,""},
	{204118,231,""},
	{204670,49,""},
	{204719,105,""},
	{204824,17,""},
	{204841,223,""},
	{205288,49,""},
	{205337,223,""},

	{0,0,""}
};

/*
static const re3cd_raw_t re3_sles_02532[]={
};

static const re3cd_raw_t re3_slus_00923[]={
};
*/

/*
	offset Hex	offset Dec	name		offset Hex
	(bytes)		(2352 sectors)			(storage of sector number)

slus_00923
offset table at 131b2db0 (sector 136287) ?

	02e95f30	20769		em10.tim	0db64540
	02eb2160	20818		em10.emd	13a01850
	02edadb0	20889		em54.tim	2566ea50
	02eedce0	20922		em54.emd
	02f027a0	20958		em59.tim	01fba5e0	05d1d430
	02f156d0	20991		em59.emd	131bb790
	031b1d10	22155		em15.tim	0469927c	07522e86
	031cdf40	22204		em15.emd
	03214950	22327		em1c.tim	25ac4a16
	03230b80	22376		em1c.emd	0426dab0
	03393030	22993		em16.tim	16cb9650
	033af260	23042		em16.emd	2877b5cf
	033eee30	23153		em1e.tim	09fe3019
	03414c90	23219		em1e.emd	2674c4d0
	03682aa0	24302		em53.tim	0196c0fa
	036959d0	24335		em53.emd	02e3ff20
	038a9570	25261		em1f.tim	0be6aeaa
	038e19d0	25359		em1f.emd	0bbcfd60
	03de21f0	27589		em11.tim	020b3460
	03df5120	27622		em11.emd	047b4180
	03e3a8d0	27743		em14.tim
	03e4d800	27776		em14.emd	04cd05a0
	03eea430	28049		em34.tim
	03f06660	28098		em34.emd	07c71cf0
	04103280	28984		em1d.tim
	041323e0	29066		em1d.emd	16a03840
	042aef30	29729		em20.tim
	042c1e60	29762		em20.emd	07021f80
	046565a0	31358		em23.tim
	046727d0	31407		em23.emd
	046c80c0	31556		em28.tim
	046daff0	31589		em28.emd
	049889d0	32783		em2d.tim
	04992600	32800		em2d.emd	03464180
	04e40320	34886		em58.tim
	04e53250	34919		em58.emd	23d55bb0
	051ecc40	36524		em2f.tim
	05208e70	36573		em2f.emd	07104130
	0524dcf0			.tim
	05421390			.tim
	055ca250			.tim
	0562ce90			.tim
	0563fdc0	38452		em13.emd	0469927c
							04701bc4
							07522e86
	05684310			.tim
	0582ffc0			.tim
	05891080			.tim
	058df1f0			.tim
	05d90630	41713		em12.tim	131b9200
	05da3560	41746		em12.emd	05d06740
	05de8d10	41867		em17.tim	0c562420
	05e04f40	41916		em17.emd	020ac310
	063df170	44525		em52.tim
	063f20a0	44558		em52.emd
	06407490	44595		em55.tim
	0641a3c0	44628		em55.emd
	0642ee80	44664		em5a.tim
	06441db0	44697		em5a.emd
	06456870	44733		em5c.tim
	064697a0	44766		em5c.emd
	06f92ce0	49742		em21.tim
	06f9c8d0	49759		em21.emd
	08763450	60359		em2c.tim
	08776380	60392		em2c.emd
	0899f310	61355		em32.tim	0167a330
	089a8f40	61372		em32.emd
	08d09ba0			em19.tim
	08d38d00	62960		em19.emd
	08d7ede0			.tim
	08dd7df0			.tim
	08e271d0			.tim
	08e7f8b0	63528		em27.tim
	08e894e0	63546		em27.emd
	08eaa9c0			.tim
	08f131e0			.tim
	090cefd0			.tim
	09140af0			.tim
	0918da10			.tim
	091c2750			.tim
	093fa5c0			.tim
	0946f800			.tim
	094d76f0			.tim
	0971d1e0			.tim
	0977ebc0			.tim
	097c3a50			em50.tim
	097d6970	67693		em50.emd
	097ed8f0			.tim
	09a305f0			.tim
	09a99740			.tim
	09b157c0			.tim
	09b5a640			.tim
	09da6d60			.tim
	09de7da0			.tim
	09e15370	70477		.tim
	09e5a1f0	70597		.tim
	0a180e60			em1a.tim
	0a1affc0	72084		em1a.emd
	0a1f3be0			.tim
	0a43e980			.tim
	0a5cb610	73915		em33.tim
	0a5f1470	73981		em33.emd	026334e0
	0a9a6760	75633		em51.tim
	0a9b9690	75667		em51.emd
	0ac17360			.tim
	0ac8f390			em5b.tim
	0aca22c0	76964		em5b.emd
					em70.tim
	0aca22c0	77409		em70.emd
	0acb76b0			em71.tim
	0acca5e0	77034		em71.emd
	01e5a1c4			em62.tim (no 2352 sectors)
	0acca5e0	77034		em62.emd
	01ea9a38			em63.tim (no 2352 sectors)
	0acf2900	77104		em63.emd
	01ef939c			em64.tim (no 2352 sectors)
	0ad1ac20	77174		em64.emd
	01f488e0			em65.tim (no 2352 sectors)
	0ad42f40	77244		em65.emd
	01f98598			em66.tim (no 2352 sectors)
	0ad6b260	77314		em66.emd	0251cb40
	0c0c0e90	85933		em3a.tim	12d9fa70
	0c0dd0d0	85983		em3a.emd	040b7a50
	0f2b02c0	108196		em26.tim
	0f2b9ef0	108213		em26.emd
					em36.tim
	0f3561f0	108485		em36.emd
	0f38cac0	108580		em37.tim
	0f3966f0	108597		em37.emd
	0f53b560	109330		em25.tim
	0f54e490	109363		em25.emd
	0faa8f20	111750		em40.tim
	0fab2b50	111767		em40.emd
	0ff1c320	113734		em35.tim
	0ff4b480	113816		em35.emd
					em3b.tim
	0ff84b40	113916		em3b.emd
	01e0a990			em5f.tim (no 2352 sectors)
	10769f40	117436		em5f.emd
	115ec930	123905		em13.tim	01f1bb40
							0298ce20
	1291d9a0	132461		em1b.tim
	12955dd0	132559		em1b.emd
	12bc0df0	133637		em22.tim
	13490fa0	137566		em56.tim
	134a3ed0	137599		em56.emd
	13a67ab0	140169		em24.tim
	13a7a9e0	140202		em24.emd
	13d92770	141581		em22.emd
	14a60ad0	147289		em30.tim
	14a86940	147356		em30.emd
	157021b0	152921		em18.tim	009b65e0
	15728010	152987		em18.emd
	1677e0a0	160270		em60.tim
	16790fd0	160303		em60.emd
	1861e6c0	173924		em61.tim
	186315f0	173957		em61.emd
	16c104b0	162307		em3f.tim
	16c233f0	162341		em3f.emd
	1715f790	164675		em38.tim
	171855f0	164741		em38.emd
	171d2e30	164875		em39.tim
	171dca70	164893		em39.emd
	174a9470	166141		em3e.tim
	174bc3a0	166174		em3e.emd
	18645780	173992		em67.tim
	186586b0	174025		em67.emd
					em57.tim
	1ce9b730	206241		em57.emd

					em2e.tim
					em2e.emd
					em5d.tim
					em5d.emd
					em5e.tim
					em5e.emd
*/

/*--- Variables ---*/

static int game_lang = 'u';

/*--- Functions prototypes ---*/

static void re3ps1game_shutdown(void);

static void re3ps1game_loadbackground(void);

static void re3ps1game_loadroom(void);
static int re3ps1game_loadroom_ard(const char *filename);

static void load_font(void);
static void get_char(int ascii, int *x, int *y, int *w, int *h);

/*--- Functions ---*/

void re3ps1game_init(state_t *game_state)
{
	game_state->priv_load_background = re3ps1game_loadbackground;
	game_state->priv_load_room = re3ps1game_loadroom;
	game_state->priv_shutdown = re3ps1game_shutdown;

	game_state->movies_list = (char **) re3ps1game_movies;

	if (state_game_file_exists("cd_data/etc/sele_obf.tim")) {
		game_lang = 'f';
	}

	game_state->load_font = load_font;
	game_state->get_char = get_char;
}

static void re3ps1game_shutdown(void)
{
}

static void re3ps1game_loadbackground(void)
{
	char *filepath;

	filepath = malloc(strlen(re3ps1game_bg)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re3ps1game_bg, game_state.num_stage, game_state.num_stage, game_state.num_room);

	logMsg(1, "bss: Start loading %s ...\n", filepath);

	logMsg(1, "bss: %s loading %s ...\n",
		background_bss_load(filepath, CHUNK_SIZE, 0) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static void re3ps1game_loadroom(void)
{
	char *filepath;

	filepath = malloc(strlen(re3ps1game_room)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, re3ps1game_room, game_state.num_stage, game_state.num_stage, game_state.num_room);

	logMsg(1, "ard: Start loading %s ...\n", filepath);

	logMsg(1, "ard: %s loading %s ...\n",
		re3ps1game_loadroom_ard(filepath) ? "Done" : "Failed",
		filepath);

	free(filepath);
}

static int re3ps1game_loadroom_ard(const char *filename)
{
	PHYSFS_sint64 length;
	Uint8 *ard_file;
	ard_object_t *ard_object;
	int i, count;
	Uint32 offset, len;
	void *file;

	ard_file = (Uint8 *) FS_Load(filename, &length);
	if (!ard_file) {
		return 0;
	}

	count = ((ard_header_t *) ard_file)->count;
	count = SDL_SwapLE32(count);
/*
	offset = 0x800;
	len = 0;
	ard_object = (ard_object_t *) (&ard_file[8]);
	for (i=0; i<count; i++) {
		logMsg(1, "ard: object %d at offset 0x%08x\n", i,offset);
		len = SDL_SwapLE32(ard_object->length);

		offset += len;
		offset |= 0x7ff;
		offset ++;
		ard_object++;
	}
*/
	offset = 0x800;
	len = 0;
	ard_object = (ard_object_t *) (&ard_file[8]);
	for (i=0; i<count; i++) {
		len = SDL_SwapLE32(ard_object->length);
		if (i==8) {
			/* Stop on embedded RDT file */
			break;
		}
		offset += len;
		offset |= 0x7ff;
		offset ++;
		ard_object++;
	}

	file = malloc(len);
	if (!file) {
		free(ard_file);
		return 0;
	}

	logMsg(3, "ard: Loading embedded RDT file from offset 0x%08x\n", offset);
	memcpy(file, &ard_file[offset], len);

	game_state.room = room_create(file, len);
	if (!game_state.room) {
		free(file);
		free(ard_file);
		return 0;
	}

	room_rdt2_init(game_state.room);

	free(ard_file);
	return 1;
}

static void load_font(void)
{
	Uint8 *font_file;
	PHYSFS_sint64 length;
	int retval = 0;
	char *filepath;
	const char *filename = re3ps1game_font;

	filepath = malloc(strlen(filename)+8);
	if (!filepath) {
		fprintf(stderr, "Can not allocate mem for filepath\n");
		return;
	}
	sprintf(filepath, filename, game_lang);

	logMsg(1, "Loading font from %s...\n", filepath);

	font_file = FS_Load(filepath, &length);
	if (font_file) {
		game_state.font = render.createTexture(0);
		if (game_state.font) {
			game_state.font->load_from_tim(game_state.font, font_file);
			retval = 1;
		}

		free(font_file);
	}

	logMsg(1, "Loading font from %s... %s\n", filepath, retval ? "Done" : "Failed");

	free(filepath);
}

static void get_char(int ascii, int *x, int *y, int *w, int *h)
{
	*x = *y = 0;
	*w = 8;
	*h = 10;

	if ((ascii<=32) || (ascii>=96+27)) {
		return;
	}

	ascii -= 32;
	*x = 128+ ((ascii & 15)<<3);
	*y = 176+ ((ascii>>4)*10);
}
