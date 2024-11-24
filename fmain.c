/* fmain.c - created Aug 86 by Talin - The Faerie Tale Adventure */

#include "ftale.h"

/****** this section defines the variables used to communicate with the 
		graphics routines */

#define PAGE_DEPTH 5
#define TEXT_DEPTH 4

#define SCREEN_WIDTH 288
#define PHANTA_WIDTH 320 /* two words added for horizontal scrolling */

#define PAGE_HEIGHT	143
#define RAST_HEIGHT	200
#define TEXT_HEIGHT	57

struct View v, *oldview;
struct ViewPort vp_page, vp_text, vp_title, *vp;

/* add name of setfig for generic messages?? */
struct {
	BYTE	cfile_entry,image_base,can_talk;
} setfig_table[] = {
	13,0,1,			/* "wizard" = 0 */
	13,4,1,			/* "priest" = 1 */
	14,0,0,			/* "guard" = 2 */
	14,1,0,			/* "guard" = 3 (back) */
	14,2,0,			/* "princess" = 4 */
	14,4,1,			/* "king" = 5 */
	14,6,0,			/* "noble" = 6 */
	14,7,0,			/* "sorceress" = 7 */
	15,0,0,			/* "bartender" = 8 */
	16,0,0,			/* "witch" = 9 */
	16,6,0,			/* "spectre" = 10 */
	16,7,0,			/* "ghost" = 11 */
	17,0,1,			/* "ranger" = 12 */
	17,4,1			/* "begger" = 13 */
};

struct seq_info seq_list[7];

/* defines the variables for currently defined actors, on screen or off */

struct encounter {
	char	hitpoints,
			agressive,
			arms,
			cleverness,
			treasure,
			file_id;
} encounter_chart[] = {
	{ 18, TRUE,2,0,2, 6 },	/* 0 - Ogre */
	{ 12, TRUE,4,1,1, 6 },	/* 1 - Orcs */
	{ 16, TRUE,6,1,4, 7 },	/* 2 - Wraith */
	{ 8,  TRUE,3,0,3, 7 },	/* 3 - Skeleton */
	{ 16, TRUE,6,1,0, 8 },	/* 4 - Snake - swamp region */
	{ 9,  TRUE,3,0,0, 7 },	/* 5 - Salamander - lava region */
	{ 10, TRUE,6,1,0, 8 },	/* 6 - Spider - spider pits */
	{ 40, TRUE,7,1,0, 8 },	/* 7 - DKnight - elf glade */
	{ 12, TRUE,6,1,0, 9 },	/* 8 - Loraii - astral plane */
	{ 50, TRUE,5,0,0, 9 },	/* 9 - Necromancer - final arena */
	{ 4,  NULL,0,0,0, 9 },	/* 10 - Woodcutter */
};

extern char treasure_probs[], weapon_probs[];

#define MAXSHAPES	25

struct shape anim_list[20];	/* 7 people + 7 objects */

/* weapon: 0=none, 1=dagger, 2=mace, 3=sword, 4=bow, 5=wand */

unsigned char anim_index[20];		/* for sorting */
short anix, anix2;			/* allocation index - how many monsters + 1 */
short mdex;

struct missile {
	unsigned short	abs_x, abs_y;
	char	missile_type,	/* NULL, arrow, rock, 'thing', or fireball */
			time_of_flight, /* in frames? */
			speed,			/* 0 = still unshot */
			direction,
			archer;			/* ID of archer */
} missile_list[6]; /* six missiles max */

char	fiery_death;

/* motion states */
#define	WALKING	12
#define STILL	13
#define FIGHTING 0
#define DYING 	14
#define DEAD  	15
#define SINK  	16
#define OSCIL	17	/* and 18 */
#define TALKING	19
#define FROZEN	20
#define FLYING	21
#define FALL	22
#define SLEEP	23
#define SHOOT1	24	/* bow up - aiming */
#define SHOOT3	25	/* bow fired, arrow given velocity */

/* goal modes */

#define	USER		0	/* character is user-controlled */
#define	ATTACK1		1	/* attack character (stupidly) */
#define	ATTACK2		2	/* attack character (cleverly) */
#define ARCHER1		3	/* archery attack style 1 */
#define ARCHER2		4	/* archery attack style 2 */
#define FLEE		5	/* run directly away */
#define	STAND		6	/* don't move but face character */
#define	DEATH		7	/* a dead character */
#define WAIT		8	/* wait to speak to character */
#define FOLLOWER	9	/* follow another character */
#define CONFUSED	10	/* run around randomly */

/* tactical modes (sub-goals) */
/* choices 2-5 can be selected randomly for getting around obstacles */

#define FRUST		0 	/* all tactics frustrated - try sonething else */
#define	PURSUE		1	/* go in the direction of the character */
#define	FOLLOW		2	/* go toward another character */
#define	BUMBLE_SEEK	3	/* bumble around looking for him */
#define	RANDOM		4	/* move randomly */
#define	BACKUP		5	/* opposite direction we were going */
#define	EVADE		6	/* move 90 degrees from character */
#define HIDE		7	/* seek a hiding place */
#define SHOOT		8	/* shoot an arrow */
#define SHOOTFRUST	9	/* arrows not getting through */
#define EGG_SEEK	10	/* snakes going for the eggs */

extern char	turtle_eggs;
extern UBYTE fallstates[];

struct transition
{	char	newstate[4];		/* transition table */
} trans_list[9] = {
	{  1, 8, 0, 1 },			/* 0 - arm down, weapon low */
	{  2, 0, 1, 0 },			/* 1 - arm down, weapon diagonal down */
	{  3, 1, 2, 8 },			/* 2 - arm swing1, weapon horizontal */
	{  4, 2, 3, 7 },			/* 3 - arm swing2, weapon raised */
	{  5, 3, 4, 6 },			/* 4 - arm swing2, weapon diag up */
	{  6, 4, 5, 5 },			/* 5 - arm swing2, weapon high */
	{  8, 5, 6, 4 },			/* 6 - arm high, weapon up */
	{  8, 6, 7, 3 },			/* 7 - arm high, weapon horizontal */
	{  0, 6, 8, 2 }};			/* 8 - arm middle, weapon raise fwd */

struct state {
	char	figure,			/* figure # to use */
			wpn_no,			/* weapon index to use */
			wpn_x, wpn_y;	/* weapon x,y coord */
} statelist[87] = {

	/* 0 = southwalk sequence */
	{  0,11,-2,11 },{  1,11,-3,11 },{  2,11,-3,10 },{  3,11,-3, 9 },
	{  4,11,-3,10 },{  5,11,-3,11 },{  6,11,-2,11 },{  7,11,-1,11 },

	/* 8 = westwalk sequence */
	{  8,9,-12,11 },{  9,9,-11,12 },{ 10,9,-8,13 },{ 11,9,-4,13 },
	{ 12,9,0,13 },{ 13,9,-4,13 },{ 14,9,-8,13 },{ 15,9,-11,12 },

	/* 16 = northwalk sequence */
    { 16,14,-1,1 },{ 17,14,-1,2 },{ 18,14,-1,3 },{ 19,14,-1,4 },
	{ 20,14,-1,3 },{ 21,14,-1,2 },{ 22,14,-1,1 },{ 23,14,-1,1 },

	/* 24 = eastwalk sequence */
	{ 24,10,5,12 },{ 25,10,3,12 },{ 26,10,2,12 },{ 27,10,3,12 },
	{ 28,10,5,12 },{ 29,10,6,12 },{ 30,10,6,11 },{ 31,10,6,12 },

	/* 32 = south fight = 32 + transition state */
	{ 32,11,-2,12 },{ 32,10,0,12 },{ 33,0,2,10 },
	{ 34,1,4,6 },{ 34,2,1,4 },{ 34,3,0,4 },
	{ 36,4,-5,0 },{ 36,5,-10,1 },{ 35,12,-5,5 },
	{ 36,0,0,+6 },{ 38,85,-6,5 },{ 37,81,-6,5 },

	/* 44 = west fight  = 44 + transition state */
	{ 40,9,-7,12 },{ 40,8,-9,9 },{ 41,7,-10,5 },
	{ 42,7,-12,4 },{ 42,6,-12,3 },{ 42,5,-12,3 },
	{ 44,5,-8,3 },{ 44,14,-7,6 },{ 43,13,-7,8 },
	{ 42,5,-12,3 },{ 46,86,-3,0 },{ 45,82,-3,0 },

	/* 56 = north fight  = 56 + transition state */
	{ 48,14,-3,0 },{ 48,6,-3,-1 },{ 49,5,-2,-3 },
	{ 50,5,-3,-4 },{ 50,4,0,0 },{ 50,3,3,0 },
	{ 52,4,6,1 },{ 52,15,7,3 },{ 51,14,1,6 },
	{ 50,4,0,0 },{ 54,87,3,0 },{ 53,83,3,0 },

	/* 68 = east fight  = 68 + transition state */
	{ 56,10,5,11 },{ 56,0,6,9 },{ 57,1,10,6 },
	{ 58,1,10,5 },{ 58,2,7,3 },{ 58,3,6,3 },
	{ 60,4,1,0 },{ 60,3,3,2 },{ 59,15,4,1 },
	{ 58,4,5,1 },{ 62,84,3,0 },{ 61,80,3,0 },

	/* 80 = death sequence 1 */
	{ 47,0,5,11 },{ 63,0,6,9 },{ 39,0,6,9 },
	/* 83 = sinking sequence 1 (with bubbles??) */
	{ 55,10,5,11 },
	/* 84, 85 - oscillations (sword at side??) */
	{ 64,10,5,11 },{ 65,10,5,11 },
	/* 86 - asleep */
	{ 66,10,5,11 }
};

extern char bow_x[], bow_y[], bowshotx[], bowshoty[], gunshoty[];

/* var1 is usually clock, var2 is usually direction */

/* three types of doors + F1/F9 doors, F1/F10 doors and F9/F10 doors */

/* horizontals all have lsb set */
#define HWOOD	1
#define VWOOD	2
#define HSTONE	3
#define VSTONE	4
#define HCITY	5
#define VCITY	6
#define CRYST	7
#define SECRET  8
#define BLACK   9
#define MARBLE	10
#define LOG     11
#define	HSTON2	13
#define VSTON2	14
#define STAIR	15
#define DESERT  17
#define CAVE	18
#define VLOG	18		/* same as cave */

#define DOORCOUNT 86

struct door {				/* mark locations of all doors */
	unsigned short
		xc1, yc1,			/* outside image coords relative to F1 */
		xc2, yc2;			/* inside image coords relative to F9 */
	char type;				/* wood, stone */
	char secs;				/* what sectors are joined by this */
} doorlist[DOORCOUNT] = {
	{ 0x1170,0x5060, 0x2870,0x8b60, HWOOD,1 }, /* desert fort */
	{ 0x1170,0x5060, 0x2870,0x8b60, HWOOD,1 }, /* desert fort */
	{ 0x1170,0x5060, 0x2870,0x8b60, HWOOD,1 }, /* desert fort */
	{ 0x1170,0x5060, 0x2870,0x8b60, HWOOD,1 }, /* desert fort */
	{ 0x1390,0x1b60, 0x1980,0x8c60, CAVE,  2 }, /* dragon cave */
	{ 0x1770,0x6aa0, 0x2270,0x96a0, BLACK, 1 }, /* pass fort */
	{ 0x1970,0x62a0, 0x1f70,0x96a0, BLACK, 1 }, /* gate fort */
	{ 0x1aa0,0x4ba0, 0x13a0,0x95a0, DESERT,1 }, /* oasis #1 */
	{ 0x1aa0,0x4c60, 0x13a0,0x9760, DESERT,1 }, /* oasis #4 */
	{ 0x1b20,0x4b60, 0x1720,0x9560, DESERT,1 }, /* oasis #2 */
	{ 0x1b80,0x4b80, 0x1580,0x9580, DESERT,1 }, /* oasis #3 */
	{ 0x1b80,0x4c40, 0x1580,0x9740, DESERT,1 }, /* oasis #5 */
	{ 0x1e70,0x3b60, 0x2880,0x9c60, HSTONE,1 }, /* west keep */
	{ 0x2480,0x33a0, 0x2e80,0x8da0, HWOOD ,1 }, /* swamp shack */
	{ 0x2960,0x8760, 0x2b00,0x92c0, STAIR ,1 }, /* stargate forwards */
	{ 0x2b00,0x92c0, 0x2960,0x8780, STAIR ,2 }, /* stargate backwards */
	{ 0x2c00,0x7160, 0x2af0,0x9360, BLACK ,1 }, /* doom tower */
	{ 0x2f70,0x2e60, 0x3180,0x9a60, HSTONE,1 }, /* lakeside keep */
	{ 0x2f70,0x63a0, 0x1c70,0x96a0, BLACK ,1 }, /* plain fort */
	{ 0x3180,0x38c0, 0x2780,0x98c0, HWOOD ,1 }, /* road's end inn */
	{ 0x3470,0x4b60, 0x0470,0x8ee0, STAIR ,2 }, /* tombs */
	{ 0x3DE0,0x1BC0, 0x2EE0,0x93C0, CRYST ,1 }, /* crystal palace */
	{ 0x3E00,0x1BC0, 0x2F00,0x93C0, CRYST ,1 }, /* crystal palace */
	{ 0x4270,0x2560, 0x2e80,0x9a60, HSTONE,1 }, /* coast keepDB */
	{ 0x4280,0x3bc0, 0x2980,0x98c0, HWOOD ,1 }, /* friendly inn */
	{ 0x45e0,0x5380, 0x25d0,0x9680, MARBLE,1 }, /* mountain keep */
	{ 0x4780,0x2fc0, 0x2580,0x98c0, HWOOD ,1 }, /* forest inn */
	{ 0x4860,0x6640, 0x1c60,0x9a40, VLOG  ,1 }, /* cabin yard #7 */
	{ 0x4890,0x66a0, 0x1c90,0x9aa0, LOG   ,1 }, /* cabin #7 */
	{ 0x4960,0x5b40, 0x2260,0x9a40, VLOG  ,1 }, /* cabin yard #6 */
	{ 0x4990,0x5ba0, 0x2290,0x9aa0, LOG   ,1 }, /* cabin #6 */
	{ 0x49a0,0x3cc0, 0x0ba0,0x82c0, VWOOD ,1 }, /* village #2 */
	{ 0x49d0,0x3dc0, 0x0bd0,0x84c0, VWOOD ,1 }, /* village #1.a */
	{ 0x49d0,0x3e00, 0x0bd0,0x8500, VWOOD ,1 }, /* village #1.b */
	{ 0x4a10,0x3c80, 0x0d10,0x8280, HWOOD ,1 }, /* village #3 */
	{ 0x4a10,0x3d40, 0x0f10,0x8340, HWOOD ,1 }, /* village #5 */
	{ 0x4a30,0x3dc0, 0x0e30,0x85c0, HWOOD ,1 }, /* village #7 */
	{ 0x4a60,0x3e80, 0x1060,0x8580, HWOOD ,1 }, /* village #8 */
	{ 0x4a70,0x3c80, 0x1370,0x8280, HWOOD ,1 }, /* village #4 */
	{ 0x4a80,0x3d40, 0x1190,0x8340, HWOOD ,1 }, /* village #6 */
	{ 0x4c70,0x3260, 0x2580,0x9c60, HSTONE,1 }, /* crag keep */
	{ 0x4d60,0x5440, 0x1f60,0x9c40, VLOG  ,1 }, /* cabin #2 */
	{ 0x4d90,0x4380, 0x3080,0x8d80, HSTON2,1 }, /* crypt */
	{ 0x4d90,0x54a0, 0x1f90,0x9ca0, LOG   ,1 }, /* cabin yard #2 */
	{ 0x4de0,0x6b80, 0x29d0,0x9680, MARBLE,1 }, /* river keep */
	{ 0x5360,0x5840, 0x2260,0x9840, VLOG  ,1 }, /* cabin yard #3 */
	{ 0x5390,0x58a0, 0x2290,0x98a0, LOG   ,1 }, /* cabin #3 */
	{ 0x5460,0x4540, 0x1c60,0x9840, VLOG  ,1 }, /* cabin yard #1 */
	{ 0x5470,0x6480, 0x2c80,0x8d80, HSTONE,1 }, /* elf glade */
	{ 0x5490,0x45a0, 0x1c90,0x98a0, LOG   ,1 }, /* cabin #1 */
	{ 0x55f0,0x52e0, 0x16e0,0x83e0, MARBLE,1 }, /* main castle */
	{ 0x56c0,0x53c0, 0x1bc0,0x84c0, HSTON2,1 }, /* city #15.a */
	{ 0x56c0,0x5440, 0x19c0,0x8540, HSTON2,1 }, /* city #17 */
	{ 0x56f0,0x51a0, 0x19f0,0x82a0, HSTON2,1 }, /* city #10 */
	{ 0x5700,0x5240, 0x1df0,0x8340, VSTON2,1 }, /* city #12 */
	{ 0x5710,0x5440, 0x1c10,0x8640, HSTON2,1 }, /* city #18 */
	{ 0x5730,0x5300, 0x1a50,0x8400, HSTON2,1 }, /* city #14 */
	{ 0x5730,0x5380, 0x1c30,0x8480, VSTON2,1 }, /* city #15.b */
	{ 0x5750,0x51a0, 0x1c60,0x82a0, HSTON2,1 }, /* city #11 */
	{ 0x5750,0x5260, 0x2050,0x8360, HSTON2,1 }, /* city #13 */
	{ 0x5760,0x53c0, 0x2060,0x84c0, HSTON2,1 }, /* city #16 */
	{ 0x5760,0x5440, 0x1e60,0x8540, HSTON2,1 }, /* city #19 */
	{ 0x5860,0x5d40, 0x1c60,0x9a40, VLOG  ,1 }, /* cabin yard #4 */
	{ 0x5890,0x5da0, 0x1c90,0x9ca0, LOG   ,1 }, /* cabin #4 */
	{ 0x58c0,0x2e60, 0x0ac0,0x8860, CAVE,  2 }, /* troll cave */
	{ 0x5960,0x6f40, 0x2260,0x9a40, VLOG  ,1 }, /* cabin yard #9 */
	{ 0x5990,0x6fa0, 0x2290,0x9ca0, LOG   ,1 }, /* cabin #9 */
	{ 0x59a0,0x6760, 0x2aa0,0x8b60, STAIR ,1 }, /* unreachable castle */
	{ 0x59e0,0x5880, 0x27d0,0x9680, MARBLE,1 }, /* farm keep */
	{ 0x5e70,0x1a60, 0x2580,0x9a60, HSTONE,1 }, /* north keep */
	{ 0x5ec0,0x2960, 0x11c0,0x8b60, CAVE  ,2 }, /* spider exit */
	{ 0x6060,0x7240, 0x1960,0x9c40, VLOG  ,1 }, /* cabin yard #10 */
	{ 0x6090,0x72a0, 0x1990,0x9ca0, LOG   ,1 }, /* cabin #10 */
	{ 0x60f0,0x32c0, 0x25f0,0x8bc0, HSTONE,1 }, /* mammoth manor */
	{ 0x64c0,0x1860, 0x03c0,0x8660, CAVE  ,2 }, /* maze cave 2 */
	{ 0x6560,0x5d40, 0x1f60,0x9a40, VLOG  ,1 }, /* cabin yard #5 */
	{ 0x6590,0x5da0, 0x1f90,0x98a0, LOG   ,1 }, /* cabin #5 */
	{ 0x65c0,0x1a20, 0x04b0,0x8840, BLACK ,2 }, /* maze cave 1 */
	{ 0x6670,0x2a60, 0x2b80,0x9a60, HSTONE,1 }, /* glade keep */
	{ 0x6800,0x1b60, 0x2af0,0x9060, BLACK ,1 }, /* witch's castle */
	{ 0x6b50,0x4380, 0x2850,0x8d80, HSTON2,1 }, /* light house */
	{ 0x6be0,0x7c80, 0x2bd0,0x9680, MARBLE,1 }, /* lonely keep */
	{ 0x6c70,0x2e60, 0x2880,0x9a60, HSTONE,1 }, /* sea keep */
	{ 0x6d60,0x6840, 0x1f60,0x9a40, VLOG  ,1 }, /* cabin yard #8 */
	{ 0x6d90,0x68a0, 0x1f90,0x9aa0, LOG   ,1 }, /* cabin #8 */
	{ 0x6ee0,0x5280, 0x31d0,0x9680, MARBLE,1 }  /* point keep */
};

/* 0-49 - regular encounters */
/* 50 = set group encounter */
/* 60 = special figure encounter */
/* 70 = bird or turtle */

/* need to define self-destructing extents?? */

struct extent {
	UWORD x1, y1, x2, y2;
	UBYTE etype, v1, v2, v3;
} *extn, extent_list[] = {
	{  2118,27237,  2618,27637, 70,0,1,11  },		/* bird extent */
	{     0,    0,     0,    0, 70,0,1,5  },		/* turtle extent */
	{  6749,34951,  7249,35351,	70,0,1,10  },		/* dragon extent */
	{  4063,34819,  4909,35125,	53, 4, 1, 6 },		/* spider pit */
	{  9563,33883, 10144,34462, 60, 1, 1, 9 },		/* necromancer */

	{ 22945, 5597, 23225, 5747, 61, 3, 2, 4 },		/* turtle eggs */
	{ 10820,35646, 10877,35670, 83, 1, 1, 0 },		/* princess extent */
	{ 19596,17123, 19974,17401, 48, 8, 8, 2 },		/* graveyard ext */
	{ 19400,17034, 20240,17484, 80, 4,20, 0 },		/* around city */

	/* arena with lots of loraii?? (replenished for 10) */
	/* dungeons with lotsa wraiths = 15, 31, etc. */

	{0x2400,0x8200,0x3100,0x8a00,52,3, 1, 8 },		/* astral plane */
	{  5272,33300,  6112,34200, 81, 0, 1, 0 },		/* king pax */
	{ 11712,37350, 12416,38020, 82, 0, 1, 0 },		/* sorceress pax */
	{  2752,33300,  8632,35400, 80, 0, 1, 0 },		/* peace 1 - buildings */
	{ 10032,35550, 12976,40270, 80, 0, 1, 0 },		/* peace 2 - specials */

	{  4712,38100, 10032,40350, 80, 0, 1, 0 },		/* peace 3 - cabins */
	{ 21405,25583, 21827,26028, 60, 1, 1, 7 },		/* hidden valley */
	{  6156,12755, 12316,15905,	 7, 1, 8, 0 },		/* swamp region */
	{  5140,34860,  6260,37260,	 8, 1, 8, 0 },		/* spider region */

	{   660,33510,  2060,34560,	 8, 1, 8, 0 },		/* spider region */
	{ 18687,15338, 19211,16136, 80, 0, 1, 0 },		/* village */
	{ 16953,18719, 20240,17484,  3, 1, 3, 0 },		/* around village */
	{ 20593,18719, 23113,22769,  3, 1, 3, 0 },		/* around city */

	{ 0,0, 0x7fff, 0x9fff,		 3, 1, 8, 0 },		/* whole world */
};

#define EXT_COUNT 22

unsigned char stone_list[] = 
{	54,43, 71,77, 78,102, 66,121, 12,85, 79,40,
	107,38, 73,21, 12,26, 26,53, 84,60 };

extern struct object ob_listg[], ob_list8[];

struct inv_item inv_list[] = {
	{  12, 10, 0, 0, 0, 8, 1, "Dirk" },
	{   9, 10,10, 0, 0, 8, 1, "Mace" },
	{   8, 10,20, 0, 0, 8, 1, "Sword" },
	{  10, 10,30, 0, 0, 8, 1, "Bow" },
	{  17, 10,40, 0, 8, 8, 1, "Magic Wand" },
/* 5 */
	{  27, 10,50, 0, 0,8,  1, "Golden Lasso" },
	{  23, 10,60, 0, 8,8,  1, "Sea Shell" },
	{  27, 10,70, 0, 8,8,  1, "Sun Stone" },
	{   3, 30, 0, 3, 7, 1,45, "Arrows" },
	{  18, 50, 0, 9, 0, 8,15, "Blue Stone" },
/* 10 */
	{  19, 65, 0, 6, 0, 5,23, "Green Jewel" },
	{  22, 80, 0, 8, 0, 7,17, "Glass Vial" },
	{  21, 95, 0, 7, 0, 6,20, "Crystal Orb" },
	{  23,110, 0,10, 0, 9,14, "Bird Totem" },
	{  17,125, 0, 6, 0, 5,23, "Gold Ring" },
/* 15 */
	{  24,140, 0,10, 0, 9,14, "Jade Skull" },
	{  25,160, 0, 5, 0, 5,25, "Gold Key" },
	{  25,172, 0, 5, 8, 5,25, "Green Key" },
	{ 114,184, 0, 5, 0, 5,25, "Blue Key" },
	{ 114,196, 0, 5, 8, 5,25, "Red Key" },
/* 20 */
	{  26,208, 0, 5, 0, 5,25, "Grey Key" },
	{  26,220, 0, 5, 8, 5,25, "White Key" },
	{  11,  0, 80,0, 8,8,  1, "Talisman" },
	{  19,  0, 90,0, 8,8,  1, "Rose" },
	{  20,  0,100,0, 8,8,  1, "Fruit" },
/* 25 */
	{  21,232, 0,10, 8,8,  5, "Gold Statue" },
	{  22,  0,110,0, 8,8,  1, "Book" },
	{   8, 14, 80,0, 8,8,  1, "Herb" },
	{   9, 14, 90,0, 8,8,  1, "Writ" },
	{  10, 14,100,0, 8,8,  1, "Bone" },
/* 30 */
	{  12, 14,110,0, 8,8,  1, "Shard" },
	{   0,  0, 0, 0, 0, 0, 2, "2 Gold Pieces" },
	{   0,  0, 0, 0, 0, 0, 5, "5 Gold Pieces" },
	{   0,  0, 0, 0, 0, 0,10, "10 Gold Pieces" },
	{   0,  0, 0, 0, 0, 0,100,"100 Gold Pieces" },

	{   0,  0, 0, 0, 0, 0, 0, "quiver of arrows" }
};

#define MAGICBASE	9
#define KEYBASE		16
#define STATBASE	25
#define	GOLDBASE	31
#define	ARROWBASE	35

UBYTE *stuff, julstuff[ARROWBASE], philstuff[ARROWBASE], kevstuff[ARROWBASE];

/* defines the variables for on-screen animated shapes */

struct sshape {
	unsigned char *backsave;
	short	savesize, blitsize, Coff, Cmod;
};

struct sshape *shp;

struct fpage fp_page1, fp_page2, *fp_drawing, *fp_viewing;

struct RasInfo	ri_page1, ri_page2, ri_text, ri_title;
struct BitMap *bm_page1, *bm_page2, *bm_text, *bm_lim, *bm_draw, *bm_source;
struct BitMap bm_scroll, pagea, pageb;
struct RastPort rp_map, rp_text, rp_text2, *rp;

#define	BM_SIZE	(sizeof (struct BitMap))

PLANEPTR *pl;
LONG i;
SHORT j,k,n;

extern struct ColorMap *GetColorMap();
struct GfxBase *GfxBase;
struct Library *LayersBase;

UBYTE *nhinor, *nhivar;
extern UBYTE hinor, hivar;

struct SimpleSprite pointer = { 0,16,0,0,0 };

long *sprite_data, _sprite_data[] = 
{	0,
	0x80060000, 0x60010002, 0x40012000, 0x10030000,
	0x08FE0001, 0x0581007F, 0x03000081, 0x06010101,
	0x0C800201, 0x08410401, 0x08200401, 0x08110401,
	0x080C0401, 0x880F0401, 0x98074400, 0x75560FFC,
	0,0,0
};

extern USHORT pagecolors [];

USHORT textcolors [] =
{	0x000, 0xFFF, 0xC00, 0xF60, 0x00f, 0xc0f, 0x090, 0xFF0,
	0xf90, 0xf0c, 0xA50, 0xFDB, 0xEB7, 0xCCC, 0x888, 0x444,
	0x000, 0xDB0, 0x740, 0xC70 };

USHORT blackcolors [] =
{	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

USHORT introcolors[] =
{	0x000, 0xFFF, 0xE00, 0xA00, 0xD80, 0xEC0, 0x390, 0x021,
	0xEEB, 0xEDA, 0xEEA, 0xCB8, 0xA95, 0x973, 0x840, 0x620,
	0xA52, 0xC74, 0xD96, 0xFCA, 0x449, 0x444, 0xDC9, 0x668,
	0x33F, 0x888, 0xA60, 0xAAF, 0xBBB, 0xCCF, 0xDDD, 0xEEE };

char numbuf[11] = { 0,0,0,0,0,0,0,0,0,0,' '};

/* definitions for the option menus */

enum cmodes {ITEMS=0, MAGIC, TALK, BUY, GAME, SAVEX, KEYS, GIVE, USE, FILE};

char label1[] = "ItemsMagicTalk Buy  Game ";
char label2[] = "List Take Look Use  Give ";
char label3[] = "Yell Say  Ask  ";
char label4[] = "PauseMusicSoundQuit Load ";
char label5[] = "Food ArrowVial Mace SwordBow  Totem";
char label6[] = "StoneJewelVial Orb  TotemRing Skull";
char label7[] = "Dirk Mace SwordBow  Wand LassoShellKey  Sun  Book ";
char label8[] = "Save Exit ";
char label9[] = "Gold GreenBlue Red  Grey White";
char labelA[] = "Gold Book Writ Bone ";
char labelB[] = "  A    B    C    D    E    F    G    H  ";
/*               |    |    |    |    |    |    |    |    |    |    |    | */

char keycolors[] = { 8,6,4,2,14,1 };
char hitgo;		/* is the option we hit enabled */

/* bit0 = selected, bit1 = displayed, else = type
	4 = toggle, 8 = immediate, 12 = radio buttons, 0 = not changeable */

char real_options[12];

struct menu {
	char 	*label_list;
	char	num, color;
	char	enabled[12];
} menus[10] = {
	{ label2, 10,6, 3,2,2,2,2,10,10,10,10,10,0,0 },		/* items */
	{ label6, 12,5, 2,3,2,2,2,8,8,8,8,8,8,8 },			/* magic */
	{ label3, 8,9,  2,2,3,2,2,10,10,10,0,0,0,0 },		/* talk */
	{ label5, 12,10,2,2,2,3,2,10,10,10,10,10,10,10 },	/* buy */
	{ label4, 10,2, 2,2,2,2,3,6,7,7,10,10,0,0 },		/* game */
	{ label8, 7,0,  2,2,2,2,2,10,10,0,0,0,0 },	 		/* save/exit */
	{ label9, 11,8, 2,2,2,2,2,10,10,10,10,10,10 },		/* keys */
	{ labelA, 9,10, 2,2,2,2,2,10,0,0,0,0,0 },			/* give */
	{ label7, 10,8, 10,10,10,10,10,10,10,10,10,0,10,10 },	/* use */
	{ labelB, 10,5, 10,10,10,10,10,10,10,10,0,0,0,0 }};	/* files */

#define LMENUS 38

struct letters
{	char letter,menu,choice; }
letter_list[] = {
	'I',ITEMS,5, 'T',ITEMS,6, '?',ITEMS,7, 'U',ITEMS,8, 'G',ITEMS,9,
	'Y',TALK,5, 'S',TALK,6, 'A',TALK,7,
	' ',GAME,5, 'M',GAME,6, 'F',GAME,7, 'Q',GAME,8, 'L',GAME,9,
	'O',BUY,5, 'R',BUY,6, '8',BUY,7, 'C',BUY,8, 'W',BUY,9, 'B',BUY,10, 'E',BUY,11,
	'V',SAVEX,5, 'X',SAVEX,6,
	10,MAGIC,5, 11,MAGIC,6, 12,MAGIC,7, 13,MAGIC,8, 14,MAGIC,9, 15,MAGIC,10,
		16,MAGIC,11,
	'1',USE,0, '2',USE,1, '3',USE,2, '4',USE,3, '5',USE,4, '6',USE,5,
		'7',USE,6, 'K',USE,7
};

char	hit;	/* which menu we hit */

/***** this section defines some variables that are used in maintaining 
	   the playing map */

extern UBYTE place_tbl[], inside_tbl[];
extern char place_msg[], inside_msg[];

unsigned short	map_x, map_y,	/* absolute map coordinates in pixels */
				hero_x, hero_y, /* shorthand variables for hero location */
				safe_x, safe_y,	safe_r, /* last 'safe zone' visited */
				img_x, img_y;	/* absolute sector coordinates */
/* 18 */
short	cheat1;
short	riding, flying, wcarry;
short	turtleprox, raftprox;
short	brave, luck, kind, wealth, hunger, fatigue;
/* 24 */
short	brother;
short	princess;
short	hero_sector;			/* what sector is the hero on?? */
USHORT	hero_place;				/* what place name is this? */
/* 8 */
USHORT	daynight, lightlevel;
short	actor_file, set_file;	/* which actor or setfig file is loaded */
short	active_carrier;			/* is turtle of bird active */
USHORT	xtype;
short	leader;					/* leader of the enemies */
short	secret_timer, light_timer, freeze_timer;
short	cmode;
USHORT	encounter_type;
/* 28 */
USHORT	pad1,pad2,pad3,pad4,pad5,pad6,pad7;

char	viewstatus;				/* 0 = normal, 1 = big, 99 = corrupt */
char	flasher;
char	actors_on_screen;		/* is there anyone else on the screen?? */
char	actors_loading;			/* currently attempting to load actors */
char	safe_flag;				/* is this a safe area?? */
char	battleflag;				/* are we in battle? */
char	frustflag;				/* is the character blocked ?? */
char	quitflag;				/* is it time to quit?? */
char	witchflag, wdir;		/* is the witch on the screen */
unsigned char goodfairy;		/* good fairy on screen? */
extern short s1,s2;
short	nearest;				/* nearest object/character to player */
short	nearest_person, perdist;/* nearest character to player */
short	last_person;			/* last character near to player */
UBYTE	witchindex;
short	dayperiod;
short	sleepwait;
unsigned char	encounter_number, danger_level;
unsigned short	encounter_x, encounter_y;	/* encounter origin */
short mixflag, wt;

char *datanames[] = { "Julian","Phillip","Kevin", };

unsigned short xreg, yreg;	/* where the region is */

#define TERRA_BLOCK	149
#define NO_REGION	10
#define MAP_STABLE	(new_region >= NO_REGION)
#define	MAP_FLUX	(new_region < NO_REGION)

UWORD new_region, lregion, region_num = 3;
struct need current_loads = { 0,0,0,0, 1,2,0,0,0 };
struct need file_index[10] = {
	{ 320,480,520,560,  0,1, 32,160,22 }, /* F1 - snowy region */
	{ 320,360,400,440,  2,3, 32,160,21 }, /* F2 - witch wood */
	{ 320,360,520,560,  2,1, 32,168,22 }, /* F3 - swampy region */
	{ 320,360,400,440,  2,3, 32,168,21 }, /* F4 - plains and rocks */
	{ 320,480,520,600,  0,4, 32,176, 0 }, /* F5 - desert area */
	{ 320,280,240,200,  5,6, 32,176,23 }, /* F6 - bay / city / farms */
	{ 320,640,520,600,  7,4, 32,184, 0 }, /* F7 - volcanic */
	{ 320,280,240,200,  5,6, 32,184,24 }, /* F8 - forest and wilderness */
	{ 680,720,800,840,  8,9, 96,192, 0 }, /* F9  - inside of buildings */
	{ 680,760,800,840, 10,9, 96,192, 0 }  /* F10 - dungeons and caves */
};

/* playing map is 6 * 19 = 114 */

extern short minimap[114];

#define MAXCOORD (16*16*128)
#define MAXMASK	 MAXCOORD - 1

#define	SETFN(n)	openflags |= n
#define TSTFN(n)	openflags & n

#define	QPLAN_SZ	4096			/* 1 plane of 64 chars */
#define IPLAN_SZ	16384			/* 1 plane of 256 chars */
#define	IMAGE_SZ	(IPLAN_SZ * 5)	/* 5 planes of 256 chars - 81K */
#define	SHAPE_SZ	(78000)
#define SHADOW_SZ	8192+4096		/* background masks */
#define	SECTOR_SZ	((128*256)+4096)	/* 256 sectors - 32K+4K region map */
#define	SECTOR_OFF	(128*256)		/* 256 sectors - 32K */
#define SAMPLE_SZ	(5632)			/* 5K for samples */

long	LoadSeg(), seg;
struct	DiskFontHeader *font;
struct  TextFont *tfont, *afont;
struct  TextAttr topaz_ta = { "topaz.font", 8, 0, FPF_ROMFONT };

unsigned char 
	*into_chip(),
	*image_mem, *sector_mem, *map_mem, *shadow_mem, 
	*shape_mem,
	*bmask_mem, *queue_mem,
	*sample_mem,
	*terra_mem;	/* Terrain data */
#define free_chip(new,old,size) if (new!=old) FreeMem(new,size);

unsigned char *nextshape, *tempshape;

#define  S_WAVBUF	(128 * 8)
#define  S_VOLBUF	(10 * 256)
#define VOICE_SZ  (S_WAVBUF	+ S_VOLBUF)
#define SCORE_SZ	5900

unsigned char  *wavmem, *volmem, *scoremem;
short new_wave[] = 
{ 	0x0000, 0x0000, 0x0000, 0x0000, 0x0005,
	0x0202, 0x0101, 0x0103, 0x0004, 0x0504, 
	0x0100, 0x0500 };

UWORD	openflags;
PLANEPTR	*planes;

unsigned char *mask_buffer, *shapedata;
short	shift, aoff, boff, bmod, planesize, wmask;

#define	CBK_SIZE	(96<<6)+5

long seed1 = 19837325, seed2 = 23098324;

/* features:
	1 = impassable, 2 = sink, 3 = slow/brush;
	others: slippery, fiery, changing, climbable, pit trap, danger,
		noisy, magnetic, stinks, slides, slopes, whirlpool, etc.

	mask applications : 
		0 = never, 1 = when down, 2 = when right, 3 = always unless flying,
		4 = only if below normal level, etc.
*/

struct in_work handler_data;

/* this function opens everything that needs to be opened */

/* allocation definitions */

#define	AL_BMAP		0x0001
#define	AL_GBASE	0x0002
#define	AL_HANDLE	0x0004
#define	AL_MUSIC	0x0008
#define	AL_IMAGE	0x0010
#define	AL_SECTOR	0x0020
#define	AL_MASK		0x0040
#define	AL_SHAPE	0x0080
#define	AL_SHADOW	0x0100
#define	AL_FONT		0x0200
#define	AL_SAMPLE	0x0400
#define AL_PORT		0x0800
#define AL_IOREQ	0x1000
#define AL_TDISK	0x2000
#define AL_TERR		0x4000

struct BitMap *wb_bmap;
struct Layer_Info *li, /* *NewLayerInfo() */ ;
struct Process *thistask /* , *FindTask() */ ;
BPTR origDir;
int trapper();

struct IOAudio *ioaudio;
struct MsgPort *audioport;
BOOL			audio_open;

struct BitMap work_bm;

open_all()
{	register long i;
	long file;

	openflags = 0;

	if ((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",0)) == NULL) return 2;
	if ((LayersBase = (struct Library *)OpenLibrary("layers.library",0)) == NULL) return 2;
	SETFN(AL_GBASE);		/* opened the graphics library */
	oldview = GfxBase->ActiView;

	if (!MakeBitMap(&work_bm,2,640,200)) return 2;

	li = (struct Layer_Info *)NewLayerInfo();
	InitRastPort(&rp_map);
	InitRastPort(&rp_text);
	InitRastPort(&rp_text2);
	rp = &rp_text;

	/* wb_bmap = oldview->ViewPort->RasInfo->BitMap; */ /* workbench bitmap */
	wb_bmap = &work_bm;

	rp_text.BitMap = wb_bmap;	/* workbench screen (not!) */
	SetBPen(rp,0);
	SetDrMd(rp,JAM2);
	SetRast(rp,0);		/* clear workbench screen (not!) */

	if ((bm_page1 =
		(struct BitMap *)AllocMem(5*BM_SIZE,MEMF_CHIP | MEMF_CLEAR)) == NULL )
			return 1;
	SETFN(AL_BMAP);		/* allocated the bitmap structures */

	if (i = AllocDiskIO()) return i;

#if 0
	if ((diskport = CreatePort(0,0))==0) return 30;
	SETFN(AL_PORT);
	if ((diskreq1=(struct IOExtTD *)CreateExtIO(diskport,sizeof(struct IOExtTD)))==0) return 31;
	SETFN(AL_IOREQ);
	if (OpenDevice(TD_NAME,0,(struct IORequest *)diskreq1,0)) return 32;
	SETFN(AL_TDISK);
	for (i=0; i<9; i++)
	{	diskreqs[i] = *diskreq1;
	}
#endif

	if ((seg = LoadSeg("fonts/Amber/9")) == NULL) return 15;
	font = (struct DiskFontHeader *) ((seg<<2)+8);
	SETFN(AL_FONT);		/* opened the font */

	tfont = (struct  TextFont *)OpenFont(&topaz_ta);
	SetFont(&rp_text,tfont);
	SetFont(&rp_text2,tfont);
	SetFont(&rp_map,tfont);
	afont = &(font->dfh_TF);

	/* add the input handler */
	handler_data.xsprite = handler_data.ysprite = 320;
	handler_data.gbase = GfxBase;
	handler_data.pbase = 0;
	if (add_device() == FALSE) return 4;

	thistask = (struct Process *)FindTask(0);
	thistask->pr_WindowPtr = (APTR)-1;
	thistask->pr_Task.tc_TrapCode = (APTR)trapper;
	/* set trap handler */

	SETFN(AL_HANDLE);
	FreeSprite(0);
	GetSprite(&pointer,0);	/* test for error */

	InitView(&v);				/* initialize view */

	InitVPort(&vp_page);		/* init view ports */
	InitVPort(&vp_text);		/* text page */

	v.ViewPort = &vp_text;		/* link view into viewport */
	vp_text.Next = &vp_page;	/* make links to additional viewports */
	vp_page.Next = NULL;

	vp_page.DWidth = 288;		/* lo-res screen is 36 bytes wide */
	vp_text.DWidth = 640;		/* hi-res screen */

	vp_page.DxOffset = 16;
	vp_page.DyOffset = vp_text.DxOffset = 0;
	vp_page.DHeight = 140;

	vp_text.DyOffset = PAGE_HEIGHT;
	vp_text.DHeight = TEXT_HEIGHT;		/* make bigger later */

	vp_text.Modes = HIRES | SPRITES | VP_HIDE;

	/* init bit map (for rasinfo and rastport) */

	bm_page2 = bm_page1 + 1;
	bm_text = bm_page1 + 2;
	bm_source = bm_page1 + 3;
	bm_lim = bm_page1 + 4;
	InitBitMap(bm_page1,PAGE_DEPTH,PHANTA_WIDTH,RAST_HEIGHT);
	InitBitMap(bm_page2,PAGE_DEPTH,PHANTA_WIDTH,RAST_HEIGHT);
	InitBitMap(bm_text,4,640,TEXT_HEIGHT);
	InitBitMap(bm_lim,1,320,200);
	InitBitMap(&pagea,5,320,200);
	InitBitMap(&pageb,5,320,200);
	InitBitMap(&bm_scroll,1,640,TEXT_HEIGHT);
	InitBitMap(bm_source,3,64,24);

	rp_text2.BitMap = bm_text;

	/* (init RasInfo) */

	ri_page1.BitMap = bm_page1;
	ri_page2.BitMap = bm_page2;
	ri_page1.RxOffset = ri_page2.RxOffset = 
		ri_page1.RyOffset = ri_page2.RyOffset = 0;
	ri_page1.Next = ri_page2.Next = NULL;

	ri_text.BitMap = bm_text;
	ri_text.RxOffset = ri_text.RyOffset = 0;
	ri_text.Next = NULL;

	fp_page1.savecop = fp_page2.savecop = NULL;
	fp_page1.ri_page = &ri_page1;
	fp_page2.ri_page = &ri_page1;
	fp_drawing = &fp_page1; fp_viewing = &fp_page2;

	/* (init color table) */

	vp_page.ColorMap = GetColorMap(32);
	vp_text.ColorMap = GetColorMap(20);

	/* reset all plane pointers */

	for (i=0; i<5; i++) bm_page1->Planes[i] = bm_page2->Planes[i] = NULL;

	/* allocate space for bitmap */
	for (i=0; i<5; i++)
	{	bm_page1->Planes[i] = (void *)AllocRaster(PHANTA_WIDTH,RAST_HEIGHT);
		if(bm_page1->Planes[i] == NULL) return 4;
		bm_page2->Planes[i] = (void *)AllocRaster(PHANTA_WIDTH,RAST_HEIGHT);
		if(bm_page2->Planes[i] == NULL) return 5;
	}

	bm_text->Planes[0] = wb_bmap->Planes[0];
	bm_text->Planes[1] = wb_bmap->Planes[1];
	bm_text->Planes[2] = bm_text->Planes[0] + (TEXT_HEIGHT*80);
	bm_text->Planes[3] = bm_text->Planes[1] + (TEXT_HEIGHT*80);

	queue_mem = bm_text->Planes[2] + (TEXT_HEIGHT*80);
	bmask_mem = bm_text->Planes[3] + (TEXT_HEIGHT*80);

	fp_page1.backsave = queue_mem + 962;
	fp_page2.backsave = bmask_mem + 962;

	fp_page1.shape_queue = (struct sshape *) queue_mem;
	fp_page2.shape_queue = (struct sshape *)
		(queue_mem + ((sizeof (struct sshape))*MAXSHAPES));

	vp_text.RasInfo = &ri_text;
	MakeVPort( &v, &vp_text );

	if (audioport = (struct MsgPort *)CreatePort(NULL,0))
	{
		if (ioaudio = (struct IOAudio *)CreateExtIO(audioport,sizeof *ioaudio))
		{	UBYTE	data = 0x0f;

			ioaudio->ioa_Data = &data;
			ioaudio->ioa_Length = 1;

			if (!OpenDevice("audio.device", 0L, &ioaudio->ioa_Request,0L))
			{
				ioaudio->ioa_Request.io_Command = CMD_RESET;
				ioaudio->ioa_Request.io_Flags = IOF_QUICK;
				BeginIO(&ioaudio->ioa_Request);

				if ( !(ioaudio->ioa_Request.io_Flags & IOF_QUICK) )
					WaitIO(&ioaudio->ioa_Request);

				audio_open = 1;
			}
		}
	}

	if ((wavmem = (unsigned char *)AllocMem(VOICE_SZ,MEMF_CHIP)) == NULL) return 16;
	if ((scoremem = (unsigned char *)AllocMem(SCORE_SZ,0)) == NULL) return 17;
	volmem = wavmem + S_WAVBUF;
	init_music(new_wave,wavmem,volmem);
	SETFN(AL_MUSIC);

	if ((image_mem = (unsigned char *)AllocMem(IMAGE_SZ,MEMF_CHIP)) == NULL) return 6;
	SETFN(AL_IMAGE);
	if ((sector_mem = (unsigned char *)AllocMem(SECTOR_SZ,MEMF_CHIP)) == NULL) return 7;
	map_mem = sector_mem + SECTOR_OFF;
	SETFN(AL_SECTOR);
	if ((shape_mem = (unsigned char *)AllocMem(SHAPE_SZ,MEMF_CHIP)) == NULL) return 8;
	SETFN(AL_SHAPE);
	if ((shadow_mem = (unsigned char *)AllocMem(SHADOW_SZ,MEMF_CHIP)) == NULL) return 10;
	SETFN(AL_SHADOW);
	if ((sample_mem = (unsigned char *)AllocMem(SAMPLE_SZ,MEMF_CHIP)) == NULL) return 11;
	SETFN(AL_SAMPLE);
	if ((terra_mem = (unsigned char *)AllocMem(1024,MEMF_CHIP)) == NULL) return 34;
	SETFN(AL_TERR);

	if (file = Open("v6",1005))
	{	Read(file,wavmem,S_WAVBUF);
		Seek(file,S_WAVBUF,0);
		Read(file,volmem,S_VOLBUF);
		Close(file);
	}

	bm_scroll.Planes[0] = bm_text->Planes[0];

	sprite_data = (long *)into_chip((void *)_sprite_data,88);

	ChangeSprite(&vp_text,&pointer,(void *)sprite_data);
	handler_data.vbase = &vp_text;

	nhinor = into_chip(&hinor,(16*16));
	nhivar = into_chip(&hivar,(16*16));
	return 0;
}

close_all()
{	register long i;
	if (TSTFN(AL_TERR)) FreeMem(terra_mem,1024);
	if (TSTFN(AL_SAMPLE)) FreeMem(sample_mem,SAMPLE_SZ);
	if (TSTFN(AL_SHADOW)) FreeMem(shadow_mem,SHADOW_SZ);
	if (TSTFN(AL_SHAPE)) FreeMem(shape_mem,SHAPE_SZ);
	if (TSTFN(AL_SECTOR)) FreeMem(sector_mem,SECTOR_SZ);
	if (TSTFN(AL_IMAGE)) FreeMem(image_mem,IMAGE_SZ);
	if (TSTFN(AL_MUSIC))
	{	wrap_music(); FreeMem(wavmem,VOICE_SZ); FreeMem(scoremem,SCORE_SZ); }

	free_chip(sprite_data,_sprite_data,88);
	free_chip(nhinor,&hinor,(16*16));
	free_chip(nhivar,&hivar,(16*16));
	LoadView(oldview);
	FreeVPortCopLists(&vp_page);
	FreeVPortCopLists(&vp_text);
	FreeCprList(fp_page1.savecop);
	FreeCprList(fp_page2.savecop);
	FreeCprList(v.SHFCprList);
	
	if (vp_page.ColorMap) FreeColorMap(vp_page.ColorMap);
	if (vp_text.ColorMap) FreeColorMap(vp_text.ColorMap);
	for(i=0; i<PAGE_DEPTH; i++)
		if (bm_page1->Planes[i])
			FreeRaster(bm_page1->Planes[i],PHANTA_WIDTH,RAST_HEIGHT);
	for(i=0; i<PAGE_DEPTH; i++)
		if (bm_page2->Planes[i])
			FreeRaster(bm_page2->Planes[i],PHANTA_WIDTH,RAST_HEIGHT);


	if (audio_open) CloseDevice(&ioaudio->ioa_Request);
	if (ioaudio) DeleteExtIO(&ioaudio->ioa_Request);
	if (audioport) DeletePort(audioport);

	if (TSTFN(AL_HANDLE)) wrap_device();
	if (TSTFN(AL_FONT)) { UnLoadSeg(seg); CloseFont(tfont); }

#if 0
	if (TSTFN(AL_TDISK)) CloseDevice((struct IORequest *)diskreq1);
	if (TSTFN(AL_IOREQ))  DeleteExtIO((struct IORequest *)diskreq1);
	if (TSTFN(AL_PORT)) DeletePort(diskport);
#endif

	FreeDiskIO();

	if (TSTFN(AL_GBASE)) { CloseLibrary((struct Library *)GfxBase); CloseLibrary(LayersBase); }
	if (TSTFN(AL_BMAP)) FreeMem(bm_page1,5*BM_SIZE);
	if (TSTFN(AL_BMAP)) UnMakeBitMap(&work_bm);
	DisposeLayerInfo(li);
	FreeSprite(0);
	openflags = 0;

	if (origDir) CurrentDir(origDir);

	exit(0);
}

short oldir = 9;
short keydir = 0, keyfight;
char diroffs[16] = {16,16,24,24,0,0,8,8,56,56,68,68,32,32,44,44};

/* music variables */
unsigned char *(track[32]);
unsigned char *(sample[6]);
unsigned long sample_size[6];

char pass, passmode;

extern long myfile, header, blocklength;

/* reads an IFF sample - shorten file format later */

read_sample()
{	long ifflen; register unsigned char *num, *smem; register long i;
	register long sp_load, sp_count;

	sp_load = sp_count = 0;
	load_track_range(920,11,sample_mem,8);

	WaitDiskIO(8); /* WaitIO((struct IORequest *)&diskreqs[8]); */
	InvalidDiskIO(8); /* diskreqs[8].iotd_Req.io_Command = CMD_INVALID; */

	smem = sample_mem;
	for (i=0; i<6; i++)
	{	num = (unsigned char *)(&ifflen);
		*num++ = *smem++; *num++ = *smem++;
		*num++ = *smem++; *num++ = *smem++;
		sample[i] = smem;
		sample_size[i] = ifflen;
		smem += ifflen;
	}
}

/* subroutine for opening doors */

extern char skipp;
char bumped;
enum ky {NOKEY=0, GOLD, GREEN, KBLUE, RED, GREY, WHITE};

/* add more door types like gates, etc */

/* open doors */
struct door_open {
	UBYTE	door_id;
	USHORT	map_id;			/* what number door and map */
	UBYTE	new1,new2;		/* new identities */
	UBYTE	above;			/* 0 = none 1=above, 2=side, 3=back */
	UBYTE	keytype;		/* key type needed to unlock */
} open_list[17] = {
	{ 64, 360, 123,124, 2, GREEN }, /* HSTONE */
	{120, 360, 125,126, 2, NOKEY }, /* HWOOD */
	{122, 360, 127,  0, 0, NOKEY }, /* VWOOD */
	{ 64, 280, 124,125, 2, GREY }, /* HSTONE2 */
	{ 77, 280, 126,  0, 0, GREY }, /* VSTONE2 */
	{ 82, 480,  84, 85, 2, KBLUE }, /* CRYST */
	{ 64, 480, 105,106, 2, GREEN }, /* OASIS */
	{128, 240, 154,155, 1, WHITE }, /* MARBLE */
	{ 39, 680,  41, 42, 2, GOLD }, /* HGATE */
	{ 25, 680,  27, 26, 3, GOLD }, /* VGATE */
	{114, 760, 116,117, 1, RED }, /* SECRET */
	{118, 760, 116,117, 1, GREY }, /* TUNNEL */
	{136, 800, 133,134,135,GOLD }, /* GOLDEN */
	{187, 800,  76, 77, 2, NOKEY }, /* HSTON3 */
	{ 73, 720,  75,  0, 0, NOKEY }, /* VSTON3 */
	{165, 800,  85, 86, 4, GREEN }, /* CABINET */
	{210, 840, 208,209, 2, NOKEY }  /* BLUE */
};

UBYTE *mapxy();

doorfind(x,y,keytype) register USHORT x,y; register ULONG keytype;
{	UBYTE sec_id; short reg_id, j, k; register ULONG l;
	if (px_to_im(x,y)==15) goto found;
	x += 4; if (px_to_im(x,y)==15) goto found;
	x -= 8; if (px_to_im(x,y)==15) goto found;
	return FALSE;
found:
	if (px_to_im(x-16,y)==15) x-=16;
	if (px_to_im(x-16,y)==15) x-=16;
	if (px_to_im(x,y+32)==15) y+=32;
	x >>= 4;
	y >>= 5;
	/* okay-- we seem to know where the door starts. now which type is it */
	sec_id = *(mapxy(x,y));
	reg_id = current_loads.image[(sec_id>>6)];
/*	prdec(x,4); prdec(y,4); prdec(sec_id,3); */
	for (j=0; j<17; j++)
	{	if (open_list[j].map_id==reg_id && open_list[j].door_id==sec_id)
		{	if (open_list[j].keytype==0 || open_list[j].keytype==keytype)
			{	*(mapxy(x,y)) = open_list[j].new1;
				k = open_list[j].new2;
				if (k)
				{	l = open_list[j].above;
					if (l == 1) *(mapxy(x,y-1)) = k;
					else if (l == 3) *(mapxy(x-1,y)) = k;
					else if (l == 4)
					{	*(mapxy(x,y-1)) = 87; *(mapxy(x+1,y)) = 86;
						*(mapxy(x+1,y-1)) = 88;
					}
					else
					{	*(mapxy(x+1,y)) = k;
						if (l != 2) *(mapxy(x+2,y)) = open_list[j].above;
					}
				}
				viewstatus = 99;
				keydir = 0;
				print("It opened.");
				return TRUE;
			}
		}
	}
	if (!bumped && !keytype) print("It's locked.");
	bumped = 1;
	return FALSE;
}

extern char titletext[];

void main(int argc,char argv[])
{	register long i;
	short j, k; char key;
	short dif_x, dif_y, xstart, ystart, xstop, ystop;
	unsigned short	xtest, ytest;
	struct shape *an;
		
	if (argc == 0)
	{	extern struct WBStartup *WBenchMsg;

		origDir = CurrentDir(WBenchMsg->sm_ArgList->wa_Lock);
	}

	light_timer = 0;
	i = open_all();
	if (i) { do_error(i); goto quit_all; }

	stopscore();

	vp_page.RasInfo = &ri_page1;
	fp_page2.ri_page = &ri_page2;
	rp_map.BitMap =  fp_viewing->ri_page->BitMap; SetRast(&rp_map,0);
	rp_map.BitMap =  fp_drawing->ri_page->BitMap; SetRast(&rp_map,0);
	rp = &rp_map;
	screen_size(156);
	SetRGB4(&vp_page,0,0,0,6);
	SetRGB4(&vp_page,1,15,15,15);

	/* showlegals */

	i = rp->FgPen;
	SetAPen(rp,1);
	ssp(titletext);
	SetAPen(rp,i);

	Delay(50);

	rp = &rp_text;
	rp_text.BitMap = &bm_scroll;
	SetFont(rp,afont); SetAPen(rp,10); SetBPen(rp,11);

	read_score();
	read_sample();

	Delay(50);

	vp = &vp_page;			/* so that iff subs can communicate */
	vp_page.RasInfo = &ri_page1;

	for (i=0; i<5; i++)
	{	pagea.Planes[i] = (pageb.Planes[i]=image_mem+(i*8000)) + 40000; }
	bm_lim->Planes[0] = sector_mem;

	playscore(track[12],track[13],track[14],track[15]);

	LoadRGB4(&vp_text,blackcolors,32);

	fp_page2.ri_page = &ri_page2;
	screen_size(0);

	pagechange();		/* 'prime the pump' */
	pagechange();

	if (skipint()) goto no_intro;

	unpackbrush("page0",&pageb,0,0);
	BltBitMap(&pageb,0,0,bm_page1,0,0,320,200,0xC0,0x1f,0);
	BltBitMap(&pageb,0,0,bm_page2,0,0,320,200,0xC0,0x1f,0);

	fp_page2.ri_page = &ri_page1;
	for (i=0; i<=160; i+=4) screen_size(i);
	fp_page2.ri_page = &ri_page2;

	if (skipint()) goto end_intro;
	copypage("p1a","p1b",21,29);
	copypage("p2a","p2b",20,29);
	copypage("p3a","p3b",20,33);
	if (!skipp) Delay(190);
end_intro:
	fp_page2.ri_page = &ri_page1;
	for (i=156; i>=0; i-=4) screen_size(i);

no_intro:
	seekn();
	/* go to normal (playing) screen mode */

	fp_page2.ri_page = &ri_page2;
	rp_map.BitMap =  fp_viewing->ri_page->BitMap; SetRast(&rp_map,0);
	rp_map.BitMap =  fp_drawing->ri_page->BitMap; SetRast(&rp_map,0);

	LoadRGB4(&vp_text,blackcolors,32);
	screen_size(156);
	SetRGB4(&vp_page,0,0,0,3);
	load_track_range(896,24,shadow_mem,0);
	WaitLastDiskIO(); /* WaitIO((struct IORequest *)lastreq); */
	InvalidLastDiskIO(); /* lastreq->iotd_Req.io_Command = CMD_INVALID; */

	SetRGB4(&vp_page,0,0,0,6);
	unpackbrush("hiscreen",bm_text,0,0);

	SetRGB4(&vp_page,1,15,15,15);

	rp = &rp_map;
	rp_map.BitMap =  fp_drawing->ri_page->BitMap;
	stillscreen();
	SetAPen(rp,1);
	placard_text(19);
	handler_data.laydown = handler_data.pickup = 0;
	k = TRUE;
	if (copy_protect_junk()==0) goto quit_all;
	Delay(20);
	
	ri_page1.RxOffset = ri_page2.RxOffset =
		ri_page1.RyOffset = ri_page2.RyOffset = 0;

	stopscore();
	revive(TRUE);
	rp_map.BitMap = fp_drawing->ri_page->BitMap;

	/* establish text page */

	vp_text.DyOffset = PAGE_HEIGHT;
	vp_text.DHeight = TEXT_HEIGHT;
	vp_page.DHeight = 140;
	vp_page.DxOffset = 16;
	vp_page.DWidth = 288;		/* lo-res screen is 36 bytes wide */
	vp_page.DyOffset = 0;
	MakeVPort( &v, &vp_text );
	LoadRGB4(&vp_text,textcolors,20);
	handler_data.pbase = &pointer;

	fp_page1.obcount = fp_page2.obcount = 0;
	fp_page1.wflag = fp_page2.wflag = 0;

	viewstatus = 99;

	cmode = 0; print_options();

	/* main program loop */

	cheat1 = quitflag = FALSE;
	while (!quitflag)
	{	short cycle, atype, inum, notpause;
		BYTE *pia = (BYTE *)0xbfe001;
		unsigned char *backalloc, crack;

		cycle++;
		flasher++;

		key = getkey();
			
		/* debug keys */

		notpause = !(menus[GAME].enabled[5] & 1);
		if (key)
		{	if (viewstatus && notpause)
			{	if ((key & 0x80) == 0) viewstatus = 99; }
			else if (anim_list[0].state == DEAD) ; /* MAYBE */
			else if (key >= 20 && key <= 29)
			{	keydir = key;	}
			else if ((key & 0x7f) == keydir) keydir = 0;
			else if (key == '0') keyfight = TRUE;
			else if ((key & 0x7f) == '0') keyfight = FALSE;

			else if (key == 'B' && cheat1)
			{	if (active_carrier == 11) stuff[5] = 1;
				move_extent(0,hero_x+20,hero_y+20);
				load_carrier(11);
			}
			else if (key == '.' && cheat1)
				{	stuff[rnd(GOLDBASE)]+=3; set_options(); stuff[22]=0; }
			else if ((key & 0x7f) >= 0x61)
			{	if (inum > 11) inum = 11;
				if (inum < 0) inum = 0;
				inum = (key&0x7f) - 0x61;
				hit = real_options[inum];
				if (key & 0x80)	/* up mouse */
				{	if (hit >= 0 && hit < menus[cmode].num) propt(inum,0); }
				else	/* down mouse */
				{	hitgo = TRUE;
					if (hit >= 0 && hit < menus[cmode].num)
					{	atype = menus[cmode].enabled[hit] & 0xfc;
						if (atype != 8) handler_data.lastmenu = 0;
						if (atype == 4)
						{	menus[cmode].enabled[hit] ^= 1;
							propt(inum,menus[cmode].enabled[hit] & 1);
							do_option(hit);
						}
						else if (atype == 8)
						{	propt(inum,1);
							do_option(hit);
						}
						else if (atype == 12)
						{	menus[cmode].enabled[hit] |= 1;
							propt(inum,1);
							do_option(hit);
						}		/* go to different 'BUY' menus?? */
						else if (!notpause);
						else if (atype == 0 && hit < 5) { cmode = hit; prq(5); }
						else propt(inum,menus[cmode].enabled[hit] & 1);
					}
				}
			}

			else if (key == 'R' && cheat1) rescue();
			else if (key == '=' && cheat1) prq(2);
			else if (key == 19 && cheat1) prq(3);
			else if (key == 18 && cheat1) daynight += 1000;
			else if (key == 1 && cheat1) { anim_list[0].abs_y -= 150; map_y -= 150; }
			else if (key == 2 && cheat1) { anim_list[0].abs_y += 150; map_y += 150; }
			else if (key == 3 && cheat1) { anim_list[0].abs_x += 280; map_x += 280; }
			else if (key == 4 && cheat1) { anim_list[0].abs_x -= 280; map_x -= 280; }
			else if (cmode == KEYS && ((key & 0x80) == 0))	/* key keys */
			{	if (key >= '1' && key <= '6') do_option(key - '1' + 5);
				else gomenu(ITEMS);
			}
			else if (key == ' ' || notpause)
			{	for (i=0; i<LMENUS; i++)
				{	if (letter_list[i].letter == key)
					{	short menu;
						menu = letter_list[i].menu;
						if (menu == SAVEX && cmode != SAVEX) break;
						cmode = menu;
						/* how to highlight option?? */
						hit = letter_list[i].choice;
						hitgo = menus[cmode].enabled[hit] & 2;
						atype = menus[cmode].enabled[hit] & 0xfc;
						if (atype == 4) menus[cmode].enabled[hit] ^= 1;
						do_option(hit);
						print_options();
						break;
					}
				}
			}
		}

		if (viewstatus == 2) 
		{	Delay(200); viewstatus = 99; }
		else if (viewstatus == 1 || viewstatus == 4)
		{	if ((flasher & 16) && viewstatus == 1)
				SetRGB4(&vp_page,31,15,15,15);
			else SetRGB4(&vp_page,31,0,0,0);
			ppick();
			continue;	/* go no further */
		}

		/* interpret controls */
		decode_mouse(); /* what if dead ?? */

		if (menus[GAME].enabled[5] & 1) { Delay(1); continue; }

		if (light_timer) light_timer--;
		if (secret_timer) secret_timer--;
		if (freeze_timer) freeze_timer--;

		fiery_death =
			(map_x>8802 && map_x<13562 && map_y>24744 && map_y<29544);

		inum = anim_list[0].state;
		if (inum == DEAD || inum==FALL)
		{	if (goodfairy == 1) { revive(FALSE); inum = STILL; }
			else if (--goodfairy < 20) ; /* do ressurection effect/glow */
			else if (luck<1 && goodfairy<200) { revive(TRUE); inum = STILL; }
			else if (anim_list[0].state == FALL && goodfairy<200)
			{	revive(FALSE); inum = STILL; }
			else if (goodfairy < 120)
			{	an = anim_list + 3;
				anix = 4;
				an->abs_x = hero_x + goodfairy*2 - 20;
				an->abs_y = hero_y;
				an->type = OBJECTS;
				an->index = 100 + (cycle & 1);
				an->state = STILL;
				an->weapon = an->environ = 0;
				an->race = 0xff;
				actors_on_screen = TRUE;
				battleflag = FALSE;
			}
		}
		else if (inum==DYING || inum==SINK || inum == SLEEP);
		else if (handler_data.qualifier&0x2000||keyfight||(*pia&(128))==0)
		{	dif_x = anim_list[0].vel_x;
			dif_y = anim_list[0].vel_y;
			if (xtype > 80)
			{	if (inum != SHOOT1 && xtype == 81) event(15);
				if (inum != SHOOT1 && xtype == 82) event(16);
				inum = SHOOT1;
			}
			else if (riding==11)
			{	if (fiery_death) event(32);
				else if (dif_x>-15 && dif_x<15 && dif_y>-15 && dif_y<15)
				{	ytest = anim_list[0].abs_y - 14;
					if (proxcheck(anim_list[0].abs_x,ytest,0)==0 &&
						proxcheck(anim_list[0].abs_x,ytest+10,0)==0)
					{	riding = 0;			/* dismount */
						anim_list[0].abs_y = ytest;
					}
				}
				else event(33);
			}
			else
			{	if (oldir < 9) anim_list[0].facing = oldir;
				if (inum >= WALKING)
				{	if (anim_list[0].weapon == 4) inum = SHOOT1;
					else if (anim_list[0].weapon == 5)
					{	if (inum < SHOOT1) inum = SHOOT1;	}
					else inum = FIGHTING;
				}
			}
		}
		else if (inum == SHOOT1) inum = SHOOT3; /* release */
		else	/* walk */
		{	if (oldir < 9)
			{	if (hunger > 120 && !(rand4()))
				{	if (rand()&1) oldir = (oldir + 1 & 7);
					else oldir = (oldir - 1 & 7);
				}
				anim_list[0].facing = oldir;
				if (handler_data.qualifier & 0x4000 || keydir)
					inum = WALKING;
			}
			else inum = STILL;
		}
		anim_list[0].state = inum;

		/* determine the monster and character motion effects */
		raftprox = turtleprox = FALSE;
		if (active_carrier) wcarry = 3; else wcarry = 1;
		xstart = anim_list[0].abs_x - anim_list[wcarry].abs_x - 4;
		ystart = anim_list[0].abs_y - anim_list[wcarry].abs_y - 4;
		if (xstart < 16 && xstart > -16 && ystart < 16 && ystart > -16)
			raftprox = 1;
		if (xstart < 9 && xstart > -9 && ystart < 9 && ystart > -9)
			raftprox = 2;

		if (riding==11) anim_list[0].environ = -2;

		noswan:
		for (i=0; i<anix; i++)
		{	short d, e, s, dex, nvx, nvy, nvx1, nvy1;
			struct missile *ms;
			char k;

			an = &(anim_list[i]);
			if (freeze_timer && i > 0) goto statc; /* what about wizard? */
			k = an->environ;
			d = an->facing;
			s = an->state;
			dex = an->index;
			inum = diroffs[d];
			ms = missile_list + mdex;
			if (an->type == OBJECTS) { j = 0; goto raise; }
			if (an->type == DRAGON)
			{	dex = 0;
				if (s == DYING) dex = 3;
				else if (s == DEAD) dex = 4;
				else if (rand4()==0)	/* and within hostile extent */
				{	ms->speed = 5;
					mdex++; dex = rand2() + 1;
					effect(5,1800 + rand256());
					an->facing = 5;		/* depends on where char is at */
					ms->missile_type = 2;
					goto dragshoot;
				}
				dex = 0;
			}
			else if (an->type == CARRIER)
			{	k = j = 0;
				if (actor_file == 11)
				{	if (raftprox && wcarry == 3 && stuff[5])
					{	d = anim_list[0].facing;
						xtest = an->abs_x = anim_list[0].abs_x; /* swan_x, swan_y; */
						ytest = an->abs_y = anim_list[0].abs_y;
						riding = 11;
					}
					else
					{	xtest = an->abs_x;
						ytest = an->abs_y;
					}
					dex = d;
					e = 0;
				}
				else
				{	if (raftprox && wcarry == 3)
					{	d = anim_list[0].facing;
						xtest = anim_list[0].abs_x;
						ytest = anim_list[0].abs_y;
						riding = 5;
						dex = d+d;
						if (anim_list[0].state == WALKING) dex += (cycle&1);
					}
					else
					{	xtest = newx(an->abs_x,d,3);
						ytest = newy(an->abs_y,d,3);
						if (px_to_im(xtest,ytest) != 5)
						{	d = (d+1)&7;
							xtest = newx(an->abs_x,d,3);
							ytest = newy(an->abs_y,d,3);
							if (px_to_im(xtest,ytest) != 5)
							{	d = (d-2)&7;
								xtest = newx(an->abs_x,d,3);
								ytest = newy(an->abs_y,d,3);
								if (px_to_im(xtest,ytest) != 5)
								{	d = (d-1)&7;
									xtest = newx(an->abs_x,d,3);
									ytest = newy(an->abs_y,d,3);
								}
							}
						}
						riding = FALSE;
						dex = d+d+(cycle&1);
					}
					j = px_to_im(xtest,ytest);
					if (j == 5) { an->abs_x = xtest; an->abs_y = ytest; }
					e = 1;
				}
				move_extent(e,xtest,ytest);
				goto raise;
			}
			else if (an->type == SETFIG)
			{	j = an->race & 0x7f;		/* id# of setfig */
				dex = setfig_table[j].image_base;
				if (s == DYING) { dex += 2; if (j==9) dex += 2; }
				else if (s == DEAD) { dex += 3; if (j==9) dex += 2; }
				else if (j == 9)
				{	set_course(i,hero_x,hero_y,0); dex = an->facing/2; witchflag = TRUE; }
				else if (s == TALKING)
				{	dex += rand2();
					if (!(--(an->tactic))) an->state = STILL;
				}
				else goto statc;
				k = j = 0;
			}
			else if (an->type == RAFT)
			{	riding = FALSE;
				if (wcarry != 1 || raftprox != 2) goto statc;
				xtest = anim_list[0].abs_x;	/* raft follows character */
				ytest = anim_list[0].abs_y;
				/* only if character very nearby?? */
				j = px_to_im(xtest,ytest);
				if (j < 3 || j >5) goto statc;
				an->abs_x = xtest;
				an->abs_y = ytest;
				riding = 1;
				goto statc;
			}
			else if (s == SINK)
			{	if (an->vitality > 0) dex = 83;
				frustflag = 0;
				goto cpx;
			}
			else if (s == WALKING)
			{	if (k == -2)
				{	if (riding == 11) e = 40; else e = 42;
					nvx1 = nvx = an->vel_x + newx(20,d,2)-20;
					nvy1 = nvy = an->vel_y + newy(20,d,2)-20;
					if (nvx1 < 0) nvx1 = -nvx;
					if (nvy1 < 0) nvy1 = -nvy;
					if (nvx1 < e-8) an->vel_x = nvx;
					if (nvy1 < e) an->vel_y = nvy;
					xtest = an->abs_x + an->vel_x/4;
					ytest = an->abs_y + an->vel_y/4;
					if (riding == 11)
					{	set_course(0,-nvx,-nvy,6);
						d = an->facing;
						goto newloc;
					}
					if ((proxcheck(xtest,ytest,i))==0) goto newloc;
					k = 0;
				}
				if (i==0 && riding == 5) e = 3;
				else if (k == -3) e = -2; /* walk backwards */
				else if (k == -1) e = 4;
				else if (k == 2 || k > 6) e = 1; else e = 2;
				xtest = newx(an->abs_x,d,e);
				ytest = newy(an->abs_y,d,e);
				j=proxcheck(xtest,ytest,i);
				if (i==0)
				{	if (j==15) { doorfind(xtest,ytest,0); }
					else bumped = 0;
					if (stuff[30] && j==12) goto newloc;
				}
				if (j) goto checkdev1;
				goto newloc;

				checkdev1:
				d = (d+1)&7;
				xtest = newx(an->abs_x,d,e);
				ytest = newy(an->abs_y,d,e);
				if (proxcheck(xtest,ytest,i)) goto checkdev2;
				goto newloc;

				checkdev2:
				d = (d-2)&7;
				xtest = newx(an->abs_x,d,e);
				ytest = newy(an->abs_y,d,e);
				if (proxcheck(xtest,ytest,i)) goto blocked;

				newloc:

				an->facing = d;
				inum = diroffs[d];
				dex = inum;
				if (!(riding && i==0) && an->race != 2) dex += ((cycle+i)&7);
				if (k == -2) goto cpx;
				if (k == -3) dex ^= 7;

				j = px_to_im(xtest,ytest);

				/* wraiths and snakes immune to water */
				if (an->race == 2 || an->race == 4)	j = 0;
				if (an->race == 4) dex = (cycle&2)+((d & 0xfe)<<2)+0x24;
				if (k > 2)
				{	if ( (j == 0) || ((j == 3) && (k > 5)) ||
						((j == 4) && (k > 10)) ) 
					{	if (hero_sector != 181) k--; goto raise; }
				}
				an->vel_x = ((short)(xtest - an->abs_x))*4;
				an->vel_y = ((short)(ytest - an->abs_y))*4;
				an->abs_x = xtest;
				an->abs_y = ytest;
				frustflag = 0;
			}
			else if (s == STILL)
			{	goto still;
				blocked:
					if (i == 0)
					{	frustflag++;
						if (frustflag > 40) dex = 40;
						else if (frustflag > 20) dex = 84+((cycle>>1)&1);
						goto cpx;
					}
					else an->tactic = FRUST;
				still:
				dex = inum + 1;
				if (k == -2) goto cpx;
				j = px_to_im(an->abs_x,an->abs_y);
			}
			else if (s >= SHOOT1)
			{	/* no shooting underwater or in pax areas */
				if (k > 15 || xtype > 80) goto cpx;
				if (s == SHOOT3)
				{	if (an->weapon == 5)
					{	dex = diroffs[d+8];
						goto cpx;
					}
					dex = diroffs[d+8] + 11;
					an->state = STILL;
					if (i==0) {	if (stuff[8]==0) goto cpx; else stuff[8]--; }
					ms->speed = 3;
					mdex++;
					effect(4,400+rand256());
				}
				else if (s == SHOOT1)
				{	if (an->type == ENEMY) dex = diroffs[d+8] + 11;
					else dex = diroffs[d+8] + 10;
					if (an->weapon == 5)
					{	ms->speed = 5;
						mdex++;
						an->state = SHOOT3;
						dex = diroffs[d+8];
						effect(5,1800+rand256());
					}
					else
					if (i==0 && stuff[8] == 0)
					{	print("No Arrows!"); goto cpx; }
					else ms->speed = 0;
				}
				ms->missile_type = an->weapon-3;
				dragshoot:
				ms->abs_x = an->abs_x + bowshotx[d];
				ms->abs_y = an->abs_y;
				if (an->weapon == 4) ms->abs_y += bowshoty[d];
				else ms->abs_y += gunshoty[d];
				ms->time_of_flight = 0;
	 			ms->direction = an->facing;
				ms->archer = i;
				if (mdex > 5) mdex = 0;
				frustflag = 0;
				goto cpx;
			}
			else if (s < 9)
			{	inum = diroffs[d+8];
				s = an->state = trans_list[s].newstate[rand4()];
				if (i>2 && s==6 || s==7) s = 8;
				dex = s + inum;
				frustflag = 0;
				goto cpx;
			}
			else
			{	if (s == DYING)
				{	if (an->tactic > 4)
					{	if (d == 0 || d > 4) dex = 80; else dex = 81; }
					else if (an->tactic > 0)
					{	if (d == 0 || d > 4) dex = 81; else dex = 80; }
					else { an->state = DEAD; dex = 82; }
					frustflag = 0;
				}
				else if (s==DEAD) dex = 82;
				else if (s==FROZEN) dex = 82;
				else if (s==OSCIL) dex = 84 + (cycle&1);
				else if (s==OSCIL+1) dex = 84;
				else if (s==SLEEP) dex = 86;
				else if (s==FALL)
				{	if (an->tactic >= 30) goto cpx;
					j = (an->tactic/5) + (brother*6);
					dex = fallstates[j];
					an->tactic++;
					an->vel_x = (an->vel_x*3)/4;
					an->vel_y = (an->vel_y*3)/4;
				}
			cpx:
				j = px_to_im(an->abs_x,an->abs_y);
				if (k == -2)
				{	an->abs_x += an->vel_x/4;
					an->abs_y += an->vel_y/4;
				}
			}
			if (s==DYING && !(--(an->tactic)))
			{	an->state = DEAD;
				if (an->race == 0x09)
				{	an->race = 10;
					an->vitality = 10;
					an->state = STILL;
					an->weapon = 0;
					leave_item(i,139); /* leave the talisman */
				}
				if (an->race == 0x89) leave_item(i,27); /* leave the lasso */
			}

			sinker:
			/* j is the Terrain type */
			if (i==0 && raftprox) k = 0; /* can't drown on raft */
			else if (j == 0) k = 0;
			else if (j == 6) k = -1;
			else if (j == 7) k = -2;
			else if (j == 8) k = -3;
			else if (j == 9 && i==0 && xtype == 52)
			{	if (an->state != FALL)
				{	dex = fallstates[brother*6];
					an->state=FALL;
					an->tactic=0;
					luck -= 2; setmood(TRUE);
				}
				k=-2;
			}
			else if (j == 2) k = 2;
			else if (j == 3) k = 5;
			else if (j == 4 || j == 5)
			{	if (j == 4) e = 10; else e = 30;
				if (k > e) k--;
				else if (k < e)
				{	k++;
					if (s == DYING || s == DEAD) ;
					else if (k == 30)
					{	if (hero_sector == 181)
						{	if (i==0)
							{	k=0;
								new_region = 9;
								xfer(0x1080,34950,FALSE);
								find_place(1);
							}
							else an->vitality = 0;
						}
						an->state = STILL;
					}
					else if (k > 15) an->state = SINK;
				}
			}
			if (k==0 && s == SINK) an->state = STILL;
			raise:
			if (MAP_STABLE) an->environ = k;

			k = an->race;
			if (an->type == ENEMY)
			{	if (k==4 && an->state<WALKING) dex=(cycle&1)+diroffs[d];
				else if (k==4 && an->state<DYING) dex=((cycle/2)&1)+diroffs[d];
				else if (k==8)
				{	if (an->state == DEAD) an->abs_x = 0;
					else if (an->state == DYING) dex = 0x3f;
					else
					{	dex = (cycle & 3) * 2;
						if (dex > 4) dex--;
						switch (i % 3)
						{	case 0: dex = 0x25; break;
							case 1: dex += 0x28; break;
							case 2: dex += 0x30; break;
						}
					}
				}
				else if (k==7 && an->vitality == 0)
				{	an->state == STILL;
					dex = 1;
				}
			}
			an->index = dex;

			if (i==0)
			{	if (region_num < 8)
				{	if (an->abs_x < 300) an->abs_x = 32565;
					else if (an->abs_x > 32565) an->abs_x = 300;
					else if (an->abs_y < 300) an->abs_y = 32565;
					else if (an->abs_y > 32565) an->abs_y = 300;
					else goto jkl;
					if (riding > 1)
					{	anim_list[3].abs_x = an->abs_x;
						anim_list[3].abs_y = an->abs_y;
					}
					jkl: ;
				}
				map_adjust(hero_x = an->abs_x,hero_y = an->abs_y);
				safe_flag = j;
			}
			if (an->vitality == 0) goto statc;
			if (fiery_death)
			{	if (i==0 && stuff[23]) an->environ = 0;
				else if (an->environ > 15) an->vitality = 0;
				else if (an->environ > 2) an->vitality--;
				checkdead(i,27);
			}
			if (an->environ == 30 && (cycle&7) == 0)
			{	if (k != 2 && k != 3) { an->vitality--; checkdead(i,6); }
			}
			statc:
			if (an->type == CARRIER && riding == 11)
			{	an->rel_x = wrap(an->abs_x - map_x - 32);
				an->rel_y = wrap(an->abs_y - map_y - 40);
			}
			else if (an->type==RAFT || an->type==CARRIER || an->type==DRAGON)
			{	an->rel_x = wrap(an->abs_x - map_x - 16);
				an->rel_y = wrap(an->abs_y - map_y - 16);
			}
			else
			{	an->rel_x = wrap(an->abs_x - map_x - 8);
				an->rel_y = wrap(an->abs_y - map_y - 26);
			}
		}

		/* check for bed */

		/* update map position */

		if (anim_list[0].state != STILL) sleepwait = 0;
		hero_x = anim_list[0].abs_x;
		hero_y = anim_list[0].abs_y;

		if (region_num == 8)	/* inside buildings */
		{	i = *(mapxy(hero_x>>4,hero_y>>5));
			/* if we're on a sleeping spot */
			if (i == 161 || i == 52 || i == 162 || i == 53)
			{	sleepwait++;
				if (sleepwait == 30)
				{	if (fatigue < 50) event(25);
					else
					{	event(26);
						hero_y = (anim_list[0].abs_y |= 0x1f);
						anim_list[0].state = SLEEP;
					}
				}
			}
			else sleepwait = 0;
		}

		/* check for door */

		i = 0;
		k = DOORCOUNT - 1;

		xtest = hero_x & 0xfff0;
		ytest = hero_y & 0xffe0;
		
		if (riding) goto nodoor3;
		if (region_num < 8)
		{	while (k >= i)
			{	struct door *d;
				j = (k + i)/2;
				d = doorlist + j;

				if (d->xc1 > xtest) k = j-1;
				else if (d->xc1+16 < xtest) i = j+1;
				else if ((d->xc1 < xtest) &&
					((d->type & 1) == 0)) /* door not Horiz. */
						i = j+1;
				else if (d->yc1 > ytest) k = j-1;
				else if (d->yc1 < ytest) i = j+1;
				else 
				{	if (d->type & 1)
					{	if (hero_y & 0x10) goto nodoor; }
					else if ((hero_x & 15) >6 ) goto nodoor;

					if (d->type == DESERT && (stuff[STATBASE]<5)) break;
					if (d->type == CAVE)
					{	xtest = d->xc2 + 24; ytest = d->yc2 + 16; }
					else if (d->type & 1)
					{	xtest = d->xc2 + 16; ytest = d->yc2; }
					else { xtest = d->xc2-1; ytest = d->yc2 + 16; }

					if (d->secs == 1) new_region = 8; else new_region = 9;
					xfer(xtest,ytest,FALSE);
					find_place(2);
					fade_page(100,100,100,TRUE,pagecolors);
				nodoor:	break;
				}
				if (i >= DOORCOUNT || k < 0) break;
			}
		}
		else 
		{	for (j=0; j<DOORCOUNT; j++)
			{	struct door *d; d = &(doorlist[j]);
				if (d->yc2==ytest && 
					( d->xc2==xtest || (d->xc2==xtest-16 && d->type & 1)))
				{	if (d->type & 1)
					{	if ((hero_y & 0x10)==0) goto nodoor2; }
					else if ((hero_x & 15) < 2) goto nodoor2;

					if (d->type == CAVE)
					{	xtest = d->xc1 - 4; ytest = d->yc1 + 16; }
					else if (d->type & 1)
					{	xtest = d->xc1 + 16; ytest = d->yc1 + 34; }
					else { xtest = d->xc1 + 20; ytest = d->yc1 + 16; }

					xfer(xtest,ytest,TRUE);
					find_place(FALSE);
				nodoor2: break;
				}
			}
		}
		nodoor3:

		/* update map position */

		hero_x = anim_list[0].abs_x;
		hero_y = anim_list[0].abs_y;

		/* generate map */

		bm_draw = fp_drawing->ri_page->BitMap;
		planes = bm_draw->Planes;

		OwnBlitter();
		for (i=fp_drawing->obcount; i>0; i--)
		{	shp = &(fp_drawing->shape_queue[i-1]);
			rest_blit(shp->backsave);
		}
		DisownBlitter();

		/* undo witch fx */

		rp_map.BitMap = fp_drawing->ri_page->BitMap;
		if (fp_drawing->wflag) witch_fx(fp_drawing);

		img_x = map_x>>4;
		img_y = map_y>>5;
		dif_x = (img_x - fp_drawing->isv_x);
		dif_y = (img_y - fp_drawing->isv_y);
		if (dif_x & 0x200) dif_x |= 0xfc00;
		else dif_x &= 0x03ff;

		if (MAP_FLUX) load_next();

		if (viewstatus == 99 || viewstatus == 98 || viewstatus == 3)
		{	gen_mini();
			map_draw();
			dif_x = dif_y = 0;
			if (viewstatus == 99) viewstatus = 98;
			else if (viewstatus == 98) viewstatus = 0;
			goto viewchange;
		}
		if (dif_x || dif_y) gen_mini();
		if (dif_x == 1)
		{	if (dif_y == 1) { scrollmap(5); }	/* scroll up left */
			else if (dif_y == 0) { scrollmap(4); } /* scroll left */
			else if (dif_y == -1) { scrollmap(3); } /* scroll down left */
			else { dif_x = dif_y = 0; map_draw(); }
		}
		else if (dif_x == 0)
		{	if (dif_y == 1)	{ scrollmap(6); } /* scroll up */
			else if (dif_y == -1) { scrollmap(2); } /* scroll down */
			else if (dif_y == 0)		/* no motion, delay here */
			{	char battle2;
				ppick();

				viewchange:

				if (anim_list[0].state == SLEEP)
				{	daynight += 63;
					if (fatigue) fatigue--;
					if (fatigue == 0 ||
						(fatigue < 30 && daynight > 9000 && daynight < 10000) ||
						(battleflag && (rand64() == 0)) )
					{	anim_list[0].state = STILL;
						hero_y = (anim_list[0].abs_y &= 0xffe0);
					}
				}
				if (!freeze_timer) /* no time in timestop */
					if ((daynight++) >= 24000) daynight = 0;
				lightlevel = daynight/40;
				if (lightlevel >= 300) lightlevel = 600 - lightlevel;
				if (lightlevel < 40) ob_listg[5].ob_stat = 3;
				else ob_listg[5].ob_stat = 2;
				i = (daynight / 2000);
				if (i != dayperiod)
				{	switch (dayperiod = i) {
					case 0: event(28); break;
					case 4: event(29); break;
					case 6: event(30); break;
					case 9: event(31); break;
					}
				}

				day_fade();

				if ((daynight & 0x3ff) == 0)
				{	if (anim_list[0].vitality < (15+brave/4) && anim_list[0].state != DEAD)
					{	anim_list[0].vitality++;
						prq(4);
					}
				}

				if (freeze_timer) goto stasis;

				find_place(2);

				if (actors_loading == TRUE && CheckDiskIO(8))
					/* CheckIO((struct IORequest *)&diskreqs[8]) ) */
				{	prep(ENEMY); motor_off();
					actors_loading = FALSE;
					anix = 3;
				}
				if ((daynight & 15)==0 && encounter_number && !actors_loading)
				{	mixflag = rand(); if (xtype > 49) mixflag = 0; wt = rand4();
					if ((xtype & 3) == 0) mixflag = 0;
					for (k=1; k<10; k++) /* try to place encounter 10 times */
					{	set_loc();
						if (px_to_im(encounter_x,encounter_y) == 0)
						{	while (encounter_number && anix < 7)
							{	if (set_encounter(anix,63)) anix++;
								encounter_number--;
							}
							for (i=3; (i<7) && encounter_number; i++)
							{	if (anim_list[i].state == DEAD &&
								   (!anim_list[i].visible ||
									anim_list[i].race == 2))
									{	set_encounter(i,63);
										encounter_number--;
									}
							}
							break;
						}
					}
				}
				if ((daynight & 31) == 0 && !actors_on_screen &&
					!actors_loading && !active_carrier && xtype < 50)
				{	if (region_num > 7) danger_level = 5 + xtype;
					else danger_level = 2 + xtype;

					if (rand64() <= danger_level)
					{	encounter_type = rand4();
						if (xtype == 7 && encounter_type == 2)
							encounter_type = 4;
						if (xtype == 8) { encounter_type = 6; mixflag = 0; }
						if (xtype == 49) { encounter_type = 2; mixflag = 0; }
						load_actors();
					}
				}
				k = anim_list[nearest_person].race;
				if (nearest_person && k != last_person)
				{	switch(k) {
					case 0x8d: speak(23); break; /* beggar */
					case 0x89: speak(46); break; /* witch */
					case 0x84: if (ob_list8[9].ob_stat) speak(16); break; /* princess */
					case    9: speak(43); break; /* necromancer */
					case    7: speak(41); break; /* dknight */
					}
					last_person = k;
				}
				actors_on_screen = FALSE;
				leader = 0;
				battle2 = battleflag;
				battleflag = FALSE;
				for (i=2; i<anix; i++) /* skip raft */
				{	short xd, yd, mode, tactic, r;

					if (goodfairy && goodfairy < 120) break;
					an = &(anim_list[i]);					
					if (an->type == CARRIER)
					{	if ((daynight & 15) == 0)
							set_course(i,hero_x,hero_y,5);
						continue;
					}
					else if (an->type == SETFIG) continue;
					mode = an->goal;
					tactic = an->tactic;

					xd = an->abs_x-hero_x;
					yd = an->abs_y-hero_y;
					if (xd < 0) xd = -xd;
					if (yd < 0) yd = -yd;
					if (xd < 300 && yd < 300)
					{	actors_on_screen = TRUE;
						if (anim_list[i].vitality < 1) continue;
						if (an->visible || battle2) battleflag = TRUE;
					}
					r = !bitrand(15);
					if (anim_list[0].state == DEAD || anim_list[0].state == FALL)
					{	if (leader == 0) mode = FLEE;
						else mode = FOLLOWER;
					}
/* special encounters don't flee with 60 xtype */
					else if (an->vitality < 2 ||
							(xtype > 59 && an->race != extn->v3))
								mode = FLEE;
					if (tactic == FRUST || tactic == SHOOTFRUST)
					{	if (an->weapon & 4) do_tactic(i,rand4()+2);
						else do_tactic(i,rand2()+3);
					}
					else if (an->state == SHOOT1) an->state = SHOOT3;
					else if (mode <= ARCHER2) /* hostile character */
					{	char thresh;
						if ((mode & 2) == 0) r = !rand4();
						if (r)
						{	if (an->race==4 && turtle_eggs) tactic = EGG_SEEK;
							else if (an->weapon < 1)
							{	mode = CONFUSED; tactic = RANDOM; }
							else if (an->vitality < 6 && rand2())
								tactic = EVADE;
							else if (mode >= ARCHER1)
							{	if (xd < 40 && yd < 30) tactic = BACKUP;
								else if (xd < 70 && yd < 70) tactic = SHOOT;
								else tactic = PURSUE;
							}
							else tactic = PURSUE;
						}
						thresh = 14 - mode;
						if (an->race == 7) thresh = 16;
						if ((an->weapon & 4)==0 && xd < thresh && yd < thresh)
						{	set_course(i,hero_x,hero_y,0);
							if (an->state >= WALKING) an->state = FIGHTING;
						}
						else if (an->race == 7 && an->vitality)
						{	an->state = STILL; an->facing = 5;	}
						else do_tactic(i,tactic);
					}
					else if (mode == FLEE) { do_tactic(i,BACKUP); }
					else if (mode == FOLLOWER) { do_tactic(i,FOLLOW); }
					else if (mode == STAND)
					{	set_course(i,hero_x,hero_y,0);
						an->state = STILL;
					}
					else if (mode == WAIT) { an->state = STILL; }

					/* no set_course for sinking characters unless swimmable */

					an->goal = mode;
					if (leader == 0) leader = i;
				}
				if ((battle2==0) && battleflag) setmood(1);
				if (battleflag==0 && battle2)
				{	prq(7); prq(4); aftermath(); }
				if ((daynight & 127) == 0 &&
					!actors_on_screen && /* set safe zone */
					!actors_loading && !witchflag &&
					anim_list[0].environ == 0 && safe_flag == 0 &&
					anim_list[0].state != DEAD)
					{	safe_r = region_num; 
						safe_x = hero_x; safe_y = hero_y;
						if (hunger > 30 && stuff[24])
						{	stuff[24]--; hunger -= 30; event(37);  }
					}
				if ((daynight & 7) == 0 && !battleflag) setmood(0);
				if ((daynight & 127) == 0 && anim_list[0].vitality &&
						anim_list[0].state != SLEEP)
				{	hunger++;
					fatigue++;
					if (hunger == 35) event(0);
					else if (hunger == 60) event(1);
					else if ((hunger & 7)==0)
					{	if (anim_list[0].vitality > 5)
						{	if (hunger > 100 || fatigue > 160)
							{	anim_list[0].vitality-=2; prq(4); }
							if (hunger > 90) event(2);
						}
						else if (fatigue > 170)
						{	event(12); anim_list[0].state = SLEEP; }
						else if (hunger > 140)
						{	event(24); hunger = 130;
							anim_list[0].state = SLEEP;
						}
					}
					if (fatigue == 70) event(3);
					else if (hunger == 90) event(4);
				}
				stasis: ;
			}
			else { dif_x = dif_y = 0; map_draw(); }
		}
		else if (dif_x == -1)
		{	if (dif_y == 1) { scrollmap(7); }	/* scroll up right */
			else if (dif_y == 0) { scrollmap(0); } /* scroll right */
			else if (dif_y == -1) { scrollmap(1); } /* scroll down right */
			else { dif_x = dif_y = 0; map_draw(); }
		}
		else { dif_x = dif_y = 0; map_draw(); }

		fp_drawing->isv_x = img_x;
		fp_drawing->isv_y = img_y;

		SetAPen(&rp_map,31);

		for (i=0; i<anix; i++)
		{	short xs,ys,wt,fc,bv,xd,yd;
			if (i > 0 && freeze_timer) break;
			if (i==1 || anim_list[i].state >= WALKING) continue;
			wt = anim_list[i].weapon;
			fc = anim_list[i].facing;
			if (!(wt & 4))
			{	if (wt >= 8) wt = 5; /* tone down touch attack! */
				wt += bitrand(2); 	 /* sometimes extended */
				xs = newx(anim_list[i].abs_x,fc,wt+wt) + rand8()-3;
				ys = newy(anim_list[i].abs_y,fc,wt+wt) + rand8()-3;
				if (i==0) bv = (brave/20)+5; else bv = 2 + rand4(); /* 3 */
				if (bv > 14) bv = 15;		/* bravery limit */
				/* check for sword proximity */
				for (j=0; j<anix; j++)
				{	if (j==1 || j==i || anim_list[j].state == DEAD ||
						anim_list[i].type == CARRIER) continue;
					xd = anim_list[j].abs_x-xs;
					yd = anim_list[j].abs_y-ys;
					if (xd<0) xd = -xd;
					if (yd<0) yd = -yd;
					if (xd > yd) yd = xd; /* the max of them */
					if ((i==0 || rand256()>brave) && yd < bv && !freeze_timer)
					{ dohit(i,j,fc,wt); break; }
					else if ((yd<bv+2) && wt !=5) effect(1,150+rand256());
				}
			}
		}

		if (!freeze_timer) for (i=0; i<6; i++)
		{	short xs,ys,fc,xd,yd,mt,bv,s; struct missile *ms;

			ms = missile_list + i;
			s = ms->speed*2;
			if (ms->missile_type == 0 || ms->missile_type == 3 ||
				s == 0 || ms->time_of_flight++ > 40)
			{	ms->missile_type = 0; continue; }

			j = px_to_im( xs=ms->abs_x , ys=ms->abs_y );
			if (j==1 || j==15) { ms->missile_type = 0; s = 0; }
			fc = ms->direction;
			mt = 6;
			if (ms->missile_type==2) mt = 9;

			for (j=0; j<anix; j++)
			{	if (j==0) bv = brave; else bv = 20; /* really?? */
				if (j == 1 || ms->archer == j || /* don't shoot self!! */
					anim_list[j].state == DEAD ||
					anim_list[j].type == CARRIER) continue;
				xd = anim_list[j].abs_x-xs; yd = anim_list[j].abs_y-ys;
				if (xd<0) xd = -xd;
				if (yd<0) yd = -yd;
				if (xd > yd) yd = xd; /* the max of them */
				if ((i != 0 || bitrand(512)>bv ) && yd < mt)
				{	if (ms->missile_type == 2) dohit(-2,j,fc,rand8()+4);
					else dohit(-1,j,fc,rand8()+4);
					ms->speed = 0;
					if (ms->missile_type == 2) ms->missile_type = 3;
					break;
				}
			}
			ms->abs_x = newx(ms->abs_x,fc,s);
			ms->abs_y = newy(ms->abs_y,fc,s);
		}

		anix2 = anix;
		do_objects();

		/* now stuff the flying missiles */

		for (i=0; i<6; i++)
		{	short t; struct shape *an; struct missile *ms;
			ms = missile_list + i;
			if (!(t = ms->missile_type)) continue;
			xstart = ms->abs_x - map_x - 9;
			ystart = ms->abs_y - map_y - 26;
			if (xstart > -20 && xstart < 340 && ystart > -20 && ystart < 190)
			{	an = anim_list + anix2;
				an->abs_x = ms->abs_x; an->abs_y = ms->abs_y;
				an->rel_x = xstart; an->rel_y = ystart;
				an->type = OBJECTS; an->weapon = 0;
				an->index = ms->direction;
				an->race = 0xff;
				if (t==3) an->index = 0x58;
				else if (t==2) an->index += 0x59;
				anix2++; if (anix2 >= 20) goto drawnow;
			}
		}

		/* now sort the animation list by coordinate */
		/* YUCKY AWFUL WASTEFUL BUBBLE SORT!! YUCK!! */

		drawnow:

		nearest_person = 0;
		perdist = 50;
		for (i=0; i<anix2; i++) anim_index[i] = i;
		for (i=0; i<anix2; i++)
		{	short k1, k2, y1, y2;
			an = anim_list + i;
			if (i && an->type!=OBJECTS && an->state!=DEAD && riding != 11)
			{	j = calc_dist(i,0);
				if (j < perdist) { perdist = j; nearest_person = i; }
			}

			for (j=1; j<anix2; j++)
			{	k1 = anim_index[j-1];
				k2 = anim_index[j];
				y1 = anim_list[k1].abs_y;
				y2 = anim_list[k2].abs_y;
				if (anim_list[k1].state==DEAD || (k2==0 && riding) || k1==1)
					y1 -= 32;
				if (anim_list[k2].state==DEAD || (k1==0 && riding) || k2==1)
					y2 -= 32;
				if (anim_list[k1].environ > 25) y1 += 32;
				if (anim_list[k2].environ > 25) y2 += 32;
				if (y2 < y1)
				{	anim_index[j-1] = k2;
					anim_index[j] = k1;
				}
			}
		}

		/* now that scroll blit is done, finish repairing the map */

		if (dif_x == 1) strip_draw(36); else if (dif_x == -1) strip_draw(0);
		if (dif_y == 1)	row_draw(10); else if (dif_y == -1) row_draw(0);

		/* do witch fx */
		fp_drawing->witchx = anim_list[2].abs_x - (map_x & 0xfff0);
		fp_drawing->witchy = (anim_list[2].abs_y - 15) - (map_y & 0xffe0);
		if (rand4()==0) { if (s1>0) wdir = 1; else wdir = -1; }
		fp_drawing->witchdir = (witchindex += wdir);
		fp_drawing->wflag=witchflag;
		if (witchflag)
		{	witch_fx(fp_drawing);
			if (s1 > 0 && s2 < 0 && (calc_dist(2,0) < 100))
				dohit(-1,0,anim_list[2].facing,rand2()+1);
		}

		fp_drawing->obcount = crack = fp_drawing->saveused = 0;
		backalloc = fp_drawing->backsave;

		for (j=0; j<anix2; j++)
		{	short	ground, cm;
			BYTE	xsize,ysize,xbw,xew,xoff,yoff;
			BYTE	blithigh, blitwide, cwide;
			UBYTE 	xm, ym, ym1, ym2;
			short	xstart1, ystart1, xstop1, ystop1;

			/* passmodes: 0=char, 1=weapon */
			passmode = pass = 0;

			i = anim_index[j];
			an = anim_list + i;
			an->visible = 0;

			newpass:
			inum = an->index;		/* which character */

			/* if offscreen, should we skip?? */

			if (an->weapon>0 && an->weapon<8 && 
				(an->state<DEAD || an->state>=SHOOT1 || xtype>80))
			{	if ((an->facing - 2) & 4) passmode = (pass^1);
				else passmode = pass;
				if (an->weapon == 4 && an->state < SHOOT1)
				{	if ((an->facing & 4) == 0) passmode = (pass ^ 1);
					else passmode = pass;
				}
				pass++;
			}

			xstart = an->rel_x;
			ystart = an->rel_y;
			ground = ystart + 32;	/* location of ground */

			/* as good a place to do this as any? */
			OwnBlitter();
			WaitBlit(); clear_blit(bmask_mem,CBK_SIZE);
			DisownBlitter();

			if (passmode)
			{	k = an->weapon;
				if (k==4 && inum < 32)
				{	xstart += bow_x[inum]; ystart += bow_y[inum]; }
				else
				{	xstart += statelist[inum].wpn_x;
					ystart += statelist[inum].wpn_y;
				}

				if (k == 4 && inum < 32) /* bow */
				{	inum = inum/8;
					if (inum&1) inum = 30;
					else if (inum&2) inum = 0x53;
					else inum = 0x51;
				}
				else if (k == 5)
				{	inum = an->facing + 103;
					if (an->facing == 2) ystart -= 6;
				}
				else /* hand weapons */
				{	if (k == 2) k = 32;
					else if (k == 3) k = 48;
					else if (k == 1) k = 64;
					else if (k == 4) k = 0;
					inum = statelist[inum].wpn_no + k;
				}
				atype = OBJECTS;
			}
			else
			{	atype = an->type;
				if ((atype == ENEMY && an->race != 8) || atype == PHIL)
				{	if (fiery_death && an->environ > 0)
					{	if (an->state == DEAD) goto offscreen;
						else if (an->state == DYING)
							{ atype = OBJECTS; inum = 0x58; goto rte; }
					}
					else if (an->state == FALL)
					{	if (an->tactic<16) atype=ENEMY; else atype=OBJECTS; }
					else inum = statelist[inum].figure;
					if (an->race == 4) { if (an->state<DYING) inum+=0x24; }
					else if (i>0) { if (an->race&1) inum|=1; else inum&=0xfffe; }
				}
				if (atype == OBJECTS && an->race == 0) goto offscreen;
				if (atype == CARRIER && riding == 0 && actor_file == 11)
				{	atype = RAFT; inum = 1; }
			}
			rte:

			xstart += (map_x & 15);
			ystart += (map_y & 31);
			ground += (map_y & 31);

			xsize = (seq_list[atype].width)<<4;
			ysize = seq_list[atype].height;

			/* do shape calculations */

			if (atype == OBJECTS)
			{	if (inum==0x1b || (inum>=8 && inum<=12) || inum==25 || inum == 26 ||
				(inum>0x10 && inum<0x18) || (inum & 128)) ysize = 8;
			}

			/* calculate width in bytes */
			cwide = xsize/8;

			/* calculate stop pixels */
			xstop = xstart + xsize - 1;
			ystop = ystart + ysize - 1;

			if (passmode == 0 && atype != OBJECTS)
			{	if (i==0 && riding==11) ystop -=16;
				else if (an->environ == 2) ystop -= 10;
				else if (an->environ > 29)
				{	if (an->state == DEAD) goto offscreen;
					ystart += 27;
					ystop = ystart + 7;
					atype = OBJECTS;
					inum = 97 + ((cycle+i)&1);
				}
				else if (an->environ > 2)
				{	ystart += an->environ; }
			}
			else
			{	if (an->environ > 29) goto offscreen; /* no weapon */
				if (an->environ > 2)
				{	ystart += an->environ;
					ystop += an->environ;
					if (ystop > ground) ystop = ground;
					if (ystart >= ground) goto offscreen;
					ground += an->environ;
				}
			}

			if (xstart > 319 || ystart > 173 || xstop < 0 || ystop < 0)
				goto offscreen;

			/* clip shape */
			xstart1 = xstart; if (xstart < 0) xstart1 = 0;
			ystart1 = ystart; if (ystart < 0) ystart1 = 0;
			xstop1 = xstop; if (xstop > 319) xstop1 = 319;
			ystop1 = ystop; if (ystop > 173) ystop1 = 173;
			xoff = xstart1 - xstart;
			yoff = ystart1 - ystart;

			if (atype==OBJECTS && (inum&128)) { inum &= 0x7f; yoff += 8; }

			/* calculate word offsets */
			xbw = xstart1>>4;
			xew = xstop1>>4;

			blitwide = xew - xbw + 1;
			blithigh = ystop1 - ystart1 + 1;

			if (xoff & 15) blitwide += 1;

			shp = &fp_drawing->shape_queue[fp_drawing->obcount];
			planesize = seq_list[atype].bytes;
			shp->savesize = blitwide * blithigh * 2;
			shp->blitsize = (blithigh<<6) + blitwide;

			shapedata = seq_list[atype].location + (planesize * 5 * inum);
			mask_buffer = seq_list[atype].maskloc + (planesize * inum);
			if (shp->savesize < 64 && crack < 5)
			{	shp->backsave = planes[crack++] + (192*40); }
			else
			{	shp->backsave = backalloc;
				backalloc += 5 * shp->savesize;
				fp_drawing->saveused += 5 * shp->savesize;
				if (fp_drawing->saveused >= (74*80)) break;
					/* no more blits if no room */
			}

			aoff = ((ystart1)&31) * blitwide * 2;
			boff = (xoff>>4)*2 + cwide*yoff;
			shp->Coff = xbw*2 + ystart1 * 40;
			shift = xstart & 15;
			if (xoff & 15) { shp->Coff -=2; wmask = 0; } else wmask = -1;
			bmod = cwide - (blitwide * 2);
			shp->Cmod = 40 - (blitwide * 2);

			OwnBlitter();
			save_blit(shp->backsave);

				/* if it's an arrow or a weapon */
			if (atype == CARRIER || (i==0 && (riding==11 || xtype == 84
				|| ( hero_x>22833 && hero_x<26428 &&
					 hero_y>26425 && hero_y<26527))))
				goto nomask;
			if (atype == OBJECTS && inum > 99 && inum < 102) goto nomask;
			if (an->race == 0x85 || an->race == 0x87) goto nomask;
			if ((atype == OBJECTS && inum < 8) || passmode) blithigh = 32;
			ym1 = ystart1>>5;
			ym2 = (ystop1>>5) - ym1;
			for (xm=0; xm<blitwide; xm++)
			{	for (ym = 0; ym <= ym2; ym++)
				{	/* ystop = (ystart1 & 31) + blithigh - (ym<<5); */
					ystop = ground - ((ym + ym1)<<5);
					cm = ((xm + xbw) * 6) + ym + ym1;
					cm = minimap[cm] * 4;
					k = terra_mem[cm+1] & 15;
					if (an->state == FALL)
					{	if (cm <= (220*4)) goto skip1;
						else k = 3;
					}
					switch(k)
					{	case 0: goto skip1;
						case 1: if (xm==0) goto skip1; break;
						case 2: if (ystop > 35) goto skip1; break;
						case 3: if (hero_sector==48 && i != 1) goto skip1; /* bridge */
								break;
						case 4: if (xm==0 || ystop > 35) goto skip1; break;
						case 5: if (xm==0 && ystop > 35) goto skip1; break;
						case 6: if (ym!=0) cm = (64*4); break; /* full if above */
						case 7: if (ystop > 20) goto skip1; break;
					}
					maskit(xm,ym,blitwide,terra_mem[cm]);
					skip1: ;
				}
			}
			nomask:

			mask_blit();
			shape_blit();
			an->visible = TRUE;
			DisownBlitter();

			fp_drawing->obcount++;

			offscreen: if (pass == 1) goto newpass;
		}
		WaitBlit();
		fp_drawing->ri_page->RxOffset = (map_x & 15);	/* modulus 16 */
		fp_drawing->ri_page->RyOffset = (map_y & 31);	/* modulus 32 */
		pagechange();
		if (viewstatus == 3) { fade_normal(); viewstatus = 0; }
	}
	stopscore();
quit_all:
	rp_text.BitMap = wb_bmap;
	SetRast(&rp_text,0);
	close_all();
}

/* asm */

xfer(xtest,ytest,flag) register USHORT xtest, ytest, flag;
{	map_x += (xtest-hero_x);
	map_y += (ytest-hero_y);
	hero_x = anim_list[0].abs_x = xtest;
	hero_y = anim_list[0].abs_y = ytest;

	encounter_number = 0;
	if (flag)
	{	xtest = (map_x + 151) >> 8;
		ytest = (map_y + 64) >> 8;
		xtest = (xtest >> 6) & 1;	/* 64 */
		ytest = (ytest >> 5) & 7;	/* 32 */
		new_region = xtest+(ytest+ytest);
	}
	keydir = 0;
	load_all();
	gen_mini();				/* to set xreg and yreg */
	viewstatus = 99;
	setmood(TRUE);
	while (proxcheck(hero_x,hero_y,0)) hero_y++;
}

find_place(flag) short flag;
{	register UBYTE *tbl; char *ms; register long i;

	findagain:
	j = hero_sector = hero_sector & 255;

	((hero_x>>8) - xreg) & 64;
	((hero_y>>8) - yreg) & 32;
	if (region_num > 7)
	{	tbl = inside_tbl; ms = inside_msg; hero_sector += 256; }
	else { tbl = place_tbl; ms = place_msg; }

	for (i=0; i<256; i++)
	{	if (j >= tbl[0] && j <= tbl[1]) break;
		tbl += 3;
	}
	i = tbl[2];
	if (i==4)		/* message number 4 -- the mountains */
	{	if (region_num > 7) ;
		else if (region_num & 1) i=0;
		else if (region_num > 3) i=5;
	}
	if (MAP_FLUX || ((hero_x>>8)-xreg)&64 || ((hero_y>>8)-yreg)&32) i = 0;
	if (i && i != hero_place)
	{	hero_place = i;
		if (flag) msg(ms,i);
	}

	extn = extent_list;
	for (i=0; i<EXT_COUNT; i++)
	{	if (hero_x > extn->x1 && hero_x < extn->x2 &&
			hero_y > extn->y1 && hero_y < extn->y2) break;
		extn++;
	}

	if (xtype != extn->etype)
	{	xtype = extn->etype;
		if (xtype == 83 && ob_list8[9].ob_stat)
		{	rescue(); flag = 0; goto findagain;		}
		else if (xtype >= 60)			/* birds/turtles/special */
		{	if (xtype == 60 || xtype == 61)
			{	if (anim_list[3].race != extn->v3 || anix < 4)
				{	encounter_x = (extn->x1 + extn->x2)/2;
					encounter_y = (extn->y1 + extn->y2)/2;
					goto force;
				}
			}
		}
		else if (xtype == 52)
		{	encounter_type = 8;
			load_actors(); prep(ENEMY); motor_off();
			actors_loading = FALSE;
		}
		else if (xtype >= 50 && flag == 1)
		{	encounter_x = hero_x;
			encounter_y = hero_y;
			force:
			encounter_type = extn->v3;
			mixflag = wt = 0;
			load_actors(); prep(ENEMY); motor_off();
			actors_loading = FALSE;
			encounter_number = extn->v1;
			anix = 3;
			while (encounter_number && (anix < 7))
			{	if (set_encounter(anix,63)) anix++;
				encounter_number--;
			}
		}
	}
	if (xtype < 70) active_carrier = 0;
	else if (xtype == 70 &&
		(!active_carrier || (!riding && actor_file != extn->v3)) )
			load_carrier(extn->v3);
}

load_actors()
{	encounter_number = extn->v1 + rnd(extn->v2);
	if ( actor_file != encounter_chart[encounter_type].file_id)
	{	actor_file = encounter_chart[encounter_type].file_id;
		anix = 3;
		nextshape = seq_list[ENEMY].location;
		read_shapes(actor_file);
		actors_loading = TRUE;
		active_carrier = 0;
	}
}

#define MAX_TRY 15

set_encounter(i,spread) USHORT i, spread;
{	register struct shape *an; USHORT xtest, ytest;
	register long race, w, j;

	an = &(anim_list[i]);
	if (extn->v3==7) { xtest = 21635; ytest = 25762; }
	else for (j=0; j<MAX_TRY; j++)
	{	xtest = encounter_x + bitrand(spread) - (spread/2);
		ytest = encounter_y + bitrand(spread) - (spread/2);
		if (proxcheck(xtest,ytest,i)==0) break;
		if (xtype == 52 && px_to_im(xtest,ytest)==7) break;
	}
	if (j==MAX_TRY) return FALSE;
	an->abs_x = xtest;
	an->abs_y = ytest;
	an->type = ENEMY;
	if ((mixflag & 2) && (encounter_type != 4))
		race = (encounter_type & 0xfffe) + rand2();
	else race = encounter_type;
	an->race = race;
	if (mixflag & 4) wt = rand4();
	w = (encounter_chart[race].arms * 4) + wt;
	an->weapon = weapon_probs[w];
	an->state = STILL;
	an->environ = an->facing = 0;
	if (an->weapon&4) an->goal = ARCHER1 + encounter_chart[race].cleverness;
	else an->goal = ATTACK1 + encounter_chart[race].cleverness;
	an->vitality = encounter_chart[race].hitpoints;
	an->rel_x = an->abs_x - map_x - 8;
	an->rel_y = an->abs_y - map_y - 26;
	return TRUE;
}

checkdead(i,dtype) register long i, dtype;
{	register struct shape *an;
	an = &(anim_list[i]);
	if (an->vitality < 1 && an->state != DYING && an->state != DEAD)
	{	an->vitality = 0; an->tactic = 7;
		an->goal = DEATH; an->state = DYING;
		if (an->race == 7) speak(42);
		else if (an->type == SETFIG && an->race != 0x89) kind -= 3;
		if (i) brave++; else { event(dtype); luck -= 5; setmood(TRUE); }
		if (kind < 0) kind = 0;
		prq(7);
	}
	if (i == 0) prq(4);
}

load_carrier(n) short n;
{	register struct shape *an;
	register long i;
	an = &(anim_list[3]);
	if (n == 10) an->type = DRAGON; else an->type = CARRIER;
	if (n==10) i = 2; else if (n==5) i = 1; else i = 0;
	if (actor_file!=n)
	{	nextshape = seq_list[ENEMY].location;
		read_shapes(n); prep(an->type);
		motor_off();
	}
	an->abs_x = extent_list[i].x1 + 250;
	an->abs_y = extent_list[i].y1 + 200;
	an->index = an->weapon = an->environ = 0;
	an->state = STILL;
	an->vitality = 50;
	anix = 4;
	an->race = actor_file = active_carrier = n;
}

/* this is the death routine for the main character */

struct bro {
	char	brave,luck,kind,wealth;
	UBYTE	*stuff;
} blist[] = 
{	{ 35,20,15,20,julstuff },	/* julian's attributes */
	{ 20,35,15,15,philstuff },	/* phillip's attributes */
	{ 15,20,35,10,kevstuff } };	/* kevin's attributes */

revive(new) short new;
{	/* new tells if this is a new character */
	register struct bro *br;
	register struct shape *an;

	an = &(anim_list[1]);
	an->type = RAFT;
	an->abs_x = 13668; an->abs_y = 14470;
	an->index = an->weapon = an->environ = 0;

	an = &(anim_list[2]);
	an->type = SETFIG;
	an->abs_x = 13668; an->abs_y = 15000;
	an->index = an->weapon = 0;

	an = &(anim_list[0]);
	an->type = PHIL;
	an->goal = USER;	/* yes, we can usurp control from the user */

	handler_data.laydown = handler_data.pickup = 0;
	battleflag = goodfairy = mdex = 0;
	if (new)
	{	stopscore();
		if (brother > 0 && brother < 3)
		{	ob_listg[brother].xc = hero_x;
			ob_listg[brother].yc = hero_y;
			ob_listg[brother].ob_stat = 1;
			ob_listg[brother+2].ob_stat = 3;
		}
		ob_list8[9].ob_stat = 3;
		br = blist + brother;
		brave = br->brave; luck = br->luck; kind = br->kind;
		wealth = br->wealth; stuff = br->stuff;
		brother++;

		for (i=0; i<GOLDBASE; i++) stuff[i] = 0;	/* has no stuff */
		stuff [0] = an->weapon = 1;					/* okay, a dirk, then */

		secret_timer = light_timer = freeze_timer = 0;
		safe_x = 19036; safe_y = 15755; region_num = safe_r = 3;
		map_adjust(safe_x,safe_y);
		viewstatus = 99;
		actors_on_screen = TRUE;
		actors_loading = FALSE;

		map_message();
		SetFont(rp,afont);
		if (brother == 1) 
		{	placard_text(0);
			rp_map.BitMap =  fp_drawing->ri_page->BitMap; SetRast(&rp_map,0);
			rp_map.BitMap =  fp_viewing->ri_page->BitMap;
		}
		else if (brother == 2) placard_text(1);
		else if (brother == 3) placard_text(3);
		else placard_text(5);

		placard(); Delay(120);

		if (brother > 3) { quitflag = TRUE; Delay(500); }
		else if (brother > 1)
		{	Delay(80);
			SetAPen(rp,0); RectFill(rp,13,13,271,107); Delay(10);
			SetAPen(rp,24);
			if (brother == 2) placard_text(2); else placard_text(4);
			Delay(120);
		}

		SetFont(rp,tfont); rp = &rp_text;

		actor_file = 6; set_file = 13; shape_read();

		if (brother < 4)
		{	message_off();
			hero_place = 2;
			event(9);
			if (brother == 1) print_cont(".");
			else if (brother == 2) event(10);
			else if (brother == 3) event(11);
		}	
	}
	else fade_down();

	hero_x = an->abs_x = safe_x;
	hero_y = an->abs_y = safe_y;
	map_x = hero_x - 144;
	map_y = hero_y - 90;
	new_region = safe_r; load_all();
	an->vitality = (15+brave/4);
	an->environ = 0;
	an->state = STILL;
	an->race = -1;
	daynight = 8000; lightlevel = 300;
	hunger = fatigue = 0;
	anix = 3;
	set_options();
	prq(7); prq(4);
	if (brother > 3) viewstatus = 2; else { viewstatus = 3; setmood(TRUE); }
	fiery_death = xtype = 0;
}

screen_size(x) register long x;
{	register long y;

	y = (x*5)/8;

	Delay(2);

	ri_page2.RxOffset = ri_page1.RxOffset = vp_page.DxOffset = 160-x;
	vp_page.DWidth = x+x;

	ri_page2.RyOffset = ri_page1.RyOffset = vp_page.DyOffset = 100-y;
	vp_page.DHeight = y+y;

	vp_text.DxOffset = vp_text.DyOffset = 0;
	vp_text.DHeight = 95-y;

	fade_page(y*2-40,y*2-70,y*2-100,0,introcolors);

	MakeVPort( &v, &vp_text );
	pagechange();
}

setmood(now) char now;
{	register long off;
	if (anim_list[0].vitality == 0) off = (6*4);
	else if (hero_x > 0x2400 && hero_x < 0x3100 &&
			hero_y > 0x8200 && hero_y < 0x8a00)
	{ off = (4*4); /* if (now==1) now=0; */ }
	else if (battleflag) off = 4;
	else if (region_num > 7)
	{	off = (5*4);
		if (region_num == 9) new_wave[10] = 0x0307;
		else new_wave[10] = 0x0100;
	}
	else if (lightlevel > 120) off = 0;
	else off = 8;

	if (menus[GAME].enabled[6] & 1)
	{	if (now)
			playscore(track[off],track[off+1],track[off+2],track[off+3]);
		else setscore(track[off],track[off+1],track[off+2],track[off+3]);
	}
	else stopscore();
}

gen_mini()
{	register unsigned long xr,yr; register long xs, ys;

	/* lregion is what region are supposed to be in */

	if (region_num < 8)
	{	if (MAP_FLUX)
		{	xs = map_x & 0x3fff;
			ys = map_y & 0x1fff;
			if ((ys > 109 && ys < 7936) && (xs > 16156 || xs <16128))
				load_all();
		}
		xs = (map_x + 151)>>8;		/* sector coords of middle screen */
		ys = (map_y + 64)>>8;
		xr = (xs>>6) & 1;			/* region coords */
		yr = (ys>>5) & 3;
		lregion = xr + yr+yr;
		if (lregion != region_num) { new_region = lregion; load_all(); }
	}
	else lregion = region_num;		/* region num > 7 */

	if ((CheckDiskIO(2) && CheckDiskIO(0))
	/* if ((CheckIO((struct IORequest *)&diskreqs[2]) && CheckIO((struct IORequest *)&diskreqs[0])) */
		|| MAP_STABLE)
	{	yr = lregion>>1;
		xr = lregion & 1;
		if (lregion > 7) xr = 0;
		xreg = xr<<6;
		yreg = yr<<5;
	}

	genmini(img_x,img_y);
}

pagechange()
{	register struct fpage *temp;

	temp = fp_drawing; fp_drawing = fp_viewing; fp_viewing = temp;
	vp_page.RasInfo = temp->ri_page;
	v.LOFCprList = temp->savecop;

	MakeVPort( &v, &vp_page );
	MrgCop( &v );
	LoadView(&v);
	temp->savecop = v.LOFCprList;

	WaitBOVP(&vp_text);
}

struct MsgPort *inputDevPort;
struct IOStdReq *inputRequestBlock;
struct Interrupt handlerStuff;

extern struct IOStdReq *CreateStdIO();

extern HandlerInterface();

add_device()
{	SHORT error;

	handler_data.laydown = handler_data.pickup = 0;
	if ((inputDevPort = (struct MsgPort *)CreatePort(0,0)) == NULL) return FALSE;
	inputRequestBlock = CreateStdIO(inputDevPort);
	if(inputRequestBlock == 0) { DeletePort(inputDevPort); return FALSE; }

	handlerStuff.is_Data = (APTR)&handler_data;
	handlerStuff.is_Code = (void (*)())HandlerInterface;
	handlerStuff.is_Node.ln_Pri = 51;

	error = OpenDevice("input.device",0,(struct IORequest *)inputRequestBlock,0);
	if (error) return FALSE;

	inputRequestBlock->io_Command = IND_ADDHANDLER;
	inputRequestBlock->io_Data = (APTR)&handlerStuff;

	DoIO((struct IORequest *)inputRequestBlock);
	return TRUE;
}

wrap_device()
{	inputRequestBlock->io_Command = IND_REMHANDLER;
	inputRequestBlock->io_Data = (APTR)&handlerStuff;
	DoIO((struct IORequest *)inputRequestBlock);
	CloseDevice((struct IORequest *)inputRequestBlock);
	DeleteStdIO(inputRequestBlock);
	DeletePort(inputDevPort);
}

print_options()
{	short i,j,x,y;
	j = 0;
	for (i = 0; i<menus[cmode].num; i++)
	{	x = menus[cmode].enabled[i];
		if ((x&2)==0) continue;

		real_options[j]=i;
		propt(j,(x & 1));
		j++;
		if (j > 11) break;
	}
	SetBPen(&rp_text2,0);
	for (; j < 12; j++)
	{	if (j & 1) x = 482; else x = 430;
		y = ((j/2) * 9) + 8;
		Move(&rp_text2,x,y);
		Text(&rp_text2,"      ",6);
		real_options[j]=-1;
	}
}

propt(j,pena) short j,pena;
{	register long x,y,k,penb;

	k = real_options[j];
	if (cmode==USE) penb=14;
	else if (cmode == FILE) penb = 13;
	else if (k<5) penb = 4;
	else if (cmode==KEYS) penb = keycolors[k-5];
	else if (cmode==SAVEX) penb = k;
	else penb = menus[cmode].color;

	SetAPen(&rp_text2,pena); SetBPen(&rp_text2,penb);

	k = real_options[j] * 5;
	if (j & 1) x = 482; else x = 430;
	y = ((j/2) * 9) + 8;
	Move(&rp_text2,x,y);
	Text(&rp_text2,"      ",6);
	Move(&rp_text2,x+4,y);
	if (cmode >= USE) Text(&rp_text2,menus[cmode].label_list+k,5);
	else if (k<25) Text(&rp_text2,label1+k,5);
	else Text(&rp_text2,menus[cmode].label_list+(k-25),5);
}

long secx, secy;
extern BYTE svflag;

extern UBYTE itrans[];
extern char jtrans[];

LONG	dbg;

do_option(hit) short hit;
{	short dist;
	USHORT y;
	register ULONG i, j, x, k;
	register struct shape *an;
	register struct BitMap *bm;

	switch (cmode) {
	case ITEMS:
		if (hit == 8) gomenu(USE);
		else if (hit == 5)
		{	short n,num,h;
			unsigned char *data;
			bm = fp_drawing->ri_page->BitMap;
			rp_map.BitMap = bm;
			SetRast(&rp_map,0);

			InitBitMap(&pagea,5,16,8000);
			data = seq_list[OBJECTS].location;
			/* all objects are 32 (16 * 1 word) bytes per plane */
			pagea.Planes[0] = data;
			pagea.Planes[1] = data+32;
			pagea.Planes[2] = data+64;
			pagea.Planes[3] = data+96;
			pagea.Planes[4] = data+128;
		
			for (j=0; j<GOLDBASE; j++)
			{	num = stuff[j];
				if (num>inv_list[j].maxshown) num = inv_list[j].maxshown;
				x = inv_list[j].xoff+20;
				y = inv_list[j].yoff;
				n = inv_list[j].image_number*80 + inv_list[j].img_off;
				h = inv_list[j].img_height;
				for (i=0; i<num; i++)
				{	BltBitMap(&pagea,0,n,bm,x,y,16,h,0xC0,0xff,NULL);
					y += inv_list[j].ydelta;
				}
			}

			SetRGB4(&vp_page,31,0,0,0);
			stillscreen();
			LoadRGB4(&vp_page,pagecolors,31);
			viewstatus = 4;
			prq(5);
		}
		else if (hit == 6)	/* find nearest object */
		{	prq(7);
			nearest_fig(0,30);
			stuff[ARROWBASE] = 0;
			if (nearest)
			{	if (anim_list[nearest].type == OBJECTS)
				/* make sure it's not an arrow */
				{	j = anim_list[nearest].index & 0xff;
					x = anim_list[nearest].vitality & 0x7f;
					if (j==0x0d)
					{	announce_treasure("50 gold pieces");
						wealth += 50;
					}
					else if (j == 0x14) /* scrap of paper */
					{	event(17);
						if (region_num > 7) event(19); else event(18);
						goto pickup;
					}
					else if (j==148)
					{	if (hunger < 15) { stuff[24]++; event(36); }
						else eat(30);
						goto pickup;
					}
					/* -- don't take turtle eggs */
					else if (j==102) break;
					else if (j==28)
					{	announce_treasure("his brother's bones.");
						ob_listg[3].ob_stat = ob_listg[4].ob_stat = 0;
						for (k=0; k<GOLDBASE; k++)
						{	if (x==1) stuff[k] += julstuff[k];
							else stuff[k] += philstuff[k];
						}
					}
					else			/* containers - can they be locked?? */
					{	if (j==0x0f) announce_container("a chest");
						else if (j==0x0e) announce_container("a brass urn");
						else if (j==0x10) announce_container("some sacks");
						else if (j==0x1d) break; /* empty chest */
						else
						{	if (j==31) break;
							for (k=0; itrans[k]; k+= 2)
							{	if (j==itrans[k])
								{	i = itrans[k+1];
									stuff[i]++;
									announce_treasure("a ");
									print_cont(inv_list[i].name);
									print_cont(".");
									goto pickup;
								}
							}
							break;
							/* announce_treasure("an unknown thing."); */
							/* goto pickup; */
						}
						k = rand4();
						if (k == 0) print("nothing.");
						else
						{	if (k==1)
							{	i = rand8() + 8;
								if (i==8) i=ARROWBASE;
								print("a ");
								print_cont(inv_list[i].name); print_cont(".");
								stuff[i]++;
							}
							else if (k==2)
							{	i = rand8() + 8;
								if (i==8) { i=(GOLDBASE+3); wealth+=100; }
									else print_cont(" a");
								print(inv_list[i].name);
								print_cont(" and a ");
								do { k = rand8() + 8; } while (k==i);
								if (k==8) k = ARROWBASE;
								print_cont(inv_list[k].name);
								if (i<GOLDBASE) stuff[i]++;
								stuff[k]++;
							}
							else if (k==3)
							{	i = rand8() + 8;
								if (i==8)
								{	print("3 keys.");
									for (k=0; k<3; k++)
									{	i = rand8() + KEYBASE;
										if (i==22) i=16;
										if (i==23) i=20;
										stuff[i]++;
									}
									goto pickup;
								}
								print("3 ");
								print_cont(inv_list[i].name); print_cont("s.");
								stuff[i]+=3;
							}
						}
					}
					pickup:
					change_object(nearest,2);
					stuff[8] += (stuff[ARROWBASE] * 10);
					if (stuff[22])
					{	quitflag = TRUE; viewstatus = 2;
						map_message(); SetFont(rp,afont); win_colors();
					}
				}
				else if (anim_list[nearest].weapon < 0) ;
				else if (anim_list[nearest].vitality == 0 || freeze_timer)
				{	extract("% searched the body and found");
					print("");
					i = anim_list[nearest].weapon; if (i > 5) i = 0;
					if (i)
					{	print_cont("a ");
						print_cont(inv_list[i-1].name);
						stuff[i-1]++;		/* weapon */
						if (i > anim_list[0].weapon) anim_list[0].weapon = i;
						if (i==4) /* a random factor?? */
						{	print_cont(" and ");
							j = rand8()+2; prdec(j,1);
							print_cont(" Arrows.");
							stuff[8] += j;
							anim_list[nearest].weapon = -1;
							break;
						}
					}
					anim_list[nearest].weapon = -1;
					j = anim_list[nearest].race;
					if (j & 0x80) j = 0;	/* setfigs have no treasure */
					else 
					{	j = (encounter_chart[j].treasure * 8) + rand8();
						j = treasure_probs[j];
					}
					if (j)
					{	if (i) print_cont(" and ");
						if (j<GOLDBASE) print_cont("a ");
						print_cont(inv_list[j].name);
						if (j >= GOLDBASE) wealth += inv_list[j].maxshown;
						else stuff[j]++;
					}
					else if (!i) print_cont("nothing");
					print_cont(".");
				}
				else event(35);
			}
			else prq(10);
		}
		else if (hit==7)					/* Look */
		{	long flag = 0;
			an = anim_list;
			for (i=0; i<anix2; i++)
			{	if (an->type==OBJECTS && an->race==0 && (calc_dist(i,0) < 40) )
					change_object(i,flag = 1);
				an++;
			}
			if (flag) event(38); else event(20);
		}
		else if (hit==9) gomenu(GIVE);		/* Give */
		break;
	case MAGIC:
		/* if only I had some Magic! */
		if (hit < 5 || stuff[4+hit] == 0) { event(21); break; }
		if (extn->v3 == 9) { speak(59); break; }
		switch (hit) {
		case 6: light_timer += 760; break;
		case 8: secret_timer += 360; break;
		case 10: if (riding > 1) return; freeze_timer += 100; break;
		case 9:
			if (cheat1==0 && region_num > 7) return;
			bm_draw = fp_drawing->ri_page->BitMap;
			planes = bm_draw->Planes; 
			bigdraw(map_x,map_y);

			i = (hero_x>>4) - ((secx + xreg)<<4) - 4;
			j = (hero_y>>4) - ((secy + yreg)<<4) + 3;
			rp_map.BitMap = bm_draw;
			SetDrMd(&rp_map,JAM1);
			SetAPen(&rp_map,31);
			if (i > 0 && i < 320 && j > 0 && j <143)
			{	Move(&rp_map,i,j); Text(&rp_map,"+",1); }
			viewstatus = 1;
			stillscreen();
			prq(5);
			break;
		case 5:
			if (hero_sector == 144)
			{	if ((hero_x & 255)/85 == 1 && (hero_y & 255)/64 == 1)
				{	short x1, y1;
					x = hero_x>>8; y = hero_y>>8;
					for (i=0; i<11; i++)
					{	if (stone_list[i+i]==x && stone_list[i+i+1]==y)
						{	i+=(anim_list[0].facing+1); if (i>10) i-=11;
							x = (stone_list[i+i]<<8) + (hero_x & 255);
							y = (stone_list[i+i+1]<<8) + (hero_y & 255);
							colorplay();
							xfer(x,y,TRUE);
							if (riding)
							{	anim_list[wcarry].abs_x = anim_list[0].abs_x;
								anim_list[wcarry].abs_y = anim_list[0].abs_y;
							}
							break;
						}
					}
				} else return;
			}
			else return;	/* didn't work so don't decrement use count */
		case 7:			/* HEAL */
			anim_list[0].vitality += rand8()+4;
			if (anim_list[0].vitality > (15+brave/4))
				anim_list[0].vitality = (15+brave/4);
			else print("That feels a lot better!");
			prq(4);
			break;
		case 11:
			an = anim_list + 1;
			for (i=1; i<anix; i++)
			{	if (an->vitality && an->type==ENEMY && an->race < 7)
				{	an->vitality = 0; checkdead(i,0); brave--; }
				an++;
			}
			if (battleflag) event(34);
			break;
		}
		if (!--stuff[4+hit]) set_options();
		break;
	case TALK:
		if (hit == 5) j = nearest_fig(1,100); else j = nearest_fig(1,50);
		if (nearest == 0) break;
		an = anim_list + nearest;
		if (an->state == DEAD) break;
		if (an->type == SETFIG) /* only if talkable */
		{	if (hit == 5 && j < 35) { speak(8); break; }
			k = an->race & 0x7f;
			if (setfig_table[k].can_talk)
			{	an->state = TALKING;
				an->tactic = 15;
			}
			switch (k) {
			case 0: if (kind < 10) speak(35);	/* wizard */
					else speak(27+an->goal); break;
			case 1: /* priest */
					if (stuff[28])
					{	if (ob_listg[10].ob_stat==0)
						{	speak(39); ob_listg[10].ob_stat = 1; }
						else speak(19);
					}
 					else if (kind < 10) speak(40);
					else
					{	speak(36 + (daynight % 3));
						anim_list[0].vitality = (15+brave/4);
						prq(4);
					}
					break;
			case 2: 
			case 3: speak(15); break;	/* guard */
			case 4: if (ob_list8[9].ob_stat) speak(16); break;	/* princess - change speech */
			case 5: if (ob_list8[9].ob_stat) speak(17); break;	/* king - change speech */
			case 6: speak(20); break;	/* noble */
			case 7: /* sorceress */
					if (ob_listg[9].ob_stat)
					{	if (luck<rand64()) luck += 5; }
					else { speak(45); ob_listg[9].ob_stat = 1; }
					prq(7);
					break;
			case 8: if (fatigue < 5) speak(13);
					else if (dayperiod > 7) speak(12);
					else speak(14); break;
			case 9: speak(46); break;	/* witch */
			case 10: speak(47); break;	/* spectre */
			case 11: speak(49); break;	/* ghost */
			case 12: if (region_num == 2) speak(22);	/* ranger */
					 else speak(53+an->goal);
					 break;
			case 13: speak(23); break;	/* beggar */
			}
		}
		else if (an->type == CARRIER && active_carrier == 5)
		{	if (stuff[6]) speak(57); /* check if has shell */
			else { stuff[6] = 1; speak(56); }
		}	
		else if (an->type == ENEMY) { speak(an->race); }
		break;
	case BUY:
		if ((nearest = nearest_person) == 0) break;
		if (anim_list[nearest].race==0x88)
		{	if (hit > 11) return;
			hit = (hit - 5) * 2;
			i = jtrans[hit++]; j = jtrans[hit];
			if (wealth > j)
			{	wealth -= j;
				prq(7);
				if (i==0) { event(22); eat(50); }
				else if (i==8) { stuff[i] += 10; event(23); }
				else
				{	stuff[i]++; extract("% bought a ");
					print_cont(inv_list[i].name); print_cont(".");
				}
			}
			else print("Not enough money!");
		}
		break;
	case GAME: 
		if (hit==6) setmood(TRUE);
		if (hit==8) gomenu(SAVEX);
		if (hit==9) { svflag = FALSE; gomenu(FILE); }
		break;
	case USE:
		if (hit == 7)
		{	if (hitgo) gomenu(KEYS); else extract("% has no keys!");
			return;
		}
		if (hit < 5)
		{	if (hitgo) anim_list[0].weapon = hit+1;
			else extract("% doesn't have one.");
		}
		if (hit == 6 && hitgo)
		{	if (hero_x<21373 && hero_x>11194 && hero_y<16208 && hero_y>10205)
				break;
			get_turtle();
		}
		if (hit == 8 && witchflag) speak(60);
		gomenu(ITEMS);
		break;
	case SAVEX:
		if (hit == 6) quitflag = TRUE;
		if (hit == 5) { svflag = TRUE; gomenu(FILE); }
		break;
	case FILE:
		savegame(hit);
		gomenu(GAME);
		break;
	case KEYS:
		hit -= 5;
		bumped = 0;
		if (stuff[hit+KEYBASE])
		{	for (i=0; i<9; i++)
			{	x = newx(hero_x,i,16);
				y = newy(hero_y,i,16);
				if (doorfind(x,y,hit+1)) { stuff[hit+KEYBASE]--; break; }
			}
			if (i > 8)
			{	extract("% tried a ");
				print_cont(inv_list[KEYBASE+hit].name);
				print_cont(" but it didn't"); print("fit.");
			}
		}
		gomenu(ITEMS);
		break;
	case GIVE:
		if (nearest_person == 0) break;
		k = anim_list[nearest_person].race;
		if (hit == 5 && wealth > 2)
		{	/* give gold to begger */
			wealth -= 2;
			if (rand64()>kind) kind++;
			prq(4); prq(7);
			if (k == 0x8d) speak(24 + anim_list[nearest_person].goal);
			else speak(50);
		}
		else if (hit == 8 && stuff[29]) /* spectre */
		{	if (k != 0x8a) speak(21);
			else { speak(48); stuff[29] = 0; leave_item(nearest_person,140); }
		}
		gomenu(ITEMS);
	}
	set_options();
}

get_turtle()
{	for (i=0; i<25; i++)
	{	set_loc();
		if (px_to_im(encounter_x,encounter_y) == 5) break;
	}
	if (i==25) return;
	move_extent(1,encounter_x,encounter_y);
	load_carrier(5);
}

gomenu(mode) short mode;
{	if (menus[GAME].enabled[5] & 1) return;
	cmode = mode;
	handler_data.lastmenu = 0;
	print_options();
}

set_options()
{	register long i,j;
	for (i=0; i<7; i++)
	{	menus[MAGIC].enabled[i+5] = stuff_flag(i+9);
		menus[USE].enabled[i] = stuff_flag(i);
	}
	j = 8;
	for (i=0; i<6; i++)
	{	if ((menus[KEYS].enabled[i+5] = stuff_flag(i+16))==10) j = 10; }
	menus[USE].enabled[7] = j;
	menus[USE].enabled[8] = stuff_flag(7);	/* sunstone */
	j=8; if (wealth>2) j = 10;
	menus[GIVE].enabled[5] = j; /* gold */
	menus[GIVE].enabled[6] = 8; /* book */
	menus[GIVE].enabled[7] = stuff_flag(28); /* writ */
	menus[GIVE].enabled[8] = stuff_flag(29); /* bone */
}

load_all()
{	while (MAP_FLUX) load_new_region(); }

load_new_region()
{	register struct need *nd; register long i;
	register unsigned char *imem;
	unsigned char *imem0;

	if (MAP_STABLE) return;
	nd = &(file_index[new_region]);
	if (nd->sector != current_loads.sector)
	{
		load_track_range(nd->sector,64,sector_mem,0);
		current_loads.sector = nd->sector;
	}
	if (nd->region != current_loads.region)
	{
		load_track_range(nd->region,8,map_mem,0);
		current_loads.region = nd->region;
	}
	if (nd->terra1 != current_loads.terra1)
	{
		load_track_range(TERRA_BLOCK+nd->terra1,1,terra_mem,1);
		current_loads.terra1 = nd->terra1;
	}
	if (nd->terra2 != current_loads.terra2)
	{
		load_track_range(TERRA_BLOCK+nd->terra2,1,terra_mem+512,2);
		current_loads.terra2 = nd->terra2;
	}
	imem0 = image_mem;
	for (i=0; i<4; i++)
	{	if (nd->image[i] != current_loads.image[i])
		{	imem = imem0;
			load_track_range(nd->image[i]+0, 8,imem,3);
			imem += IPLAN_SZ;
			load_track_range(nd->image[i]+8, 8,imem,4);
			imem += IPLAN_SZ;
			load_track_range(nd->image[i]+16,8,imem,5);
			imem += IPLAN_SZ;
			load_track_range(nd->image[i]+24,8,imem,6);
			imem += IPLAN_SZ;
			load_track_range(nd->image[i]+32,8,imem,7);
			current_loads.image[i] = nd->image[i];
			return;
		}
		imem0 += QPLAN_SZ;
	}

	if (new_region == 4 && stuff[STATBASE] < 5) /* are we in desert sector */
	{	i = ((11*128)+26);
		map_mem[i] = map_mem[i+1] = map_mem[i+128] = map_mem[i+129] = 254;
	}

	for (i=0; i<7; i++)
	{
#if 1
		if (IsReadDiskIO(i)) WaitDiskIO(i);
		InvalidDiskIO(i);
#else
		lastreq = &(diskreqs[i]);
		if (lastreq->iotd_Req.io_Command == CMD_READ) WaitIO((struct IORequest *)lastreq);
		lastreq->iotd_Req.io_Command = CMD_INVALID;
#endif
	}

	motor_off();
	region_num = new_region;
	new_region = NO_REGION;
}

effect(num,speed) short num; long speed;
{	if (menus[GAME].enabled[7] & 1)
	{	playsample(sample[num],sample_size[num]/2,speed); }
}

mod1save()
{	/* save stuff */
	saveload(julstuff,35);
	saveload(philstuff,35);
	saveload(kevstuff,35);
	/* set stuff pointer */
	stuff = blist[brother-1].stuff;

	/* save missile list - mdex already saved */
	saveload((void *)missile_list,6 * sizeof (struct missile));
}
