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
	Uint32	count;	/* Number of sectors */
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
static const re3cd_raw_t re3_sles_02530[]={
	{126,0,"capcom.tim"},
	{202,0,"continue.tim"},
	{284,0,"core00f.tim"},
	{316,0,"died00f.tim"},
	{341,0,"eidos.tim"},
	{427,0,"epi_sel.tim"},
	{503,0,"epi_none.tim"},
	{520,0,"insta_i.tim"},
	{537,0,"insta_t0.tim"},
	{549,0,"insta_t1.tim"},
	{561,0,"insta_t2.tim"},
	{573,0,"insta_t3.tim"},
	{585,0,"insta_t4.tim"},
	{597,0,"insta_t5.tim"},
	{609,0,"insta_t6.tim"},
	{621,0,"insta_t7.tim"},
	{633,0,"insta_t8.tim"},
	{645,0,"dario_i.tim"},
	{662,0,"dario_t0.tim"},
	{674,0,"dario_t1.tim"},
	{686,0,"dario_t2.tim"},
	{698,0,"dario_t3.tim"},
	{710,0,"dario_t4.tim"},
	{722,0,"dmerc_i.tim"},
	{739,0,"dmerc_t0.tim"},
	{751,0,"dmerc_t1.tim"},
	{763,0,"dmerc_t2.tim"},
	{775,0,"dmerc_t3.tim"},
	{787,0,"dmerc_t4.tim"},
	{799,0,"dmerc_t5.tim"},
	{811,0,"dmerc_t6.tim"},
	{823,0,"fax1_i.tim"},
	{840,0,"fax1_t0.tim"},
	{852,0,"fax1_t1.tim"},
	{864,0,"marvin_i.tim"},
	{881,0,"marvin_t0.tim"},
	{893,0,"marvin_t1.tim"},
	{905,0,"marvin_t2.tim"},
	{917,0,"marvin_t3.tim"},
	{929,0,"marvin_t4.tim"},
	{941,0,"david_i.tim"},
	{958,0,"david_t0.tim"},
	{970,0,"david_t1.tim"},
	{982,0,"david_t2.tim"},
	{994,0,"david_t3.tim"},
	{1006,0,"david_t4.tim"},
	{1018,0,"town_i.tim"},
	{1035,0,"town_t0.tim"},
	{1047,0,"town_t1.tim"},
	{1059,0,"town_t2.tim"},
	{1071,0,"town_t3.tim"},
	{1083,0,"town_t4.tim"},
	{1095,0,"report_i.tim"},
	{1112,0,"report_t0.tim"},
	{1124,0,"report_t1.tim"},
	{1136,0,"report_t2.tim"},
	{1148,0,"report_t3.tim"},
	{1160,0,"report_t4.tim"},
	{1172,0,"report_t5.tim"},
	{1184,0,"inst_i.tim"},
	{1201,0,"inst_t0.tim"},
	{1213,0,"inst_t1.tim"},
	{1225,0,"inst_t2.tim"},
	{1237,0,"inst_t3.tim"},
	{1249,0,"inst_t4.tim"},
	{1261,0,"nmerc_i.tim"},
	{1278,0,"nmerc_t0.tim"},
	{1290,0,"nmerc_t1.tim"},
	{1302,0,"nmerc_t2.tim"},
	{1314,0,"nmerc_t3.tim"},
	{1326,0,"nmerc_t4.tim"},
	{1338,0,"nmerc_t5.tim"},
	{1350,0,"nmerc_t6.tim"},
	{1362,0,"postcard_i.tim"},
	{1379,0,"postcard_t0.tim"},
	{1391,0,"postcard_t1.tim"},
	{1403,0,"postcard_t2.tim"},
	{1415,0,"super_i.tim"},
	{1432,0,"super_t0.tim"},
	{1444,0,"super_t1.tim"},
	{1456,0,"super_t2.tim"},
	{1468,0,"super_t3.tim"},
	{1480,0,"super_t4.tim"},
	{1492,0,"super_t5.tim"},
	{1504,0,"order_i.tim"},
	{1521,0,"order_t0.tim"},
	{1533,0,"order_t1.tim"},
	{1545,0,"order_t2.tim"},
	{1557,0,"director_i.tim"},
	{1574,0,"director_t0.tim"},
	{1586,0,"director_t1.tim"},
	{1598,0,"director_t2.tim"},
	{1610,0,"director_t3.tim"},
	{1622,0,"director_t4.tim"},
	{1634,0,"director_t5.tim"},
	{1646,0,"director_t6.tim"},
	{1658,0,"manager1_i.tim"},
	{1675,0,"manager1_t0.tim"},
	{1687,0,"manager1_t1.tim"},
	{1699,0,"manager1_t2.tim"},
	{1711,0,"manager1_t3.tim"},
	{1723,0,"manager1_t4.tim"},
	{1735,0,"manager1_t5.tim"},
	{1747,0,"manager1_t6.tim"},
	{1759,0,"manager1_t7.tim"},
	{1771,0,"security_i.tim"},
	{1788,0,"security_t0.tim"},
	{1800,0,"security_t1.tim"},
	{1812,0,"security_t2.tim"},
	{1824,0,"security_t3.tim"},
	{1836,0,"meca_i.tim"},
	{1853,0,"meca_t0.tim"},
	{1865,0,"meca_t1.tim"},
	{1877,0,"meca_t2.tim"},
	{1889,0,"meca_t3.tim"},
	{1901,0,"meca_t4.tim"},
	{1913,0,"kendo_i.tim"},
	{1930,0,"kendo_t0.tim"},
	{1942,0,"kendo_t1.tim"},
	{1954,0,"kendo_t2.tim"},
	{1966,0,"kendo_t3.tim"},
	{1978,0,"kendo_t4.tim"},
	{1990,0,"kendo_t5.tim"},
	{2002,0,"manager2_i.tim"},
	{2019,0,"manager2_t0.tim"},
	{2031,0,"manager2_t1.tim"},
	{2043,0,"manager2_t2.tim"},
	{2055,0,"manager2_t3.tim"},
	{2067,0,"manager2_t4.tim"},
	{2079,0,"manager2_t5.tim"},
	{2091,0,"medic_i.tim"},
	{2108,0,"medic_t0.tim"},
	{2120,0,"medic_t1.tim"},
	{2132,0,"medic_t2.tim"},
	{2144,0,"medic_t3.tim"},
	{2156,0,"fax2_i.tim"},
	{2173,0,"fax2_t0.tim"},
	{2185,0,"fax2_t1.tim"},
	{2197,0,"incinerator_i.tim"},
	{2214,0,"incinerator_t0.tim"},
	{2226,0,"incinerator_t1.tim"},
	{2238,0,"incinerator_t2.tim"},
	{2250,0,"incinerator_t3.tim"},
	{2262,0,"photoa_i.tim"},
	{2279,0,"photoa_t0.tim"},
	{2291,0,"photoa_t1.tim"},
	{2303,0,"photoa_t2.tim"},
	{2315,0,"photoa_t3.tim"},
	{2327,0,"photob_i.tim"},
	{2344,0,"photob_t0.tim"},
	{2356,0,"photob_t1.tim"},
	{2368,0,"photob_t2.tim"},
	{2380,0,"photob_t3.tim"},
	{2392,0,"photoc_i.tim"},
	{2409,0,"photoc_t0.tim"},
	{2421,0,"photoc_t1.tim"},
	{2433,0,"photoc_t2.tim"},
	{2445,0,"photoe_i.tim"},
	{2462,0,"photoe_t0.tim"},
	{2474,0,"photoe_t1.tim"},
	{2486,0,"photoe_t2.tim"},
	{2498,0,"photod_i.tim"},
	{2515,0,"photod_t0.tim"},
	{2527,0,"photod_t1.tim"},
	{2539,0,"photod_t2.tim"},
	{2551,0,"photod_t3.tim"},
	{2563,0,"belfry_i.tim"},
	{2580,0,"belfry_t0.tim"},
	{2592,0,"belfry_t1.tim"},
	{2604,0,"belfry_t2.tim"},
	{2616,0,"belfry_t3.tim"},
	{2628,0,"instb_i.tim"},
	{2645,0,"instb_t0.tim"},
	{2657,0,"instb_t1.tim"},
	{2669,0,"instb_t2.tim"},
	{2681,0,"instb_t3.tim"},
	{2693,0,"instb_t4.tim"},
	{2705,0,"instb_t5.tim"},
	{2717,0,"instb_t6.tim"},
	{2729,0,"instb_t7.tim"},
	{2741,0,"secret_i.tim"},
	{2758,0,"secret_t0.tim"},
	{2770,0,"secret_t1.tim"},
	{2782,0,"secret_t2.tim"},
	{2794,0,"djill_i.tim"},
	{2811,0,"djill_t0.tim"},
	{2823,0,"djill_t1.tim"},
	{2835,0,"djill_t2.tim"},
	{2847,0,"djill_t3.tim"},
	{2859,0,"djill_t4.tim"},
	{2871,0,"djill_t5.tim"},
	{2883,0,"djill_t6.tim"},
	{2895,0,"djill_t7.tim"},
	{2907,0,"fileif.tim"},
	{2924,0,"filei.tim"},
	{2941,0,"fontst0f.tim"},
	{3006,0,"fontst1f.tim"},
	{3071,0,"fontst2f.tim"},
	{3136,0,"fontst3f.tim"},
	{3201,0,"fontst4f.tim"},
	{3266,0,"fontst5f.tim"},
	{3331,0,"fontst6f.tim"},
	{3396,0,"fontst7f.tim"},
	{3461,0,"nemesis_icon.tim"},
	{3505,0,"colt1.tim"},
	{3510,0,"knife.tim"},
	{3515,0,"colt2.tim"},
	{3520,0,"colt3.tim"},
	{3525,0,"shotgun1.tim"},
	{3530,0,"magnum.tim"},
	{3535,0,"grenade_launcher1.tim"},
	{3540,0,"grenade_launcher2.tim"},
	{3545,0,"grenade_launcher3.tim"},
	{3550,0,"grenade_launcher4.tim"},
	{3555,0,"rocket_launcher.tim"},
	{3560,0,"gattling.tim"},
	{3565,0,"supercolt.tim"},
	{3570,0,"lasercolt.tim"},
	{3575,0,"m16_1.tim"},
	{3580,0,"m16_2.tim"},
	{3585,0,"shotgun2.tim"},
	{3590,0,"colt4.tim"}, copy
	{3595,0,"colt5.tim"}, copy
	{3600,0,"shotgun.tim"}, copy
	{3605,0,"supercolt.tim"}, copy
	{3610,0,"photoa_t2.tim"},
	{3615,0,"photoa_t2.tim"},
	{3620,0,"photoa_t2.tim"},
	{3625,0,"photoa_t2.tim"},

Sector 3630 (offset 0x008246b8): 8 bits TIM image
Sector 3635 (offset 0x008274a8): 8 bits TIM image
Sector 3640 (offset 0x0082a298): 8 bits TIM image
Sector 3645 (offset 0x0082d088): 8 bits TIM image
Sector 3650 (offset 0x0082fe78): 8 bits TIM image
Sector 3655 (offset 0x00832c68): 8 bits TIM image
Sector 3660 (offset 0x00835a58): 8 bits TIM image
Sector 3665 (offset 0x00838848): 8 bits TIM image
Sector 3670 (offset 0x0083b638): 8 bits TIM image
Sector 3675 (offset 0x0083e428): 8 bits TIM image
Sector 3680 (offset 0x00841218): 8 bits TIM image
Sector 3685 (offset 0x00844008): 8 bits TIM image
Sector 3690 (offset 0x00846df8): 8 bits TIM image
Sector 3695 (offset 0x00849be8): 8 bits TIM image
Sector 3700 (offset 0x0084c9d8): 8 bits TIM image
Sector 3705 (offset 0x0084f7c8): 8 bits TIM image
Sector 3710 (offset 0x008525b8): 8 bits TIM image
Sector 3715 (offset 0x008553a8): 8 bits TIM image
Sector 3720 (offset 0x00858198): 8 bits TIM image
Sector 3725 (offset 0x0085af88): 8 bits TIM image
Sector 3730 (offset 0x0085dd78): 8 bits TIM image
Sector 3735 (offset 0x00860b68): 8 bits TIM image
Sector 3740 (offset 0x00863958): 8 bits TIM image
Sector 3745 (offset 0x00866748): 8 bits TIM image
Sector 3750 (offset 0x00869538): 8 bits TIM image
Sector 3755 (offset 0x0086c328): 8 bits TIM image
Sector 3760 (offset 0x0086f118): 8 bits TIM image
Sector 3765 (offset 0x00871f08): 8 bits TIM image
Sector 3770 (offset 0x00874cf8): 8 bits TIM image
Sector 3775 (offset 0x00877ae8): 8 bits TIM image
Sector 3780 (offset 0x0087a8d8): 8 bits TIM image
Sector 3785 (offset 0x0087d6c8): 8 bits TIM image
Sector 3790 (offset 0x008804b8): 8 bits TIM image
Sector 3795 (offset 0x008832a8): 8 bits TIM image
Sector 3800 (offset 0x00886098): 8 bits TIM image
Sector 3805 (offset 0x00888e88): 8 bits TIM image
Sector 3810 (offset 0x0088bc78): 8 bits TIM image
Sector 3815 (offset 0x0088ea68): 8 bits TIM image
Sector 3820 (offset 0x00891858): 8 bits TIM image
Sector 3825 (offset 0x00894648): 8 bits TIM image
Sector 3830 (offset 0x00897438): 8 bits TIM image
Sector 3835 (offset 0x0089a228): 8 bits TIM image
Sector 3840 (offset 0x0089d018): 8 bits TIM image
Sector 3845 (offset 0x0089fe08): 8 bits TIM image
Sector 3850 (offset 0x008a2bf8): 8 bits TIM image
Sector 3855 (offset 0x008a59e8): 8 bits TIM image
Sector 3860 (offset 0x008a87d8): 8 bits TIM image
Sector 3865 (offset 0x008ab5c8): 8 bits TIM image
Sector 3870 (offset 0x008ae3b8): 8 bits TIM image
Sector 3875 (offset 0x008b11a8): 8 bits TIM image
Sector 3880 (offset 0x008b3f98): 8 bits TIM image
Sector 3885 (offset 0x008b6d88): 8 bits TIM image
Sector 3890 (offset 0x008b9b78): 8 bits TIM image
Sector 3895 (offset 0x008bc968): 8 bits TIM image
Sector 3900 (offset 0x008bf758): 8 bits TIM image
Sector 3905 (offset 0x008c2548): 8 bits TIM image
Sector 3910 (offset 0x008c5338): 8 bits TIM image
Sector 3915 (offset 0x008c8128): 8 bits TIM image
Sector 3920 (offset 0x008caf18): 8 bits TIM image
Sector 3925 (offset 0x008cdd08): 8 bits TIM image
Sector 3930 (offset 0x008d0af8): 8 bits TIM image
Sector 3935 (offset 0x008d38e8): 8 bits TIM image
Sector 3940 (offset 0x008d66d8): 8 bits TIM image
Sector 3945 (offset 0x008d94c8): 8 bits TIM image
Sector 3950 (offset 0x008dc2b8): 8 bits TIM image
Sector 3955 (offset 0x008df0a8): 8 bits TIM image
Sector 3960 (offset 0x008e1e98): 8 bits TIM image
Sector 3965 (offset 0x008e4c88): 8 bits TIM image
Sector 3970 (offset 0x008e7a78): 8 bits TIM image
Sector 3975 (offset 0x008ea868): 8 bits TIM image
Sector 3980 (offset 0x008ed658): 8 bits TIM image
Sector 3985 (offset 0x008f0448): 8 bits TIM image
Sector 3990 (offset 0x008f3238): 8 bits TIM image
Sector 3995 (offset 0x008f6028): 8 bits TIM image
Sector 4000 (offset 0x008f8e18): 8 bits TIM image
Sector 4005 (offset 0x008fbc08): 8 bits TIM image
Sector 4010 (offset 0x008fe9f8): 8 bits TIM image
Sector 4015 (offset 0x009017e8): 8 bits TIM image
Sector 4020 (offset 0x009045d8): 8 bits TIM image
Sector 4025 (offset 0x009073c8): 8 bits TIM image
Sector 4030 (offset 0x0090a1b8): 8 bits TIM image
Sector 4035 (offset 0x0090cfa8): 8 bits TIM image
Sector 4040 (offset 0x0090fd98): 8 bits TIM image
Sector 4045 (offset 0x00912b88): 8 bits TIM image
Sector 4050 (offset 0x00915978): 8 bits TIM image
Sector 4055 (offset 0x00918768): 8 bits TIM image
Sector 4060 (offset 0x0091b558): 8 bits TIM image
Sector 4065 (offset 0x0091e348): 8 bits TIM image
Sector 4070 (offset 0x00921138): 8 bits TIM image
Sector 4075 (offset 0x00923f28): 8 bits TIM image
Sector 4080 (offset 0x00926d18): 8 bits TIM image
Sector 4085 (offset 0x00929b08): 8 bits TIM image
Sector 4090 (offset 0x0092c8f8): 8 bits TIM image
Sector 4095 (offset 0x0092f6e8): 8 bits TIM image
Sector 4100 (offset 0x009324d8): 8 bits TIM image
Sector 4105 (offset 0x009352c8): 8 bits TIM image
Sector 4110 (offset 0x009380b8): 8 bits TIM image
Sector 4115 (offset 0x0093aea8): 8 bits TIM image
Sector 4120 (offset 0x0093dc98): 8 bits TIM image
Sector 4125 (offset 0x00940a88): 8 bits TIM image
Sector 4130 (offset 0x00943878): 8 bits TIM image
Sector 4135 (offset 0x00946668): 8 bits TIM image
Sector 4140 (offset 0x00949458): 8 bits TIM image
Sector 4145 (offset 0x0094c248): 8 bits TIM image
Sector 4150 (offset 0x0094f038): 8 bits TIM image
Sector 4155 (offset 0x00951e28): 8 bits TIM image
Sector 4160 (offset 0x00954c18): 8 bits TIM image
Sector 4165 (offset 0x00957a08): 8 bits TIM image
Sector 4170 (offset 0x0095a7f8): 8 bits TIM image
Sector 4309 (offset 0x009aa508): 16 bits TIM image
Sector 4385 (offset 0x009d5f48): 8 bits TIM image
Sector 4728 (offset 0x00a9ae98): 16 bits TIM image
Sector 4804 (offset 0x00ac68d8): 16 bits TIM image
Sector 4880 (offset 0x00af2318): 16 bits TIM image
Sector 4956 (offset 0x00b1dd58): 16 bits TIM image
Sector 5032 (offset 0x00b49798): 16 bits TIM image
Sector 5108 (offset 0x00b751d8): 16 bits TIM image
Sector 5184 (offset 0x00ba0c18): 16 bits TIM image
Sector 5260 (offset 0x00bcc658): 16 bits TIM image
Sector 5336 (offset 0x00bf8098): 16 bits TIM image
Sector 5412 (offset 0x00c23ad8): 16 bits TIM image
Sector 5488 (offset 0x00c4f518): 8 bits TIM image
Sector 5712 (offset 0x00ccff18): 16 bits TIM image
Sector 5863 (offset 0x00d26a68): 8 bits TIM image
Sector 6087 (offset 0x00da7468): 16 bits TIM image
Sector 6238 (offset 0x00dfdfb8): 4 bits TIM image
Sector 6271 (offset 0x00e10ee8): 16 bits TIM image
Sector 6347 (offset 0x00e3c928): 16 bits TIM image
Sector 6423 (offset 0x00e68368): 16 bits TIM image
Sector 6505 (offset 0x00e974c8): 4 bits TIM image
Sector 6508 (offset 0x00e99058): 16 bits TIM image
Sector 6584 (offset 0x00ec4a98): 8 bits TIM image
Sector 6633 (offset 0x00ee0cc8): 16 bits TIM image
Sector 6709 (offset 0x00f0c708): 16 bits TIM image
Sector 6785 (offset 0x00f38148): 16 bits TIM image
Sector 6861 (offset 0x00f63b88): 16 bits TIM image
Sector 6937 (offset 0x00f8f5c8): 8 bits TIM image
Sector 6986 (offset 0x00fab7f8): 4 bits TIM image
Sector 7185 (offset 0x0101dc48): 8 bits TIM image
Sector 7221 (offset 0x01032708): 8 bits TIM image
Sector 7256 (offset 0x01046898): 8 bits TIM image
Sector 7291 (offset 0x0105aa28): 8 bits TIM image
Sector 7326 (offset 0x0106ebb8): 4 bits TIM image
Sector 7331 (offset 0x010719a8): 8 bits TIM image
Sector 7607 (offset 0x01110168): 4 bits TIM image
Sector 7672 (offset 0x01135698): 16 bits TIM image
Sector 7914 (offset 0x011c05f8): 16 bits TIM image
Sector 16181 (offset 0x0244b708): 8 bits TIM image
Sector 16230 (offset 0x02467938): EMD model (maybe)
Sector 16301 (offset 0x02490588): 8 bits TIM image
Sector 16334 (offset 0x024a34b8): EMD model (maybe)
Sector 16370 (offset 0x024b7f78): 8 bits TIM image
Sector 16403 (offset 0x024caea8): EMD model (maybe)
Sector 17567 (offset 0x027674e8): 8 bits TIM image
Sector 17616 (offset 0x02783718): EMD model (maybe)
Sector 17739 (offset 0x027ca128): 8 bits TIM image
Sector 17788 (offset 0x027e6358): EMD model (maybe)
Sector 18405 (offset 0x02948808): 8 bits TIM image
Sector 18454 (offset 0x02964a38): EMD model (maybe)
Sector 18565 (offset 0x029a4608): 8 bits TIM image
Sector 18631 (offset 0x029ca468): EMD model (maybe)
Sector 19583 (offset 0x02becee8): 8 bits TIM image
Sector 19649 (offset 0x02c12d48): EMD model (maybe)
Sector 19714 (offset 0x02c38278): 8 bits TIM image
Sector 19747 (offset 0x02c4b1a8): EMD model (maybe)
Sector 20673 (offset 0x02e5ed48): 8 bits TIM image
Sector 20771 (offset 0x02e971a8): EMD model (maybe)
Sector 20839 (offset 0x02ebe268): 8 bits TIM image
Sector 20872 (offset 0x02ed1198): EMD model (maybe)
Sector 21846 (offset 0x03100638): 8 bits TIM image
Sector 21912 (offset 0x03126498): EMD model (maybe)
Sector 23001 (offset 0x033979c8): 8 bits TIM image
Sector 23034 (offset 0x033aa8f8): EMD model (maybe)
Sector 23155 (offset 0x033f00a8): 8 bits TIM image
Sector 23188 (offset 0x03402fd8): EMD model (maybe)
Sector 23309 (offset 0x03448788): 8 bits TIM image
Sector 23342 (offset 0x0345b6b8): EMD model (maybe)
Sector 23461 (offset 0x0349fc08): 8 bits TIM image
Sector 23510 (offset 0x034bbe38): EMD model (maybe)
Sector 23581 (offset 0x034e4a88): 8 bits TIM image
Sector 23614 (offset 0x034f79b8): EMD model (maybe)
Sector 24396 (offset 0x036b8a58): 8 bits TIM image
Sector 24478 (offset 0x036e7bb8): EMD model (maybe)
Sector 25141 (offset 0x03864708): 8 bits TIM image
Sector 25174 (offset 0x03877638): EMD model (maybe)
Sector 25943 (offset 0x03a30f68): 8 bits TIM image
Sector 25992 (offset 0x03a4d198): EMD model (maybe)
Sector 26770 (offset 0x03c0bd78): 8 bits TIM image
Sector 26819 (offset 0x03c27fa8): EMD model (maybe)
Sector 26968 (offset 0x03c7d898): 8 bits TIM image
Sector 27001 (offset 0x03c907c8): EMD model (maybe)
Sector 27150 (offset 0x03ce60b8): 8 bits TIM image
Sector 27199 (offset 0x03d022e8): EMD model (maybe)
Sector 28058 (offset 0x03eef6f8): 8 bits TIM image
Sector 28124 (offset 0x03f15558): EMD model (maybe)
Sector 28195 (offset 0x03f3e1a8): 8 bits TIM image
Sector 28212 (offset 0x03f47dd8): EMD model (maybe)
Sector 28981 (offset 0x04101708): 8 bits TIM image
Sector 29014 (offset 0x04114638): EMD model (maybe)
Sector 29133 (offset 0x04158b88): 8 bits TIM image
Sector 29182 (offset 0x04174db8): EMD model (maybe)
Sector 29643 (offset 0x0427d928): 8 bits TIM image
Sector 29692 (offset 0x04299b58): EMD model (maybe)
Sector 29839 (offset 0x042ee1e8): 8 bits TIM image
Sector 29888 (offset 0x0430a418): EMD model (maybe)
Sector 30551 (offset 0x04486f68): 8 bits TIM image
Sector 30600 (offset 0x044a3198): EMD model (maybe)
Sector 30671 (offset 0x044cbde8): 8 bits TIM image
Sector 30704 (offset 0x044ded18): EMD model (maybe)
Sector 32155 (offset 0x04820028): 8 bits TIM image
Sector 32188 (offset 0x04832f58): EMD model (maybe)
Sector 32309 (offset 0x04878708): 8 bits TIM image
Sector 32358 (offset 0x04894938): EMD model (maybe)
Sector 32478 (offset 0x048d97b8): 8 bits TIM image
Sector 32527 (offset 0x048f59e8): EMD model (maybe)
Sector 33292 (offset 0x04aace58): 8 bits TIM image
Sector 33341 (offset 0x04ac9088): EMD model (maybe)
Sector 34032 (offset 0x04c55d18): 8 bits TIM image
Sector 34081 (offset 0x04c71f48): EMD model (maybe)
Sector 34204 (offset 0x04cb8958): 8 bits TIM image
Sector 34237 (offset 0x04ccb888): EMD model (maybe)
Sector 34356 (offset 0x04d0fdd8): 8 bits TIM image
Sector 34405 (offset 0x04d2c008): EMD model (maybe)
Sector 35101 (offset 0x04ebba88): 8 bits TIM image
Sector 35183 (offset 0x04eeabe8): EMD model (maybe)
Sector 35270 (offset 0x04f1cb38): 8 bits TIM image
Sector 35303 (offset 0x04f2fa68): EMD model (maybe)
Sector 35406 (offset 0x04f6acb8): 8 bits TIM image
Sector 35455 (offset 0x04f86ee8): EMD model (maybe)
Sector 37498 (offset 0x0541c0f8): 8 bits TIM image
Sector 37531 (offset 0x0542f028): EMD model (maybe)
Sector 37652 (offset 0x054747d8): 8 bits TIM image
Sector 37701 (offset 0x05490a08): EMD model (maybe)
Sector 37824 (offset 0x054d7418): 8 bits TIM image
Sector 37873 (offset 0x054f3648): EMD model (maybe)
Sector 38638 (offset 0x056aaab8): 8 bits TIM image
Sector 38687 (offset 0x056c6ce8): EMD model (maybe)
Sector 40144 (offset 0x05a0b718): 8 bits TIM image
Sector 40242 (offset 0x05a43b78): EMD model (maybe)
Sector 40310 (offset 0x05a6ac38): 8 bits TIM image
Sector 40343 (offset 0x05a7db68): EMD model (maybe)
Sector 40380 (offset 0x05a92f58): 8 bits TIM image
Sector 40413 (offset 0x05aa5e88): EMD model (maybe)
Sector 40449 (offset 0x05aba948): 8 bits TIM image
Sector 40482 (offset 0x05acd878): EMD model (maybe)
Sector 40518 (offset 0x05ae2338): 8 bits TIM image
Sector 40551 (offset 0x05af5268): EMD model (maybe)
Sector 41605 (offset 0x05d52608): 8 bits TIM image
Sector 41703 (offset 0x05d8aa68): EMD model (maybe)
Sector 42219 (offset 0x05eb2f28): 8 bits TIM image
Sector 42268 (offset 0x05ecf158): EMD model (maybe)
Sector 42391 (offset 0x05f15b68): 8 bits TIM image
Sector 42440 (offset 0x05f31d98): EMD model (maybe)
Sector 43042 (offset 0x0608b878): 8 bits TIM image
Sector 43091 (offset 0x060a7aa8): EMD model (maybe)
Sector 43202 (offset 0x060e7678): 8 bits TIM image
Sector 43268 (offset 0x0610d4d8): EMD model (maybe)
Sector 44229 (offset 0x06335208): 8 bits TIM image
Sector 44295 (offset 0x0635b068): EMD model (maybe)
Sector 45374 (offset 0x065c69b8): 8 bits TIM image
Sector 45440 (offset 0x065ec818): EMD model (maybe)
Sector 45527 (offset 0x0661e768): 8 bits TIM image
Sector 45544 (offset 0x06628398): EMD model (maybe)
Sector 45606 (offset 0x0664bd38): 8 bits TIM image
Sector 45655 (offset 0x06667f68): EMD model (maybe)
Sector 46785 (offset 0x068f0d48): 8 bits TIM image
Sector 46851 (offset 0x06916ba8): EMD model (maybe)
Sector 46940 (offset 0x06949d58): 8 bits TIM image
Sector 46973 (offset 0x0695cc88): EMD model (maybe)
Sector 47076 (offset 0x06997ed8): 8 bits TIM image
Sector 47125 (offset 0x069b4108): EMD model (maybe)
Sector 48176 (offset 0x06c0f918): 8 bits TIM image
Sector 48225 (offset 0x06c2bb48): EMD model (maybe)
Sector 48346 (offset 0x06c712f8): 8 bits TIM image
Sector 48395 (offset 0x06c8d528): EMD model (maybe)
Sector 49084 (offset 0x06e18f58): 8 bits TIM image
Sector 49117 (offset 0x06e2be88): EMD model (maybe)
Sector 49220 (offset 0x06e670d8): 8 bits TIM image
Sector 49269 (offset 0x06e83308): EMD model (maybe)
Sector 50539 (offset 0x0715c728): 8 bits TIM image
Sector 50572 (offset 0x0716f658): EMD model (maybe)
Sector 50693 (offset 0x071b4e08): 8 bits TIM image
Sector 50775 (offset 0x071e3f68): EMD model (maybe)
Sector 50862 (offset 0x07215eb8): 8 bits TIM image
Sector 50895 (offset 0x07228de8): EMD model (maybe)
Sector 50998 (offset 0x07264038): 8 bits TIM image
Sector 51047 (offset 0x07280268): EMD model (maybe)
Sector 51633 (offset 0x073d0a48): 8 bits TIM image
Sector 51666 (offset 0x073e3978): EMD model (maybe)
Sector 56042 (offset 0x07db45f8): 8 bits TIM image
Sector 56075 (offset 0x07dc7528): EMD model (maybe)
Sector 56150 (offset 0x07df2638): 8 bits TIM image
Sector 56167 (offset 0x07dfc268): EMD model (maybe)
Sector 56181 (offset 0x07e04308): 8 bits TIM image
Sector 56214 (offset 0x07e17238): EMD model (maybe)
Sector 57073 (offset 0x08004648): 8 bits TIM image
Sector 57106 (offset 0x08017578): EMD model (maybe)
Sector 57177 (offset 0x080401c8): 8 bits TIM image
Sector 57194 (offset 0x08049df8): EMD model (maybe)
Sector 58700 (offset 0x083aaa58): 8 bits TIM image
Sector 58782 (offset 0x083d9bb8): EMD model (maybe)
Sector 58904 (offset 0x0841fc98): 8 bits TIM image
Sector 58970 (offset 0x08445af8): EMD model (maybe)
Sector 59059 (offset 0x08478ca8): 8 bits TIM image
Sector 59092 (offset 0x0848bbd8): EMD model (maybe)
Sector 59197 (offset 0x084c8088): 8 bits TIM image
Sector 59246 (offset 0x084e42b8): EMD model (maybe)
Sector 59351 (offset 0x08520768): 8 bits TIM image
Sector 59368 (offset 0x0852a398): EMD model (maybe)
Sector 59426 (offset 0x0854b878): 8 bits TIM image
Sector 59459 (offset 0x0855e7a8): EMD model (maybe)
Sector 59608 (offset 0x085b4098): 8 bits TIM image
Sector 59657 (offset 0x085d02c8): EMD model (maybe)
Sector 60381 (offset 0x0876fe88): 8 bits TIM image
Sector 60430 (offset 0x0878c0b8): EMD model (maybe)
Sector 60579 (offset 0x087e19a8): 8 bits TIM image
Sector 60612 (offset 0x087f48d8): EMD model (maybe)
Sector 60713 (offset 0x0882e8c8): 8 bits TIM image
Sector 60746 (offset 0x088417f8): EMD model (maybe)
Sector 60805 (offset 0x08863608): 8 bits TIM image
Sector 60854 (offset 0x0887f838): EMD model (maybe)
Sector 61794 (offset 0x08a9b478): 8 bits TIM image
Sector 61876 (offset 0x08aca5d8): EMD model (maybe)
Sector 61998 (offset 0x08b106b8): 8 bits TIM image
Sector 62064 (offset 0x08b36518): EMD model (maybe)
Sector 62179 (offset 0x08b785a8): 8 bits TIM image
Sector 62228 (offset 0x08b947d8): EMD model (maybe)
Sector 63193 (offset 0x08dbe9c8): 8 bits TIM image
Sector 63242 (offset 0x08ddabf8): EMD model (maybe)
Sector 63363 (offset 0x08e203a8): 8 bits TIM image
Sector 63412 (offset 0x08e3c5d8): EMD model (maybe)
Sector 63483 (offset 0x08e65228): 8 bits TIM image
Sector 63516 (offset 0x08e78158): EMD model (maybe)
Sector 63556 (offset 0x08e8f0d8): 8 bits TIM image
Sector 63589 (offset 0x08ea2008): EMD model (maybe)
Sector 64564 (offset 0x090d1dd8): 8 bits TIM image
Sector 64630 (offset 0x090f7c38): EMD model (maybe)
Sector 64747 (offset 0x0913af28): 8 bits TIM image
Sector 64845 (offset 0x09173388): EMD model (maybe)
Sector 64963 (offset 0x091b6fa8): 8 bits TIM image
Sector 65012 (offset 0x091d31d8): EMD model (maybe)
Sector 65083 (offset 0x091fbe28): 8 bits TIM image
Sector 65116 (offset 0x0920ed58): EMD model (maybe)
Sector 66113 (offset 0x0944b548): 8 bits TIM image
Sector 66146 (offset 0x0945e478): EMD model (maybe)
Sector 66221 (offset 0x09489588): 8 bits TIM image
Sector 66238 (offset 0x094931b8): EMD model (maybe)
Sector 66300 (offset 0x094b6b58): 8 bits TIM image
Sector 66349 (offset 0x094d2d88): EMD model (maybe)
Sector 66420 (offset 0x094fb9d8): 8 bits TIM image
Sector 66453 (offset 0x0950e908): EMD model (maybe)
Sector 67825 (offset 0x09822648): 8 bits TIM image
Sector 67907 (offset 0x098517a8): EMD model (maybe)
Sector 68025 (offset 0x098953c8): 8 bits TIM image
Sector 68074 (offset 0x098b15f8): EMD model (maybe)
Sector 69047 (offset 0x09ae0168): 8 bits TIM image
Sector 69113 (offset 0x09b05fc8): EMD model (maybe)
Sector 69738 (offset 0x09c6cdf8): 8 bits TIM image
Sector 69804 (offset 0x09c92c58): EMD model (maybe)
Sector 71202 (offset 0x09fb5878): 8 bits TIM image
Sector 71284 (offset 0x09fe49d8): EMD model (maybe)
Sector 71371 (offset 0x0a016928): 8 bits TIM image
Sector 71404 (offset 0x0a029858): EMD model (maybe)
Sector 71457 (offset 0x0a047f48): 8 bits TIM image
Sector 71490 (offset 0x0a05ae78): EMD model (maybe)
Sector 72545 (offset 0x0a2b8b48): 8 bits TIM image
Sector 72578 (offset 0x0a2cba78): EMD model (maybe)
Sector 72618 (offset 0x0a2e29f8): 8 bits TIM image
Sector 72651 (offset 0x0a2f5928): EMD model (maybe)
Sector 72684 (offset 0x0a308858): 8 bits TIM image
Sector 72717 (offset 0x0a31b788): EMD model (maybe)
Sector 72754 (offset 0x0a330b78): 8 bits TIM image
Sector 72787 (offset 0x0a343aa8): EMD model (maybe)
Sector 72824 (offset 0x0a358e98): 8 bits TIM image
Sector 72857 (offset 0x0a36bdc8): EMD model (maybe)
Sector 72894 (offset 0x0a3811b8): 8 bits TIM image
Sector 72927 (offset 0x0a3940e8): EMD model (maybe)
Sector 72964 (offset 0x0a3a94d8): 8 bits TIM image
Sector 72997 (offset 0x0a3bc408): EMD model (maybe)
Sector 73034 (offset 0x0a3d17f8): 8 bits TIM image
Sector 73067 (offset 0x0a3e4728): EMD model (maybe)
Sector 73104 (offset 0x0a3f9b18): 8 bits TIM image
Sector 73137 (offset 0x0a40ca48): EMD model (maybe)
Sector 74626 (offset 0x0a763a78): 8 bits TIM image
Sector 74692 (offset 0x0a7898d8): EMD model (maybe)
Sector 74757 (offset 0x0a7aee08): 8 bits TIM image
Sector 74790 (offset 0x0a7c1d38): EMD model (maybe)
Sector 74827 (offset 0x0a7d7128): 8 bits TIM image
Sector 74860 (offset 0x0a7ea058): EMD model (maybe)
Sector 75863 (offset 0x0aa29f68): 8 bits TIM image
Sector 75929 (offset 0x0aa4fdc8): EMD model (maybe)
Sector 75994 (offset 0x0aa752f8): 8 bits TIM image
Sector 76027 (offset 0x0aa88228): EMD model (maybe)
Sector 76957 (offset 0x0ac9e288): 8 bits TIM image
Sector 76990 (offset 0x0acb11b8): EMD model (maybe)
Sector 77111 (offset 0x0acf6968): 8 bits TIM image
Sector 77144 (offset 0x0ad09898): EMD model (maybe)
Sector 77263 (offset 0x0ad4dde8): 8 bits TIM image
Sector 77312 (offset 0x0ad6a018): EMD model (maybe)
Sector 77383 (offset 0x0ad92c68): 8 bits TIM image
Sector 77416 (offset 0x0ada5b98): EMD model (maybe)
Sector 78396 (offset 0x0afd8758): 8 bits TIM image
Sector 78686 (offset 0x0b07efb8): 8 bits TIM image
Sector 78735 (offset 0x0b09b1e8): EMD model (maybe)
Sector 78806 (offset 0x0b0c3e38): 8 bits TIM image
Sector 78839 (offset 0x0b0d6d68): EMD model (maybe)
Sector 79816 (offset 0x0b307d98): 8 bits TIM image
Sector 79865 (offset 0x0b323fc8): EMD model (maybe)
Sector 79986 (offset 0x0b369778): 8 bits TIM image
Sector 80035 (offset 0x0b3859a8): EMD model (maybe)
Sector 80106 (offset 0x0b3ae5f8): 8 bits TIM image
Sector 80139 (offset 0x0b3c1528): EMD model (maybe)
Sector 81585 (offset 0x0b6ffa48): 8 bits TIM image
Sector 81634 (offset 0x0b71bc78): EMD model (maybe)
Sector 81757 (offset 0x0b762688): 8 bits TIM image
Sector 81806 (offset 0x0b77e8b8): EMD model (maybe)
Sector 81830 (offset 0x0b78c538): 8 bits TIM image
Sector 81863 (offset 0x0b79f468): EMD model (maybe)
Sector 82348 (offset 0x0b8b5c58): 8 bits TIM image
Sector 82397 (offset 0x0b8d1e88): EMD model (maybe)
Sector 83035 (offset 0x0ba40428): 8 bits TIM image
Sector 83084 (offset 0x0ba5c658): EMD model (maybe)
Sector 83207 (offset 0x0baa3068): 8 bits TIM image
Sector 83256 (offset 0x0babf298): EMD model (maybe)
Sector 84411 (offset 0x0bd56628): 8 bits TIM image
Sector 84460 (offset 0x0bd72858): EMD model (maybe)
Sector 84531 (offset 0x0bd9b4a8): 8 bits TIM image
Sector 84564 (offset 0x0bdae3d8): EMD model (maybe)
Sector 84604 (offset 0x0bdc5358): 8 bits TIM image
Sector 84637 (offset 0x0bdd8288): EMD model (maybe)
Sector 86473 (offset 0x0c1f66c8): 8 bits TIM image
Sector 86539 (offset 0x0c21c528): EMD model (maybe)
Sector 86604 (offset 0x0c241a58): 8 bits TIM image
Sector 86637 (offset 0x0c254988): EMD model (maybe)
Sector 87352 (offset 0x0c3ef298): 8 bits TIM image
Sector 87401 (offset 0x0c40b4c8): EMD model (maybe)
Sector 88091 (offset 0x0c597828): 8 bits TIM image
Sector 88140 (offset 0x0c5b3a58): EMD model (maybe)
Sector 88211 (offset 0x0c5dc6a8): 8 bits TIM image
Sector 88244 (offset 0x0c5ef5d8): EMD model (maybe)
Sector 89108 (offset 0x0c7df7d8): 8 bits TIM image
Sector 89157 (offset 0x0c7fba08): EMD model (maybe)
Sector 89262 (offset 0x0c837eb8): 8 bits TIM image
Sector 89279 (offset 0x0c841ae8): EMD model (maybe)
Sector 89337 (offset 0x0c862fc8): 8 bits TIM image
Sector 89370 (offset 0x0c875ef8): EMD model (maybe)
Sector 89519 (offset 0x0c8cb7e8): 8 bits TIM image
Sector 89568 (offset 0x0c8e7a18): EMD model (maybe)
Sector 96738 (offset 0x0d8fcc78): 8 bits TIM image
Sector 96771 (offset 0x0d90fba8): EMD model (maybe)
Sector 97585 (offset 0x0dae3248): 8 bits TIM image
Sector 97651 (offset 0x0db090a8): EMD model (maybe)
Sector 97766 (offset 0x0db4b138): 8 bits TIM image
Sector 97815 (offset 0x0db67368): EMD model (maybe)
Sector 98786 (offset 0x0dd94c78): 8 bits TIM image
Sector 98835 (offset 0x0ddb0ea8): EMD model (maybe)
Sector 98958 (offset 0x0ddf78b8): 8 bits TIM image
Sector 99007 (offset 0x0de13ae8): EMD model (maybe)
Sector 99112 (offset 0x0de4ff98): 8 bits TIM image
Sector 99129 (offset 0x0de59bc8): EMD model (maybe)
Sector 99187 (offset 0x0de7b0a8): 8 bits TIM image
Sector 99220 (offset 0x0de8dfd8): EMD model (maybe)
Sector 99369 (offset 0x0dee38c8): 8 bits TIM image
Sector 99418 (offset 0x0deffaf8): EMD model (maybe)
Sector 99489 (offset 0x0df28748): 8 bits TIM image
Sector 99522 (offset 0x0df3b678): EMD model (maybe)
Sector 100327 (offset 0x0e109a68): 8 bits TIM image
Sector 100360 (offset 0x0e11c998): EMD model (maybe)
Sector 100435 (offset 0x0e147aa8): 8 bits TIM image
Sector 100452 (offset 0x0e1516d8): EMD model (maybe)
Sector 100514 (offset 0x0e175078): 8 bits TIM image
Sector 100563 (offset 0x0e1912a8): EMD model (maybe)
Sector 101434 (offset 0x0e3854f8): 8 bits TIM image
Sector 101483 (offset 0x0e3a1728): EMD model (maybe)
Sector 102228 (offset 0x0e54d3d8): 8 bits TIM image
Sector 102277 (offset 0x0e569608): EMD model (maybe)
Sector 102372 (offset 0x0e59fed8): 8 bits TIM image
Sector 102405 (offset 0x0e5b2e08): EMD model (maybe)
Sector 103818 (offset 0x0e8de3f8): 8 bits TIM image
Sector 103851 (offset 0x0e8f1328): EMD model (maybe)
Sector 103972 (offset 0x0e936ad8): 8 bits TIM image
Sector 104005 (offset 0x0e949a08): EMD model (maybe)
Sector 104080 (offset 0x0e974b18): 8 bits TIM image
Sector 104097 (offset 0x0e97e748): EMD model (maybe)
Sector 104148 (offset 0x0e99bbd8): 8 bits TIM image
Sector 104197 (offset 0x0e9b7e08): EMD model (maybe)
Sector 104320 (offset 0x0e9fe818): 8 bits TIM image
Sector 104369 (offset 0x0ea1aa48): EMD model (maybe)
Sector 104464 (offset 0x0ea51318): 8 bits TIM image
Sector 104481 (offset 0x0ea5af48): EMD model (maybe)
Sector 104498 (offset 0x0ea64b78): 8 bits TIM image
Sector 104531 (offset 0x0ea77aa8): EMD model (maybe)
Sector 105214 (offset 0x0ebffdb8): 8 bits TIM image
Sector 105247 (offset 0x0ec12ce8): EMD model (maybe)
Sector 105275 (offset 0x0ec22e28): 8 bits TIM image
Sector 105292 (offset 0x0ec2ca58): EMD model (maybe)
Sector 105345 (offset 0x0ec4b148): 8 bits TIM image
Sector 105394 (offset 0x0ec67378): EMD model (maybe)
Sector 105489 (offset 0x0ec9dc48): 8 bits TIM image
Sector 105506 (offset 0x0eca7878): EMD model (maybe)
Sector 106004 (offset 0x0edc57d8): 8 bits TIM image
Sector 106053 (offset 0x0ede1a08): EMD model (maybe)
Sector 106148 (offset 0x0ee182d8): 8 bits TIM image
Sector 106165 (offset 0x0ee21f08): EMD model (maybe)
Sector 106931 (offset 0x0efd9ca8): 8 bits TIM image
Sector 106964 (offset 0x0efecbd8): EMD model (maybe)
Sector 106992 (offset 0x0effcd18): 8 bits TIM image
Sector 107009 (offset 0x0f006948): EMD model (maybe)
Sector 107060 (offset 0x0f023dd8): 8 bits TIM image
Sector 107109 (offset 0x0f040008): EMD model (maybe)
Sector 107746 (offset 0x0f1adc78): 8 bits TIM image
Sector 107795 (offset 0x0f1c9ea8): EMD model (maybe)
Sector 107868 (offset 0x0f1f3d58): 8 bits TIM image
Sector 107885 (offset 0x0f1fd988): EMD model (maybe)
Sector 109852 (offset 0x0f667158): 8 bits TIM image
Sector 109934 (offset 0x0f6962b8): EMD model (maybe)
Sector 110017 (offset 0x0f6c5d48): 8 bits TIM image
Sector 110034 (offset 0x0f6cf978): EMD model (maybe)
Sector 110041 (offset 0x0f6d39c8): 8 bits TIM image
Sector 110074 (offset 0x0f6e68f8): EMD model (maybe)
Sector 111302 (offset 0x0f9a7b38): 8 bits TIM image
Sector 111351 (offset 0x0f9c3d68): EMD model (maybe)
Sector 111446 (offset 0x0f9fa638): 8 bits TIM image
Sector 111463 (offset 0x0fa04268): EMD model (maybe)
Sector 112563 (offset 0x0fc7bca8): 8 bits TIM image
Sector 112612 (offset 0x0fc97ed8): EMD model (maybe)
Sector 112707 (offset 0x0fcce7a8): 8 bits TIM image
Sector 112724 (offset 0x0fcd83d8): EMD model (maybe)
Sector 113521 (offset 0x0fea1e48): 8 bits TIM image
Sector 113554 (offset 0x0feb4d78): EMD model (maybe)
Sector 113591 (offset 0x0feca168): 8 bits TIM image
Sector 113624 (offset 0x0fedd098): EMD model (maybe)
Sector 113661 (offset 0x0fef2488): 8 bits TIM image
Sector 113694 (offset 0x0ff053b8): EMD model (maybe)
Sector 113731 (offset 0x0ff1a7a8): 8 bits TIM image
Sector 113764 (offset 0x0ff2d6d8): EMD model (maybe)
Sector 113801 (offset 0x0ff42ac8): 8 bits TIM image
Sector 113834 (offset 0x0ff559f8): EMD model (maybe)
Sector 113871 (offset 0x0ff6ade8): 8 bits TIM image
Sector 113904 (offset 0x0ff7dd18): EMD model (maybe)
Sector 114511 (offset 0x100da5e8): 8 bits TIM image
Sector 114560 (offset 0x100f6818): EMD model (maybe)
Sector 114655 (offset 0x1012d0e8): 8 bits TIM image
Sector 114688 (offset 0x10140018): EMD model (maybe)
Sector 115444 (offset 0x102f21d8): 8 bits TIM image
Sector 115493 (offset 0x1030e408): EMD model (maybe)
Sector 115588 (offset 0x10344cd8): 8 bits TIM image
Sector 115605 (offset 0x1034e908): EMD model (maybe)
Sector 115622 (offset 0x10358538): 8 bits TIM image
Sector 115655 (offset 0x1036b468): EMD model (maybe)
Sector 115692 (offset 0x10380858): 8 bits TIM image
Sector 115725 (offset 0x10393788): EMD model (maybe)
Sector 115762 (offset 0x103a8b78): 8 bits TIM image
Sector 115795 (offset 0x103bbaa8): EMD model (maybe)
Sector 115832 (offset 0x103d0e98): 8 bits TIM image
Sector 115865 (offset 0x103e3dc8): EMD model (maybe)
Sector 115902 (offset 0x103f91b8): 8 bits TIM image
Sector 115935 (offset 0x1040c0e8): EMD model (maybe)
Sector 115972 (offset 0x104214d8): 8 bits TIM image
Sector 116005 (offset 0x10434408): EMD model (maybe)
Sector 116593 (offset 0x10585e48): 8 bits TIM image
Sector 116626 (offset 0x10598d78): EMD model (maybe)
Sector 116747 (offset 0x105de528): 8 bits TIM image
Sector 116780 (offset 0x105f1458): EMD model (maybe)
Sector 116855 (offset 0x1061c568): 8 bits TIM image
Sector 116872 (offset 0x10626198): EMD model (maybe)
Sector 116925 (offset 0x10644888): 8 bits TIM image
Sector 116974 (offset 0x10660ab8): EMD model (maybe)
Sector 117069 (offset 0x10697388): 8 bits TIM image
Sector 117086 (offset 0x106a0fb8): EMD model (maybe)
Sector 117155 (offset 0x106c89a8): 8 bits TIM image
Sector 117204 (offset 0x106e4bd8): EMD model (maybe)
Sector 117228 (offset 0x106f2858): 8 bits TIM image
Sector 117261 (offset 0x10705788): EMD model (maybe)
Sector 117298 (offset 0x1071ab78): 8 bits TIM image
Sector 117331 (offset 0x1072daa8): EMD model (maybe)
Sector 117368 (offset 0x10742e98): 8 bits TIM image
Sector 117401 (offset 0x10755dc8): EMD model (maybe)
Sector 117438 (offset 0x1076b1b8): 8 bits TIM image
Sector 117471 (offset 0x1077e0e8): EMD model (maybe)
Sector 117508 (offset 0x107934d8): 8 bits TIM image
Sector 117541 (offset 0x107a6408): EMD model (maybe)
Sector 117578 (offset 0x107bb7f8): 8 bits TIM image
Sector 117611 (offset 0x107ce728): EMD model (maybe)
Sector 118299 (offset 0x10959828): 8 bits TIM image
Sector 118348 (offset 0x10975a58): EMD model (maybe)
Sector 118473 (offset 0x109bd6c8): 8 bits TIM image
Sector 118506 (offset 0x109d05f8): EMD model (maybe)
Sector 118580 (offset 0x109fadd8): 8 bits TIM image
Sector 118613 (offset 0x10a0dd08): EMD model (maybe)
Sector 118764 (offset 0x10a64858): 8 bits TIM image
Sector 118813 (offset 0x10a80a88): EMD model (maybe)
Sector 118908 (offset 0x10ab7358): 8 bits TIM image
Sector 118925 (offset 0x10ac0f88): EMD model (maybe)
Sector 118994 (offset 0x10ae8978): 8 bits TIM image
Sector 119043 (offset 0x10b04ba8): EMD model (maybe)
Sector 119067 (offset 0x10b12828): 8 bits TIM image
Sector 119100 (offset 0x10b25758): EMD model (maybe)
Sector 119137 (offset 0x10b3ab48): 8 bits TIM image
Sector 119170 (offset 0x10b4da78): EMD model (maybe)
Sector 119207 (offset 0x10b62e68): 8 bits TIM image
Sector 119240 (offset 0x10b75d98): EMD model (maybe)
Sector 119277 (offset 0x10b8b188): 8 bits TIM image
Sector 119310 (offset 0x10b9e0b8): EMD model (maybe)
Sector 119347 (offset 0x10bb34a8): 8 bits TIM image
Sector 119380 (offset 0x10bc63d8): EMD model (maybe)
Sector 119417 (offset 0x10bdb7c8): 8 bits TIM image
Sector 119450 (offset 0x10bee6f8): EMD model (maybe)
Sector 120023 (offset 0x10d37768): 8 bits TIM image
Sector 120072 (offset 0x10d53998): EMD model (maybe)
Sector 120165 (offset 0x10d89008): 8 bits TIM image
Sector 120182 (offset 0x10d92c38): EMD model (maybe)
Sector 120257 (offset 0x10dbdd48): 8 bits TIM image
Sector 120306 (offset 0x10dd9f78): EMD model (maybe)
Sector 121131 (offset 0x10fb3b28): 8 bits TIM image
Sector 121180 (offset 0x10fcfd58): EMD model (maybe)
Sector 121275 (offset 0x11006628): 8 bits TIM image
Sector 121292 (offset 0x11010258): EMD model (maybe)
Sector 121309 (offset 0x11019e88): 8 bits TIM image
Sector 121342 (offset 0x1102cdb8): EMD model (maybe)
Sector 121379 (offset 0x110421a8): 8 bits TIM image
Sector 121412 (offset 0x110550d8): EMD model (maybe)
Sector 121449 (offset 0x1106a4c8): 8 bits TIM image
Sector 121482 (offset 0x1107d3f8): EMD model (maybe)
Sector 121519 (offset 0x110927e8): 8 bits TIM image
Sector 121552 (offset 0x110a5718): EMD model (maybe)
Sector 121589 (offset 0x110bab08): 8 bits TIM image
Sector 121622 (offset 0x110cda38): EMD model (maybe)
Sector 121659 (offset 0x110e2e28): 8 bits TIM image
Sector 121692 (offset 0x110f5d58): EMD model (maybe)
Sector 122870 (offset 0x1139a438): 8 bits TIM image
Sector 122936 (offset 0x113c0298): EMD model (maybe)
Sector 123052 (offset 0x11402c58): 8 bits TIM image
Sector 123101 (offset 0x1141ee88): EMD model (maybe)
Sector 123196 (offset 0x11455758): 8 bits TIM image
Sector 123213 (offset 0x1145f388): EMD model (maybe)
Sector 123230 (offset 0x11468fb8): 8 bits TIM image
Sector 123263 (offset 0x1147bee8): EMD model (maybe)
Sector 123300 (offset 0x114912d8): 8 bits TIM image
Sector 123333 (offset 0x114a4208): EMD model (maybe)
Sector 123370 (offset 0x114b95f8): 8 bits TIM image
Sector 123403 (offset 0x114cc528): EMD model (maybe)
Sector 123440 (offset 0x114e1918): 8 bits TIM image
Sector 123473 (offset 0x114f4848): EMD model (maybe)
Sector 123510 (offset 0x11509c38): 8 bits TIM image
Sector 123543 (offset 0x1151cb68): EMD model (maybe)
Sector 123580 (offset 0x11531f58): 8 bits TIM image
Sector 123613 (offset 0x11544e88): EMD model (maybe)
Sector 128637 (offset 0x12089c88): 8 bits TIM image
Sector 128735 (offset 0x120c20e8): EMD model (maybe)
Sector 129346 (offset 0x12220e78): 8 bits TIM image
Sector 129379 (offset 0x12233da8): EMD model (maybe)
Sector 129814 (offset 0x1232da38): 8 bits TIM image
Sector 129863 (offset 0x12349c68): EMD model (maybe)
Sector 129972 (offset 0x123885d8): 8 bits TIM image
Sector 130005 (offset 0x1239b508): EMD model (maybe)
Sector 130978 (offset 0x125ca078): 8 bits TIM image
Sector 131076 (offset 0x126024d8): EMD model (maybe)
Sector 131182 (offset 0x1263f2b8): 8 bits TIM image
Sector 131215 (offset 0x126521e8): EMD model (maybe)
Sector 131960 (offset 0x127fde98): 8 bits TIM image
Sector 132058 (offset 0x128362f8): EMD model (maybe)
Sector 132164 (offset 0x128730d8): 8 bits TIM image
Sector 132197 (offset 0x12886008): EMD model (maybe)
Sector 133188 (offset 0x12abf0d8): 8 bits TIM image
Sector 133286 (offset 0x12af7538): EMD model (maybe)
Sector 133372 (offset 0x12b28b58): 8 bits TIM image
Sector 133389 (offset 0x12b32788): EMD model (maybe)
Sector 133759 (offset 0x12c06ee8): 8 bits TIM image
Sector 133792 (offset 0x12c19e18): EMD model (maybe)
Sector 133831 (offset 0x12c30468): 8 bits TIM image
Sector 133864 (offset 0x12c43398): EMD model (maybe)
Sector 133901 (offset 0x12c58788): 8 bits TIM image
Sector 133934 (offset 0x12c6b6b8): EMD model (maybe)
Sector 134741 (offset 0x12e3ad08): 8 bits TIM image
Sector 134839 (offset 0x12e73168): EMD model (maybe)
Sector 134945 (offset 0x12eaff48): 8 bits TIM image
Sector 134978 (offset 0x12ec2e78): EMD model (maybe)
Sector 135539 (offset 0x130050a8): 8 bits TIM image
Sector 135572 (offset 0x13017fd8): EMD model (maybe)
Sector 135611 (offset 0x1302e628): 8 bits TIM image
Sector 135644 (offset 0x13041558): EMD model (maybe)
Sector 136504 (offset 0x1322f298): 8 bits TIM image
Sector 136537 (offset 0x132421c8): EMD model (maybe)
Sector 137679 (offset 0x134d1de8): 8 bits TIM image
Sector 137777 (offset 0x1350a248): EMD model (maybe)
Sector 137883 (offset 0x13547028): 8 bits TIM image
Sector 137916 (offset 0x13559f58): EMD model (maybe)
Sector 137973 (offset 0x1357ab08): 8 bits TIM image
Sector 137990 (offset 0x13584738): EMD model (maybe)
Sector 139498 (offset 0x138e65f8): 8 bits TIM image
Sector 139531 (offset 0x138f9528): EMD model (maybe)
Sector 139638 (offset 0x13936c38): 8 bits TIM image
Sector 139671 (offset 0x13949b68): EMD model (maybe)
Sector 140287 (offset 0x13aab6e8): 8 bits TIM image
Sector 140320 (offset 0x13abe618): EMD model (maybe)
Sector 140411 (offset 0x13af2a28): 8 bits TIM image
Sector 140444 (offset 0x13b05958): EMD model (maybe)
Sector 141056 (offset 0x13c65018): 8 bits TIM image
Sector 141089 (offset 0x13c77f48): EMD model (maybe)
Sector 141129 (offset 0x13c8eec8): 8 bits TIM image
Sector 141146 (offset 0x13c98af8): EMD model (maybe)
Sector 141627 (offset 0x13dace28): 8 bits TIM image
Sector 141725 (offset 0x13de5288): EMD model (maybe)
Sector 142320 (offset 0x13f3ad18): 8 bits TIM image
Sector 142353 (offset 0x13f4dc48): EMD model (maybe)
Sector 143159 (offset 0x1411c968): 8 bits TIM image
Sector 143208 (offset 0x14138b98): EMD model (maybe)
Sector 143801 (offset 0x1428d3c8): 8 bits TIM image
Sector 143867 (offset 0x142b3228): EMD model (maybe)
Sector 144772 (offset 0x144bacd8): 8 bits TIM image
Sector 144870 (offset 0x144f3138): EMD model (maybe)
Sector 144976 (offset 0x1452ff18): 8 bits TIM image
Sector 145009 (offset 0x14542e48): EMD model (maybe)
Sector 149490 (offset 0x14f4ff78): 8 bits TIM image
Sector 149556 (offset 0x14f75dd8): EMD model (maybe)
Sector 149670 (offset 0x14fb7538): 8 bits TIM image
Sector 149719 (offset 0x14fd3768): EMD model (maybe)
Sector 149824 (offset 0x1500fc18): 8 bits TIM image
Sector 149841 (offset 0x15019848): EMD model (maybe)
Sector 149899 (offset 0x1503ad28): 8 bits TIM image
Sector 149932 (offset 0x1504dc58): EMD model (maybe)
Sector 150031 (offset 0x150869e8): 8 bits TIM image
Sector 150064 (offset 0x15099918): EMD model (maybe)
Sector 150904 (offset 0x1527be98): 8 bits TIM image
Sector 150937 (offset 0x1528edc8): EMD model (maybe)
Sector 152168 (offset 0x15551b98): 8 bits TIM image
Sector 152234 (offset 0x155779f8): EMD model (maybe)
Sector 152348 (offset 0x155b9158): 8 bits TIM image
Sector 152446 (offset 0x155f15b8): EMD model (maybe)
Sector 153253 (offset 0x157c0c08): 8 bits TIM image
Sector 153302 (offset 0x157dce38): EMD model (maybe)
Sector 153411 (offset 0x1581b7a8): 8 bits TIM image
Sector 153444 (offset 0x1582e6d8): EMD model (maybe)
Sector 153501 (offset 0x1584f288): 8 bits TIM image
Sector 153518 (offset 0x15858eb8): EMD model (maybe)
Sector 153541 (offset 0x15866208): 8 bits TIM image
Sector 153574 (offset 0x15879138): EMD model (maybe)
Sector 155652 (offset 0x15d224d8): 8 bits TIM image
Sector 155718 (offset 0x15d48338): EMD model (maybe)
Sector 155818 (offset 0x15d819f8): 8 bits TIM image
Sector 155851 (offset 0x15d94928): EMD model (maybe)
Sector 155942 (offset 0x15dc8d38): 8 bits TIM image
Sector 155991 (offset 0x15de4f68): EMD model (maybe)
Sector 156140 (offset 0x15e3a858): 8 bits TIM image
Sector 156173 (offset 0x15e4d788): EMD model (maybe)
Sector 156771 (offset 0x15fa4da8): 8 bits TIM image
Sector 156804 (offset 0x15fb7cd8): EMD model (maybe)
Sector 156841 (offset 0x15fcd0c8): 8 bits TIM image
Sector 156874 (offset 0x15fdfff8): EMD model (maybe)
Sector 157642 (offset 0x16198ff8): 8 bits TIM image
Sector 157691 (offset 0x161b5228): EMD model (maybe)
Sector 157786 (offset 0x161ebaf8): 8 bits TIM image
Sector 157803 (offset 0x161f5728): EMD model (maybe)
Sector 158880 (offset 0x1645fe18): 8 bits TIM image
Sector 158913 (offset 0x16472d48): EMD model (maybe)
Sector 158922 (offset 0x16477ff8): 8 bits TIM image
Sector 158955 (offset 0x1648af28): EMD model (maybe)
Sector 160063 (offset 0x167072e8): 8 bits TIM image
Sector 160129 (offset 0x1672d148): EMD model (maybe)
Sector 160213 (offset 0x1675d508): 8 bits TIM image
Sector 160230 (offset 0x16767138): EMD model (maybe)
Sector 160684 (offset 0x1686bc58): 8 bits TIM image
Sector 160701 (offset 0x16875888): EMD model (maybe)
Sector 161247 (offset 0x169af0e8): 8 bits TIM image
Sector 161313 (offset 0x169d4f48): EMD model (maybe)
Sector 161448 (offset 0x16a22798): 8 bits TIM image
Sector 161465 (offset 0x16a2c3c8): EMD model (maybe)
Sector 162713 (offset 0x16cf8dc8): 8 bits TIM image
Sector 162746 (offset 0x16d0bcf8): EMD model (maybe)
Sector 162751 (offset 0x16d0eae8): 8 bits TIM image
Sector 162784 (offset 0x16d21a18): EMD model (maybe)
Sector 163692 (offset 0x16f2b058): 8 bits TIM image
Sector 163741 (offset 0x16f47288): EMD model (maybe)
Sector 163836 (offset 0x16f7db58): 8 bits TIM image
Sector 163853 (offset 0x16f87788): EMD model (maybe)
Sector 170554 (offset 0x17e8f4f8): 8 bits TIM image
Sector 170587 (offset 0x17ea2428): EMD model (maybe)
Sector 170622 (offset 0x17eb65b8): 8 bits TIM image
Sector 170655 (offset 0x17ec94e8): EMD model (maybe)
Sector 170934 (offset 0x17f69838): 8 bits TIM image
Sector 171105 (offset 0x17fcbb48): 8 bits TIM image
Sector 171171 (offset 0x17ff19a8): EMD model (maybe)
Sector 172404 (offset 0x182b59d8): 8 bits TIM image
Sector 172486 (offset 0x182e4b38): EMD model (maybe)
Sector 172554 (offset 0x1830bbf8): 8 bits TIM image
Sector 172587 (offset 0x1831eb28): EMD model (maybe)
Sector 172627 (offset 0x18335aa8): 8 bits TIM image
Sector 172660 (offset 0x183489d8): EMD model (maybe)
Sector 173216 (offset 0x18487e18): 8 bits TIM image
Sector 173233 (offset 0x18491a48): EMD model (maybe)
Sector 173291 (offset 0x184b2f28): 8 bits TIM image
Sector 173324 (offset 0x184c5e58): EMD model (maybe)
Sector 173776 (offset 0x185c9718): 8 bits TIM image
Sector 173793 (offset 0x185d3348): EMD model (maybe)
Sector 174247 (offset 0x186d7e68): 8 bits TIM image
Sector 174345 (offset 0x187102c8): EMD model (maybe)
Sector 175383 (offset 0x18964368): 8 bits TIM image
Sector 175416 (offset 0x18977298): EMD model (maybe)
Sector 175467 (offset 0x18994728): 8 bits TIM image
Sector 175500 (offset 0x189a7658): EMD model (maybe)
Sector 175540 (offset 0x189be5d8): 8 bits TIM image
Sector 175573 (offset 0x189d1508): EMD model (maybe)
Sector 176036 (offset 0x18adb2d8): 8 bits TIM image
Sector 176053 (offset 0x18ae4f08): EMD model (maybe)
Sector 176462 (offset 0x18bcfcb8): 8 bits TIM image
Sector 176495 (offset 0x18be2be8): EMD model (maybe)
Sector 177321 (offset 0x18dbd0c8): 8 bits TIM image
Sector 177419 (offset 0x18df5528): EMD model (maybe)
Sector 178280 (offset 0x18fe3b98): 8 bits TIM image
Sector 178329 (offset 0x18fffdc8): EMD model (maybe)
Sector 179190 (offset 0x191ee438): 8 bits TIM image
Sector 179223 (offset 0x19201368): EMD model (maybe)
Sector 180058 (offset 0x193e0af8): 8 bits TIM image
Sector 180091 (offset 0x193f3a28): EMD model (maybe)
Sector 180711 (offset 0x19557a68): 8 bits TIM image
Sector 180793 (offset 0x19586bc8): EMD model (maybe)
Sector 181832 (offset 0x197db598): 8 bits TIM image
Sector 181849 (offset 0x197e51c8): EMD model (maybe)
Sector 184551 (offset 0x19df4a68): 8 bits TIM image
Sector 184600 (offset 0x19e10c98): EMD model (maybe)
Sector 184695 (offset 0x19e47568): 8 bits TIM image
Sector 184712 (offset 0x19e51198): EMD model (maybe)
Sector 185407 (offset 0x19fe02e8): 8 bits TIM image
Sector 185473 (offset 0x1a006148): EMD model (maybe)
Sector 186651 (offset 0x1a2aa828): 8 bits TIM image
Sector 186684 (offset 0x1a2bd758): EMD model (maybe)
Sector 187164 (offset 0x1a3d1158): 8 bits TIM image
Sector 187213 (offset 0x1a3ed388): EMD model (maybe)
Sector 187915 (offset 0x1a580528): 8 bits TIM image
Sector 187948 (offset 0x1a593458): EMD model (maybe)
Sector 187976 (offset 0x1a5a3598): 8 bits TIM image
Sector 187993 (offset 0x1a5ad1c8): EMD model (maybe)
Sector 188698 (offset 0x1a741ef8): 8 bits TIM image
Sector 188796 (offset 0x1a77a358): EMD model (maybe)
Sector 189611 (offset 0x1a94e328): 8 bits TIM image
Sector 189644 (offset 0x1a961258): EMD model (maybe)
Sector 190449 (offset 0x1ab2f648): 8 bits TIM image
Sector 190531 (offset 0x1ab5e7a8): EMD model (maybe)
Sector 191743 (offset 0x1ae166e8): 8 bits TIM image
Sector 191825 (offset 0x1ae45848): EMD model (maybe)
Sector 192638 (offset 0x1b0185b8): 8 bits TIM image
Sector 192655 (offset 0x1b0221e8): EMD model (maybe)
Sector 193090 (offset 0x1b11be78): 8 bits TIM image
Sector 193172 (offset 0x1b14afd8): EMD model (maybe)
Sector 194359 (offset 0x1b3f4968): 8 bits TIM image
Sector 194457 (offset 0x1b42cdc8): EMD model (maybe)
Sector 196531 (offset 0x1b8d3ca8): 8 bits TIM image
Sector 196580 (offset 0x1b8efed8): EMD model (maybe)
Sector 196652 (offset 0x1b919458): 8 bits TIM image
Sector 196685 (offset 0x1b92c388): EMD model (maybe)
Sector 197477 (offset 0x1baf3008): 8 bits TIM image
Sector 197510 (offset 0x1bb05f38): EMD model (maybe)
Sector 198132 (offset 0x1bc6b1d8): 8 bits TIM image
Sector 198181 (offset 0x1bc87408): EMD model (maybe)
Sector 198256 (offset 0x1bcb2518): 8 bits TIM image
Sector 198289 (offset 0x1bcc5448): EMD model (maybe)
Sector 199392 (offset 0x1bf3ea18): 8 bits TIM image
Sector 199425 (offset 0x1bf51948): EMD model (maybe)
Sector 200097 (offset 0x1c0d3748): 8 bits TIM image
Sector 200130 (offset 0x1c0e6678): EMD model (maybe)
Sector 201501 (offset 0x1c3f9a88): 8 bits TIM image
Sector 201583 (offset 0x1c428be8): EMD model (maybe)
Sector 201646 (offset 0x1c44ceb8): 8 bits TIM image
Sector 201679 (offset 0x1c45fde8): EMD model (maybe)
Sector 202193 (offset 0x1c587048): 8 bits TIM image
Sector 202242 (offset 0x1c5a3278): EMD model (maybe)
Sector 202347 (offset 0x1c5df728): 8 bits TIM image
Sector 202364 (offset 0x1c5e9358): EMD model (maybe)
Sector 202753 (offset 0x1c6c8948): 8 bits TIM image
Sector 202819 (offset 0x1c6ee7a8): EMD model (maybe)
Sector 202881 (offset 0x1c712148): 8 bits TIM image
Sector 202930 (offset 0x1c72e378): EMD model (maybe)
Sector 203889 (offset 0x1c954e48): 8 bits TIM image
Sector 203955 (offset 0x1c97aca8): EMD model (maybe)
Sector 204069 (offset 0x1c9bc408): 8 bits TIM image
Sector 204118 (offset 0x1c9d8638): EMD model (maybe)
Sector 204670 (offset 0x1cb155b8): 8 bits TIM image
Sector 204719 (offset 0x1cb317e8): EMD model (maybe)
Sector 204824 (offset 0x1cb6dc98): 8 bits TIM image
Sector 204841 (offset 0x1cb778c8): EMD model (maybe)
Sector 205288 (offset 0x1cc78398): 8 bits TIM image
Sector 205337 (offset 0x1cc945c8): EMD model (maybe)
};

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
