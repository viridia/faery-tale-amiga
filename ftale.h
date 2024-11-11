/* include file for faery tale adventures - by Talin */

#include "fmain.p"
#include "fmain2.p"
#include "iffsubs.p"

#define free_chip(new,old,size) if (new!=old) FreeMem(new,size);

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
#define DOOR_SEEK	11	/* dknight blocking door */
#define DOOR_LET	12	/* dknight letting pass */

struct shape {
	unsigned short	abs_x, abs_y, rel_x, rel_y;
	char	type;
	UBYTE	race;
	char	index,visible, 	/* image index and on-screen flag */
			weapon,			/* type of weapon carried */
			environ,		/* environment variable */
			goal,tactic,	/* current goal mode and means to carry it out */
			state, facing;	/* current movement state and facing */
	short	vitality;		/* also original object number */
	char	vel_x,vel_y;	/* velocity for slippery areas */
/*	APTR	source_struct;	*/ /* address of generating structure */
};

struct fpage {
	struct RasInfo *ri_page;
	struct cprlist *savecop;
	long isv_x, isv_y;
	short	obcount;
	struct sshape *shape_queue;
	unsigned char *backsave;
	long	saveused;
	short	witchx, witchy, witchdir, wflag;	/* for erasure */
};

struct seq_info {
	short width, height, count;	/* this part loaded in */
	unsigned char  *location, *maskloc;
	short bytes;				/* this part calculated */
	short current_file;
};

enum sequences {PHIL, OBJECTS, ENEMY, RAFT, SETFIG, CARRIER, DRAGON};

struct object {				/* 250 objects, for a start */
	unsigned short	xc, yc;
	char	ob_id, ob_stat;
};

struct inv_item {
	UBYTE	image_number;	/* what image number to use */
	UBYTE	xoff,yoff;		/* x and y offset on image screen */
	UBYTE	ydelta;			/* y increment value */
	UBYTE	img_off,img_height; /* what part of the image to draw */
	UBYTE	maxshown;		/* maximum number that can be shown */
	char *name;
};

struct need {
	USHORT image[4], terra1, terra2, sector, region, setchar;
};

struct	in_work {			/* input handler data area */
	short	xsprite, ysprite;
	short	qualifier;		/* input qualifier */
	UBYTE	laydown,pickup;
	char	newdisk, lastmenu;
	struct GfxBase *gbase;
	struct SimpleSprite *pbase;
	struct ViewPort *vbase;
	unsigned char keybuf[128];
	short	ticker;
};


