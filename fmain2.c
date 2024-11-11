#include "exec/types.h"
#include "exec/memory.h"
#include "graphics/gfxbase.h"
#include "graphics/rastport.h"
#include "graphics/clip.h"
#include "graphics/layers.h"
#include "graphics/view.h"
#include "libraries/dos.h"
#include "libraries/dosextens.h"
#include "devices/trackdisk.h"

#include "ftale.h"

#define NO_PROTECT

extern unsigned short xreg, yreg;	/* where the region is */
#asm

; shape structure - anim_list

abs_x		equ		0		; short
abs_y		equ		2
rel_x		equ		4
rel_y		equ		6
type		equ		8
race		equ		9
index		equ		10
visible		equ		11
weapon		equ		12
environ		equ		13
goal		equ		14
tactic		equ		15
state		equ		16
facing		equ		17
vitality	equ		18
vel_x		equ		20
vel_y		equ		21
l_shape		equ		22

#endasm

extern struct shape anim_list[20];
extern short anix, anix2;

extern USHORT hero_x, hero_y, map_x, map_y;

/* motion states */
#define	WALKING	12
#define STILL	13
#define FIGHTING 0
#define DYING 	14
#define DEAD  	15

/* asm */

char com2[9] = { 0,1,2,7,9,3,6,5,4 };

/*set_course(object,target_x,target_y,mode)
	unsigned short object, target_x, target_y, mode;
{	short xdif, ydif, deviation, j;
	register long xabs, yabs, xdir, ydir;
	; */
#asm
com2	dc.b	0,1,2,7,9,3,6,5,4,0

		public	_set_course
_set_course
		movem.l	d0-d7/a0-a1,-(sp)
		move.l	40+4(sp),d2			; object #
		move.l	40+8(sp),d0			; target x
		move.l	40+12(sp),d1		; target_y
		move.l	40+16(sp),d3		; mode

		lea		_anim_list,a1		; start of anim_list
		mulu.w	#l_shape,d2			; object # times length of struct
		add.l	d2,a1				; a1 = start of anim_list[object]

;	if (mode == 6) { xdif = target_x; ydif = target_y; }

		cmp.b	#6,d3				; if mode != 6
		beq.s	1$					; calculate offset normally

;	else
;	{	xdif = anim_list[object].abs_x - target_x;
;		ydif = anim_list[object].abs_y - target_y;
;	}

		neg.w	d0					; d0 = -target_x
		neg.w	d1					; d1 = -target_y
		add.w	abs_x(a1),d0		; d0 = x difference
		add.w	abs_y(a1),d1		; d1 = y difference
1$

;	xabs = yabs = xdir = ydir = 0;

		clr.l	d6					; xdir = 0;
		clr.l	d7					; ydir = 0;

;	if (xdif > 0) { xabs = xdif; xdir = 1; }

		tst.w	d0					; if (xdif > 0)
		beq		3$					; if xdif = 0, do nothing
		bmi		2$					; if xdif < 0
		moveq	#1,d6				; xdir = 1; xabs = xdif
		bra		3$

;	if (xdif < 0) { xabs = -xdif; xdir = -1; }

2$		neg.w	d0					; xabs = -xdif
		moveq	#-1,d6				; xdir = -1;
3$

;	if (ydif > 0) { yabs = ydif; ydir = 1; }

		tst.w	d1					; if (ydif > 0)
		beq		5$					; if ydif = 0, do nothing
		bmi		4$					; if ydif < 0
		moveq	#1,d7				; ydir = 1; yabs = ydif
		bra		5$

;	if (ydif < 0) { yabs = -ydif; ydir = -1; }

4$		neg.w	d1					; yabs = -ydif
		moveq	#-1,d7				; ydir = -1;
5$

;	if (mode != 4)

		cmp.b	#4,d3				; if mode 4
		beq		6$					; then don't do

;	{	if ((xabs>>1) > yabs) ydir = 0;	/* if SMART_SEEK */

		move.w	d0,d4				; copy xabs
		lsr.w	#1,d4				; d4 = xabs >> 1
		cmp.w	d4,d1				; if (xabs>>1) > yabs
		bge.s	55$
		clr.l	d7
55$

;		if ((yabs>>1) > xabs) xdir = 0;

		move.w	d1,d4				; copy yabs
		lsr.w	#1,d4				; d4 = yabs >> 1
		cmp.w	d4,d0				; if (yabs>>1) > xabs
		bge.s	6$
		clr.l	d6					; xdir = 0
6$

;	}

;	deviation = 0;

		clr.l	d4					; d4 = deviation = 0

;	if (mode==1 && (xabs+yabs) < 40) deviation = 1;

		move.w	d0,d5				; d5 = xabs
		add.w	d1,d5				; d5 = xabs + yabs

		cmp.b	#1,d3				; if (mode == 1)
		bne.s	7$
		cmp.w	#40,d5				; and (dist < 40)
		bge.s	7$
		moveq	#1,d4				; deviation = 1;
7$

;	else if (mode==2 && (xabs+yabs) < 30) deviation = 2;

		cmp.b	#2,d3				; if (mode == 2)
		bne.s	8$
		cmp.w	#30,d5				; and (dist < 30)
		bge.s	8$
		moveq	#1,d4				; deviation = 1;
8$
;	else if (mode==3) { xdir = -xdir; ydir = -ydir; }

		cmp.b	#3,d3
		bne.s	81$
		neg.b	d6					; xdir = -xdir;
		neg.b	d7					; ydir = -ydir

;	j = com2[4 - ydir - ydir - ydir - xdir];
81$
		moveq	#4,d5				; d4 = 4
		sub.b	d7,d5
		sub.b	d7,d5
		sub.b	d7,d5
		sub.b	d6,d5				; d4 - 4 - ydir - ydir - ydir - xdir
		lea		com2(pc),a0
		move.b	(a0,d5.w),d5		; d5 = j = com2[d5]
****
;	if (j == 9) anim_list[object].state = STILL;

		cmp.b	#9,d5
		bne.s	9$
		move.b	#13,state(a1)		; #STILL -> anim_list[object].state
		bra		99$
9$

;	else 
;	{	if (rand()&1) j += deviation; else j -= deviation;

		jsr		_rand				; go left or right
		btst	#1,d0				; test a bit
		beq		10$
		add.b	d4,d5				; j += deviation
		bra		11$
10$		sub.b	d4,d5				; j -= deviation

;		anim_list[object].facing = j & 7;

11$		and.b	#7,d5				; and j with 7
		move.b	d5,facing(a1)		; move to facing

;		if (mode != 5) anim_list[object].state = WALKING;

		cmp.b	#5,d3				; if mode != 5
		beq		99$					; move #WALKING to state
		move.b	#12,state(a1)
99$		movem.l	(sp)+,d0-d7/a0-a1
		rts
#endasm

/* a hit, a palpable hit! */
#define FLEE		5	/* run directly away */

extern char *stuff;

dohit(i,j,fc,wt) short wt; register long j,i,fc;
{	if (anim_list[0].weapon < 4 &&
		(anim_list[j].race == 9 ||
			(anim_list[j].race == 0x89 && stuff[7] == 0) ))
	{	speak(58); return; }
	if (anim_list[j].race == 0x8a || anim_list[j].race == 0x8b) return;
	anim_list[j].vitality -= wt;	/* Ow, that hurt! */
	if (anim_list[j].vitality < 0) anim_list[j].vitality = 0;
	if (i==-1) effect(2,500+rand64());
	else if (i==-2) effect(5,3200+bitrand(511));
	else if (j==0) effect(0,800+bitrand(511));
	else effect(3,400+rand256());
		/* push back the foe!! Yar! */
	if (anim_list[j].type != DRAGON && anim_list[j].type != SETFIG
		&& move_figure(j,fc,2) && (i >= 0))
			move_figure(i,fc,2);
	checkdead(j,5);
}

extern short nearest;
extern short xtype;
char	turtle_eggs;

aftermath()
{	register long dead, flee, i, j;
	dead = flee = 0;
	for (i=3; i<anix; i++)
	{	if (anim_list[i].type != ENEMY) ;
		else if (anim_list[i].state == DEAD) dead++;
		else if (anim_list[i].goal == FLEE) flee++;
	}
	if (anim_list[0].vitality < 1) ;	/* if dead, no message */
	else if (anim_list[0].vitality < 5 && dead) print("Bravely done!");
	else if (xtype < 50)
	{	if (dead)	/* foes??? what are their names?? */
		{	print(""); prdec(dead,1);
			print_cont("foes were defeated in battle.");
		}
		if (flee)
		{	print(""); prdec(flee,1);
			print_cont("foes fled in retreat.");
		}
	}
	/* check for presence of turtle eggs */
	if (turtle_eggs) get_turtle();
}

proxcheck(x,y,i) short x,y,i;
{	register long x1, y1;
	register long j;
	if (anim_list[i].type != ENEMY || anim_list[i].race != 2) /* wraith */
	{	x1=prox(x,y);
		if (i==0 && (x1 == 8 || x1 == 9)) x1 = 0;
		if (x1) return x1;
	}

	for (j=0; j<anix; j++)
	{	if (i != j && j != 1 && anim_list[j].type != 5
		&& anim_list[j].state != DEAD) /* can walk over dead and rafts */
		{	x1 = x - anim_list[j].abs_x;
			y1 = y - anim_list[j].abs_y;
			if (x1 < 11 && x1 > -11 && y1 < 9 && y1 > -9) return 16;
		}
	}
	return 0;
}

nearest_fig(constraint,dist) char constraint; short dist;
{	register long d,i;
	nearest = 0;
	for (i=1; i<anix2; i++)
	{	if (anim_list[i].type == OBJECTS &&
			(constraint || anim_list[i].index == 0x1d)) continue;
		d = calc_dist(i,0);
		if (d<dist) { nearest = i; dist = d; }
	}
	return dist;
}

/* asm */

calc_dist(a,b) register long a,b;
{	register long x,y; short dist;
	x = anim_list[a].abs_x - anim_list[b].abs_x; if (x<0) x = -x;
	y = anim_list[a].abs_y - anim_list[b].abs_y; if (y<0) y = -y;
	if (x>(y+y)) return x;
	if (y>(x+x)) return y;
	return (x+y)*5/7;
}

/* asm */

move_figure(fig,dir,dist) short fig,dir,dist;
{	register unsigned short xtest,ytest;
	xtest = newx(anim_list[fig].abs_x,dir,dist);
	ytest = newy(anim_list[fig].abs_y,dir,dist);
	if (proxcheck(xtest,ytest,fig)) return FALSE;
	anim_list[fig].abs_x = xtest;
	anim_list[fig].abs_y = ytest;
	return TRUE;
}

struct compvars {
	UBYTE	xrect, yrect;
	UBYTE	xsize, ysize;
} comptable[10] = {
	{  0, 0, 16,8 },
	{ 16, 0, 16,9 },
	{ 32, 0, 16,8 },
	{ 30, 8, 18,8 },
	{ 32,16, 16,8 },
	{ 16,13, 16,11 },
	{  0,16, 16,8 },
	{  0, 8, 18,8 },
	{  0, 0,  1,1 },
	{  0, 0,  1,1 }
};

extern UBYTE *nhinor, *nhivar;
extern struct BitMap *bm_text, *bm_source;

drawcompass(dir) short dir;
{	register long xr, yr, xs, ys;
	xr = comptable[dir].xrect;
	yr = comptable[dir].yrect;
	xs = comptable[dir].xsize;
	ys = comptable[dir].ysize;
	bm_source->Planes[2] = nhinor;
	BltBitMap(bm_source,0,0,bm_text,567,15,48,24,0xC0,4,NULL);
	if (dir < 9)
	{	bm_source->Planes[2] = nhivar;
		BltBitMap(bm_source,xr,yr,bm_text,567+xr,15+yr,xs,ys,0xC0,4,NULL);
	}
}

USHORT fader[32];

USHORT pagecolors [] = {
	0x0000,0x0FFF,0x0E96,0x0B63,0x0631,0x07BF,0x0333,0x0DB8,
	0x0223,0x0445,0x0889,0x0BBC,0x0521,0x0941,0x0F82,0x0FC7,
	0x0040,0x0070,0x00B0,0x06F6,0x0005,0x0009,0x000D,0x037F,
	0x0C00,0x0F50,0x0FA0,0x0FF6,0x0EB6,0x0EA5,0x000F,0x0BDF };

extern short secret_timer, light_timer, freeze_timer;
struct ViewPort vp_page;
extern UWORD region_num, new_region;

fade_page(r,g,b,limit,colors) short r,g,b,limit; USHORT *colors;
{	register long r1,b1,g1,g2;
	short i;

	if (region_num == 4) pagecolors[31] = 0x0980;
	else if (region_num == 9)
	{	if (secret_timer) pagecolors[31] = 0x00f0;
		else pagecolors[31] = 0x0445;
	}
	else pagecolors[31] = 0x0bdf;

	if (r>100) r = 100;
	if (g>100) g = 100;
	if (b>100) b = 100;
	if (limit)
	{	if (r<10) r=10;			/* night limits */
		if (g<25) g=25;
		if (b<60) b=60;
		g2 = (100-g)/3;
	}
	else
	{	if (r<0) r = 0;
		if (g<0) g = 0;
		if (b<0) b = 0;
		g2 = 0;
	}
	for (i=0; i<32; i++)
	{	r1 = (colors[i] & 0x0f00)>>4;
		g1 = colors[i] & 0x00f0;
		b1 = colors[i] & 0x000f;
		if (light_timer && (r1 < g1)) r1 = g1;
		r1 = (r * r1)/1600;
		g1 = (g * g1)/1600;
		b1 = (b * b1 + (g2*g1))/100;
		if (limit)
		{	if (i>= 16 && i<= 24 && g > 20)
			{	if (g < 50) b1+=2; else if (g < 75) b1++; }
			if (b1 > 15) b1 = 15;
		}
		fader[i]=(r1<<8)+(g1<<4)+b1;
	}
	LoadRGB4(&vp_page,fader,32);
}

extern struct RastPort *rp;

/* asm */

colorplay() /* teleport effect */
{	register long i,j;
	for (j=0; j<32; j++)
	{	for (i=1; i<32; i++) fader[i]=bitrand(0xfff);
		LoadRGB4(&vp_page,fader,32);
		Delay(1);
	}
}

char print_que[32];
short prec=0, pplay=0;

extern short brave, luck, kind, wealth, hero_sector;
SHORT sg1, sg2;

short db1, db2;

ppick()
{	register long p;
	if (prec == pplay) Delay(1);
	else
	{	p = print_que[pplay];
		pplay = (pplay + 1) & 31;
		switch (p) {
		case 2:	print("Coords = "); prdec(hero_x,6); prdec(hero_y,6);
				print("Memory Available: "); prdec(AvailMem(0),6);
				break;
		case 3:	print("You are at: ");
				prdec(hero_x/256,3); prdec(hero_y/256,3);
				print("H/Sector = "); prdec(hero_sector,3);
				text(" Extent = ",10); prdec(xtype,2);
				break;
		case 4: move(245,52);
				text("Vit:",4); prdec(anim_list[0].vitality,3);
				break;
		case 5: print_options(); break;
		case 7:	if (luck < 0) luck = 0;
				move(14,52); text("Brv:",4);prdec(brave,3);
				move(90,52); text("Lck:",4);prdec(luck,3);
				move(168,52); text("Knd:",4);prdec(kind,3);
				move(321,52); text("Wlth:",5);prdec(wealth,3);
				break;
		case 10: print("Take What?"); break;
		}
	}
}

#asm
		public	_prec,_pplay,_print_que,_prq
_prq
		movem.l	d0/d1/a1,-(sp)
		move.w	_prec,d0
		move.w	_pplay,d1
		addq	#1,d0
		and.w	#31,d0
		cmp.w	d0,d1
		beq.s	prqx
		move.w	_prec,d1
		move.w	d0,_prec
		lea		_print_que,a1
		move.b	3+4+12(sp),(a1,d1)
prqx	movem.l	(sp)+,d0/d1/a1
		rts
#endasm

#define TXMIN	16
#define TXMAX	400
#define	TYMIN	5
#define	TYMAX	44

print(str) register char *str;
{	register long l;
	l = 0; while (str[l]) l++;
	ScrollRaster(rp,0,10,TXMIN,TYMIN,TXMAX,TYMAX);
	move(TXMIN,42);
	text(str,l);
}

print_cont(str) register char *str;
{	register long l = 0;
	while (str[l]) l++;
	text(str,l);
}

#define MLEN 35
char *mbuf, mesbuf[200];
extern char *datanames[];
extern short brother;

extract(start) register char *start;
{	register char *ss, *lbreak;
	char *lstart, c; short i;

	lstart = mbuf = mesbuf;
	i = 0;
	while (TRUE)
	{	lbreak = 0;				/* end of current line */
		for ( ; i<37; i++)
		{	c = *start++;
			if (c == ' ') lbreak = mbuf;
			if (c == 0) { lbreak = mbuf; *lbreak = 0; i=1000; }
			if (c == 13) { lbreak = mbuf; *lbreak = 0; break; }
			if (c == '%')
			{	ss = datanames[brother-1];
				while (*ss) { *mbuf++ = *ss++; i++; }
			}
			else *mbuf++ = c;
		}
		if (lbreak)				/* found wrap point */
		{	*lbreak++ = 0;		/* end the line */
			print(lstart);		/* print it */
			if (i > 38) break;
			i = mbuf - lbreak;
			lstart = lbreak;
		}
		else
		{	*mbuf++ = 0;		/* no wrap point so just print it */
			if (i > 38) break;
			print (lstart);
			i = 0;
			lstart = mbuf;
		}
	}
}
			
#asm
;msg(start,num) register char *start; register long num;
;{	while (num) if (*start++ == 0) num--;
;	extract(start);
;}

		public	_event,_speak,_msg,_event_msg,_speeches
_event
		lea		_event_msg,a0
		move.l	4(sp),d0
		bra		msg1
_speak
		lea		_speeches,a0
		move.l	4(sp),d0
		bra		msg1
_msg
		move.l	4(sp),a0
		move.l	8(sp),d0

msg1	beq		msgx
1$		tst.b	(a0)+
		bne.s	1$
		subq.w	#1,d0
		bne.s	1$
msgx	move.l	a0,4(sp)
		bra		_extract

#endasm

announce_container(s) char *s;
{	print(datanames[brother-1]);
	print_cont(" found ");
	print_cont(s);
	print_cont(" containing ");
}

announce_treasure(s) char *s;
{	print(datanames[brother-1]);
	print_cont(" found ");
	print_cont(s);
}

name()
{	print_cont(datanames[brother-1]); }

extern struct RastPort rp_map, rp_text;
extern struct ViewPort vp_page, vp_text;
extern struct BitMap *bm_draw;
extern struct fpage *fp_drawing, *fp_viewing;
extern char	viewstatus;

map_message()
{	fade_down();
	rp = &rp_map;
	rp_map.BitMap =  fp_drawing->ri_page->BitMap;
	SetDrMd(rp,JAM1);
	SetAPen(rp,24);
	SetRast(rp,0);

	vp_text.Modes = HIRES | SPRITES | VP_HIDE;
	stillscreen();
	LoadRGB4(&vp_page,pagecolors,32);
	viewstatus = 2;
}

message_off()
{	fade_down();
	rp = &rp_text;
	vp_text.Modes = HIRES | SPRITES;
	pagechange();
	viewstatus = 3;
}

fade_down()
{	register long i;
	for (i=100; i>=0; i-=5) { fade_page(i,i,i,FALSE,pagecolors); Delay(1); } }

fade_normal()
{	register long i;
	for (i=0; i<=100; i+=5) { fade_page(i,i,i,FALSE,pagecolors); Delay(1); } }

stillscreen()
{	fp_drawing->ri_page->RxOffset = fp_drawing->ri_page->RyOffset = 0;
	pagechange();
}

extern struct seq_info seq_list[];

struct cfile_info {
	UBYTE	width, height, count;	/* size of character set */
	UBYTE	numblocks;				/* number of block to load in */
	UBYTE	seq_num;				/* which seq_list slot to load into */
	USHORT	file_id;
} cfiles[] = {
	{ 1,32,67, 42, PHIL,  1376  }, /* julian */
	{ 1,32,67, 42, PHIL,  1418  }, /* phillip */
	{ 1,32,67, 42, PHIL,  1460  }, /* kevin */
	{ 1,16,116,36,OBJECTS,1312  }, /* objects */
	{ 2,32, 2,  3, RAFT,  1348  }, /* raft */

	{ 2,32,16, 20,CARRIER,1351  }, /* turtle */
	{ 1,32,64, 40, ENEMY,  960  }, /* ogre file */
	{ 1,32,64, 40, ENEMY, 1080  }, /* ghost file */
	{ 1,32,64, 40, ENEMY, 1000  }, /* dknight file (spiders) */
	{ 1,32,64, 40, ENEMY, 1040  }, /* necromancer (farmer / loraii) */

	{ 3,40, 5, 12, DRAGON,1160  }, /* dragon */
	{ 4,64, 8, 40,CARRIER,1120  }, /* bird */
	{ 1,32,64, 40, ENEMY, 1376  }, /* snake and salamander */
	{ 1,32, 8,  5,SETFIG,  936  }, /* wizard/priest */
	{ 1,32, 8,  5,SETFIG,  931  }, /* royal set */

	{ 1,32, 8,  5,SETFIG,  941  }, /* bartender */
	{ 1,32, 8,  5,SETFIG,  946  }, /* witch */
	{ 1,32, 8,  5,SETFIG,  951  }, /* ranger/begger */
};

#define	SHAPE_SZ	(78000)			/* 73K for now */
extern unsigned char *shape_mem, *nextshape;
extern unsigned short safe_r;

extern short actor_file, set_file;

shape_read()
{	nextshape = shape_mem;
	read_shapes(3); prep(OBJECTS);
	read_shapes(brother-1); prep(PHIL);
	read_shapes(4); prep(RAFT);
	seq_list[ENEMY].location = nextshape;
	read_shapes(actor_file); prep(cfiles[actor_file].seq_num);
	read_shapes(set_file); prep(SETFIG);
	new_region = region_num; load_all();
	motor_off();
}

read_shapes(num) register long num;
{	long size; register long slot;

	slot = cfiles[num].seq_num;

	seq_list[slot].bytes = cfiles[num].height * cfiles[num].width * 2;
	size = seq_list[slot].bytes * cfiles[num].count;
	seq_list[slot].location = nextshape;
	seq_list[slot].width = cfiles[num].width;
	seq_list[slot].height = cfiles[num].height;
	seq_list[slot].count = cfiles[num].count;
	if ((nextshape + (size*6)) <= (shape_mem + SHAPE_SZ))
	{	load_track_range(cfiles[num].file_id,
			cfiles[num].numblocks,nextshape,8);
		nextshape += size*5;
		seq_list[slot].maskloc = nextshape;
		nextshape += size;
	}
}

#if 0
extern struct IOExtTD *lastreq, diskreqs[10], *diskreq1;

load_track_range(f_block,b_count,buffer,dr)
short f_block, b_count, dr; APTR buffer;
{	short error;

	lastreq = &(diskreqs[dr]);
	if (lastreq->iotd_Req.io_Command == CMD_READ) WaitIO((struct IORequest *)lastreq);
	*lastreq = *diskreq1;
	lastreq->iotd_Req.io_Length = b_count * 512;
	lastreq->iotd_Req.io_Data = buffer;
	lastreq->iotd_Req.io_Command = CMD_READ;
	lastreq->iotd_Req.io_Offset = f_block * 512;
	SendIO((struct IORequest *)lastreq);
}

motor_off()
{	diskreqs[9] = *diskreq1;
	diskreqs[9].iotd_Req.io_Length = 0;
	diskreqs[9].iotd_Req.io_Command = TD_MOTOR;
	DoIO((struct IORequest *)&diskreqs[9]); 
}
#endif

seekn()
{	cpytest();
	/* diskreqs[9] = *diskreq1 */ ;

/*	diskreqs[9].iotd_Req.io_Length = 512;
	diskreqs[9].iotd_Req.io_Data = (APTR)shape_mem;
	diskreqs[9].iotd_Req.io_Offset = 0;
	diskreqs[9].iotd_Req.io_Command = CMD_READ;
	DoIO(&(diskreqs[9]));
	prot2();
	motor_off(); */
}

prep(slot) short slot;
{
	WaitDiskIO(8); /* WaitIO((struct IORequest *)&diskreqs[8]); */
	InvalidDiskIO(8); /* diskreqs[8].iotd_Req.io_Command = CMD_INVALID; */

	make_mask(seq_list[slot].location,seq_list[slot].maskloc,
		seq_list[slot].width,seq_list[slot].height,seq_list[slot].count);
}

load_next()
{	if (!IsReadLastDiskIO() || CheckLastDiskIO())
	/* if (lastreq->iotd_Req.io_Command != CMD_READ || CheckIO((struct IORequest *)lastreq)) */
		load_new_region();
}

extern unsigned char *(track[32]), *scoremem;

read_score()
{	long file;
	long packlen, sc_load, sc_count;
	register long i;
	sc_load = sc_count = 0;
	if (file = Open("songs",1005))
	{	for (i=0; i<(4*7); i++)
		{	Read(file,&packlen,4);
			if ((packlen*2 + sc_load) > 5900) break;
			track[sc_count] = scoremem + sc_load;
			sc_count++;
			Read(file,scoremem+sc_load,packlen*2);
			sc_load += (packlen*2);
		}
		Close(file);
	}
}

char skipp;
extern struct BitMap pagea, pageb;

copypage(br1,br2,x,y) char *br1, *br2; short x,y;
{	if (skipp) return;
	Delay(350);
	BltBitMap(&pageb,0,0,&pagea,0,0,320,200,0xC0,0x1f,0);
	unpackbrush(br1,&pageb,4,24);
	unpackbrush(br2,&pageb,x,y);
	if (skipint()) return;
	flipscan();
	skipint();
}

char flip1[] = { 8,6,5,4,3,2,3,5,13,0,0, 13,5,3,2,3,4,5,6,8,0,0 };
char flip2[] = { 7,5,4,3,2,1,1,1, 1,0,0,  1,1,1,1,2,3,4,5,7,0,0 };
char flip3[] = {12,9,6,3,0,0,0,0, 0,0,0,  0,0,0,0,0,0,3,6,9,0,0 };

flipscan()
{	short i, scol, h; register long bcol, dcol, rate, wide;
	struct BitMap *temp;
	for (i=0; i<22; i++)
	{	temp = fp_drawing->ri_page->BitMap;
		dcol = 0;
		rate = flip1[i];
		wide = flip2[i];
		if (i<11)
		{	bcol = 161-wide;
			BltBitMap(&pageb,160,0,temp,160,0,135,200,0xC0,0x1f,0);
			if (rate == 0) goto lab1;
			for (scol=wide; scol<136; scol+=rate)
			{	h = page_det(scol);
				BltBitMap(&pagea,bcol+scol,h,temp,161+dcol,h,wide,200-h-h,0xC0,0x1f,0);
				dcol+=wide;
			}
			BltBitMap(&pagea,296,7,temp,161+dcol,h,1,186,0xC0,0x1f,0);
		}
		else
		{	bcol = 160;
			if (rate == 0)
			{	BltBitMap(&pageb,24,0,temp,24,0,135,200,0xC0,0x1f,0);
				goto lab1;
			}
			BltBitMap(&pagea,24,0,temp,24,0,135,200,0xC0,0x1f,0);
			for (scol=wide; scol<136; scol+=rate)
			{	h = page_det(scol);
				dcol+=wide;
				BltBitMap(&pageb,bcol-scol,h,temp,bcol-dcol,h,wide,200-h-h,0xC0,0x1f,0);
			}
			scol = 135; h = 7;
			BltBitMap(&pageb,24,h,temp,159-dcol,h,1,200-h-h,0xC0,0x1f,0);
		}
		lab1: pagechange();
		if (flip3[i]) Delay(flip3[i]);
	}
}

skipint()
{	return skipp = (getkey()==' ');}

/* UBYTE *AllocMem(); */

UBYTE *into_chip(oldpointer,size) register UBYTE *oldpointer; long size;
{	register UBYTE *newpointer; register long i;

	if (TypeOfMem(oldpointer) & MEMF_CHIP) return oldpointer;

	newpointer = AllocMem(size,MEMF_CHIP);
	for (i=0; i<size; i++) newpointer[i] = *oldpointer++;
	return newpointer;
}

char jtrans[] = { 0,3, 8,10, 11,15, 1,30, 2,45, 3,75, 13,20 };

char treasure_probs[] = {
	 0, 0, 0, 0, 0, 0, 0, 0,	/* no treasure */
	 9,11,13,31,31,17,17,32,	/* stone,vial,totem,gold,keys */
	 12,14,20,20,20,31,33,31,	/* keys,skull,gold,nothing */
	 10,10,16,16,11,17,18,19,	/* magic and keys */
	 15,21,0,0,0,0,0,0			/* jade skull and white key */
};

char weapon_probs[] = {
	0,0,0,0,	/* no weapons */
	1,1,1,1,	/* dirks only */
	1,2,1,2,	/* dirks and maces */
	1,2,3,2,	/* mostly maces */
	4,4,3,2,	/* swords and bows */
	5,5,5,5,	/* magic wand */
	8,8,8,8,	/* touch attack! */
	3,3,3,3,	/* swords only */
};

UBYTE fallstates[] = {
	0,0,0,0,0,0,
	0x20,0x22,0x3a,0x6f,0x70,0x71,
	0x24,0x27,0x3c,0x6f,0x70,0x71,
	0x37,0x38,0x3d,0x6f,0x70,0x71 };

char bow_x[32] = {
	1,2,3,4,3,2,1,0, 3,2,0,-2,-3,-2,0,2,
	-3,-3,-3,-3,-3,-3,-3,-2, 0,1,1,1,0,-2,-3,-2 };
char bow_y[32] = { 
	8,8,8,7,8,8,8,8, 11,12,13,13,13,13,13,12,
	8,7,6,5,6,7,8,9, 12,12,12,12,12,12,11,12 };

				  /*  0  1  2  3  4  5  6  7 */
char bowshotx[8] = {  0, 0, 3, 6,-3,-3,-3,-6 };
char bowshoty[8] = { -6,-6,-1, 0, 6, 8, 0,-1 };
char gunshoty[8] = {  2, 0, 4, 7, 9, 4, 7, 8 };

BYTE witchpoints[] = {	/* 64 points - 256 entries */
0,100,0,10, 	9,99,0,9, 		19,98,1,9,		29,95,2,9,
38,92,3,9,		47,88,4,8,		55,83,5,8,		63,77,6,7,
70,70,7,7,		77,63,7,6,		83,55,8,5,		88,47,8,4,
92,38,9,3,		95,29,9,2,		98,19,9,1,		99,9,9,0,
100,0,10,0,		99,-10,9,-1,	98,-20,9,-2,	95,-30,9,-3,
92,-39,9,-4,	88,-48,8,-5,	83,-56,8,-6,	77,-64,7,-7,
70,-71,7,-8,	63,-78,6,-8,	55,-84,5,-9,	47,-89,4,-9,
38,-93,3,-10,	29,-96,2,-10,	19,-99,1,-10,	9,-100,0,-10,
0,-100,0,-10,	-10,-100,-1,-10,-20,-99,-2,-10,	-30,-96,-3,-10,
-39,-93,-4,-10,	-48,-89,-5,-9,	-56,-84,-6,-9,	-64,-78,-7,-8,
-71,-71,-8,-8,	-78,-64,-8,-7,	-84,-56,-9,-6,	-89,-48,-9,-5,
-93,-39,-10,-4,	-96,-30,-10,-3,	-99,-20,-10,-2,	-100,-10,-10,-1,
-100,-1,-10,-1,	-100,9,-10,0,	-99,19,-10,1,	-96,29,-10,2,
-93,38,-10,3,	-89,47,-9,4,	-84,55,-9,5,	-78,63,-8,6,
-71,70,-8,7,	-64,77,-7,7,	-56,83,-6,8,	-48,88,-5,8,
-39,92,-4,9,	-30,95,-3,9,	-20,98,-2,9,	-10,99,-1,9,
};

struct TmpRas mytmp, *InitTmpRas();
struct AreaInfo myAreaInfo;
WORD	areabuffer[20];
PLANEPTR	myras, AllocRaster();

struct Layer *layer, templayer, *oldlayer /*, *CreateUpfrontLayer() */ ;
extern struct Layer_Info *li;
SHORT	s1,s2;

witch_fx(fp) register struct fpage *fp;
{	UBYTE	w1; register BYTE *w;
	register struct RastPort *r;
	SHORT	x1,y1,x2,y2,x3,y3,x4,y4;
	SHORT	xh, yh, dx, dy, dx1, dy1;
	xh = hero_x - (map_x & 0xfff0);
	yh = hero_y - (map_y & 0xffe0);

	layer = CreateUpfrontLayer(li,rp_map.BitMap,0,0,16*19,6*32,
		LAYERSIMPLE,NULL);
	r = &rp_map; oldlayer = r->Layer; r->Layer = layer;

	w1 = ((fp->witchdir+63) * 4); w = witchpoints + w1;
	x1 = w[0] + fp->witchx; y1 = w[1] + fp->witchy;
	x2 = w[2] + fp->witchx; y2 = w[3] + fp->witchy;
	w1 = ((fp->witchdir+1) * 4); w = witchpoints + w1;
	x3 = w[0] + fp->witchx; y3 = w[1] + fp->witchy;
	x4 = w[2] + fp->witchx; y4 = w[3] + fp->witchy;

	dx = x2 - x1; dy = y2 - y1;
	dx1 = xh - x1; dy1 = yh - y1;
	s1 = (dx*dy1-dy*dx1);
	sg1 = 0; if (s1 >= 0) sg1 = 1;

	dx = x4 - x3; dy = y4 - y3;
	dx1 = xh - x3; dy1 = yh - y3;
	s2 = (dx*dy1-dy*dx1);
	sg2 = 0; if (s2 >= 0) sg2 = 1;

	SetDrMd(r,COMPLEMENT);
	if (myras = AllocRaster(100,100))
	{	struct TmpRas *oldtmp; struct AreaInfo *oldarea;
		oldtmp = r->TmpRas;
		oldarea = r->AreaInfo;
		r->TmpRas = InitTmpRas(&mytmp,myras,RASSIZE(100,100));
		InitArea(&myAreaInfo,areabuffer,9);
		r->AreaInfo = &myAreaInfo;
		AreaMove(r,x1,y1);
		AreaDraw(r,x2,y2);
		AreaDraw(r,x4,y4);
		AreaDraw(r,x3,y3);
		AreaEnd(r);
		FreeRaster(myras,100,100);
		r->TmpRas = oldtmp;
		r->AreaInfo = oldarea;
	}
	r->Layer = oldlayer;
	DeleteLayer(NULL,layer);
}

enum obytes {
	QUIVER=11,
	MONEY=13, URN, CHEST, SACKS, G_RING, B_STONE, G_JEWEL,
		SCRAP, C_ORB, VIAL, B_TOTEM, J_SKULL,
	GOLD_KEY=25, GREY_KEY=26,
	FOOTSTOOL=31,
	TURTLE=102,
	BLUE_KEY=114,
	M_WAND=145, MEAL, ROSE, FRUIT, STATUE, BOOK, SHELL,
	GREEN_KEY=153, WHITE_KEY=154, RED_KEY=242, 
};

UBYTE itrans[] = {
	QUIVER,35,
	B_STONE,9,G_JEWEL,10,VIAL,11,C_ORB,12,B_TOTEM,13,G_RING,14,J_SKULL,15,
	M_WAND,4, 27,5, 8,2, 9,1, 12,0, 10,3, ROSE,23, FRUIT,24, STATUE,25,
	BOOK,26, SHELL,6, 155,7, 136,27, 137,28, 138,29, 139,22, 140,30, 
	GOLD_KEY,16,GREEN_KEY,17,BLUE_KEY,18,RED_KEY,19,GREY_KEY,20,WHITE_KEY,21,
	0,0 };

UBYTE rand_treasure[] = {
	SACKS, SACKS, SACKS, SACKS,
	CHEST, MONEY, GOLD_KEY, QUIVER,
	GREY_KEY, GREY_KEY, GREY_KEY, RED_KEY,
	B_TOTEM, VIAL, WHITE_KEY, CHEST
};

#define	TENBLANKS {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}

/* divide object list up by region number */
/* 0=nonexist, 1=ground, 2=inventory, 3=setfig, 4=dead setfig, 5=hidden object */
/* 6 = cabinet items */

struct object				/* 250 objects, for a start */
ob_listg[] = {	/* global objects */
	{     0,    0,0,0},			/* special item (for 'give') */
	{     0,    0,28,0},		/* dead brother 1 - to be filled in later */
	{     0,    0,28,0},		/* dead brother 2 - to be filled in later */
	{ 19316,15747,11,0},		/* ghost brother 1 - to be filled in later */
	{ 18196,15735,11,0},		/* ghost brother 2 - to be filled in later */
	{ 12439,36202,10,3},		/* spectre - can be disabled */
	{ 11092,38526,STATUE,1},	/* gold statues (6 = seahold) */
	{ 25737,10662,STATUE,1},	/* (7 = ogre den) */
	{  2910,39023,STATUE,1},	/* (8 = octal room) */
	{ 12025,37639,STATUE,0},	/* (9 = sorceress) */
	{  6700,33766,STATUE,0},	/* (10 = priest) */
},
ob_list0[] = {	/* snow land objects */
	{ 3340,6735,12,3},		/* ranger west */
	{ 9678,7035,12,3},		/* ranger east */
	{ 4981,6306,12,3},		/* ranger north */
	TENBLANKS
},
ob_list1[] = {	/* maze forest (north) objects */
/*	{23297,5797,TURTLE,1}, */
	{23087,5667,TURTLE,1},
	TENBLANKS
},
ob_list2[] = {	/* swamp land objects */
	{13668,15000,0,3},		/* wizard */
	{10627,13154,0,3},		/* wizard */
	{ 4981,10056,12,3},		/* ranger */
	{13950,11087,SACKS,1},
	{ 10344,36171,SHELL,1},	/* shell */
	TENBLANKS
},
ob_list3[] = {	/* maze forest (south) and manor objects (and tambry) */
	{19298,16128,CHEST,1},
	{18310,15969,13,3}, /* beggar */
	{20033,14401,0,3},	/* wizard */
	{24794,13102,13,3}, /* beggar */
	{ 21626,15446,B_STONE,1},	/* 3 items at stone ring */

	{ 21616,15456,MONEY,1},
	{ 21636,15456,G_RING,1},
	{ 20117,14222,G_JEWEL,1},
	{ 24185,9840,SACKS,1},
	{ 25769,10617,MONEY,1},

	{ 25678,10703,B_STONE,1},
	{ 17177,10599,SCRAP,1},
	TENBLANKS
},
ob_list4[] = {	/* desert objects */
	{ 0,0,0,0},	/* dummy */
	{ 0,0,0,0}, /* dummy */
	{6817,19693,13,3}, /* beggar - must be 3rd object */
	TENBLANKS
},
ob_list5[] = {	/* farm and city objects */
	{22184,21156,13,3}, /* beggar */
	{18734,17595,G_RING,1},
	{21294,22648,CHEST,1},
	{22956,19955,0,3},	/* wizard */
	{28342,22613,0,3},		/* wizard */
	TENBLANKS
},
ob_list6[] = {	/* lava plain objects */
	{24794,13102,13,3}, /* DUMMY OBJECT */
	TENBLANKS
},
ob_list7[] = {	/* southern mountain land objects */
	{23297,5797,TURTLE,1},	/* dummy object */
	TENBLANKS
},
ob_list8[] = {			/* interiors of buildings */
	{ 6700,33756,1,3},	/* priest in chapel */
	{ 5491,33780,5,3},	/* king on throne - never mask */
	{ 5592,33764,6,3},	/* noble */
	{ 5514,33668,2,3},	/* guard */
	{ 5574,33668,2,3},	/* guard */
	{ 8878,38995,0,3},	/* wizard */
	{ 7776,34084,0,3},	/* wizard */
	{ 5514,33881,3,3},	/* guard */
	{ 5574,33881,3,3},	/* guard */
	{ 10853,35656,4,3},	/* princess */
	{ 12037,37614,7,3},	/* sorceress */

	{ 11013,36804,9,3},	/* witch */
	{ 9631,38953,8,3},	/* bartender */
	{ 10191,38953,8,3},	/* bartender */
	{ 10649,38953,8,3},	/* bartender */
	{  2966,33964,8,3},	/* bartender */
	{ 9532,40002,FOOTSTOOL,1},
	{ 6747,33751,FOOTSTOOL,1},
	{ 11410,36169,27+128,1},	/* sunstone */
	{ 9550,39964,B_TOTEM,1}, /* cabinet item */
	{ 9552,39964,B_TOTEM,1},

	{ 9682,39964,B_TOTEM,1}, /* cabinet item */
	{ 9684,39964,B_TOTEM,1},
	{ 9532,40119,B_TOTEM,1}, /* on table */
	{ 9575,39459,URN,1},
	{ 9590,39459,URN,1},
	{ 9605,39459,URN,1},
	{ 9680,39453,VIAL,1},
	{ 9682,39453,VIAL,1},
	{ 9784,39453,VIAL,1},
	{ 9668,39554,CHEST,1},

	{ 11090,39462,MONEY,1},
	{ 11108,39458,B_TOTEM,1},
	{ 11118,39459,B_TOTEM,1},
	{ 11128,39459,B_TOTEM,1},
	{ 11138,39458,B_TOTEM,1},
	{ 11148,39459,B_TOTEM,1},
	{ 11158,39459,B_TOTEM,1},
	{ 11855,36206,FOOTSTOOL,1},
	{ 11909,36198,CHEST,1},
	{ 11918,36246,B_TOTEM,1},	/* cabinet items? */

	{ 11928,36246,B_TOTEM,1},
	{ 11938,36246,B_TOTEM,1},
	{ 12212,38481,CHEST,1},
	{ 11652,38481,RED_KEY,1},
	{ 10427,39977,FOOTSTOOL,1},
	{ 10323,40071,URN,1},
	{ 10059,38472,SACKS,1},
	{ 10344,36171,SHELL,1},	/* shell */
	{ 11936,36207,SCRAP,1},		/* spectre note */
	{  9674,35687,URN,1},
	{  5473,38699,ROSE,1},
	{  7185,34342,FRUIT,1},
 	{  7190,34342,FRUIT,1},
	{  7195,34342,FRUIT,1},
	{  7185,34347,FRUIT,1},
 	{  7190,34347,FRUIT,1},
	{  7195,34347,FRUIT,1},
	{  6593,34085,FRUIT,1},
	{  6598,34085,FRUIT,1},
	{  6593,34090,FRUIT,1},
	{  6598,34090,FRUIT,1},

	/* 'look' items */
	{ 3872,33546,GOLD_KEY,5 },
	{ 3887,33510,B_TOTEM,5 },
	{ 4495,33510,VIAL,5 },
	{ 3327,33383,J_SKULL,5 },
	{ 4221,34119,QUIVER,5 },
	{ 7610,33604,VIAL,5 },
	{ 7616,33522,MONEY,5 },
	{ 9570,35768,B_STONE,5 },
	{ 9668,35769,QUIVER,5 },
	{ 9553,38951,G_RING,5 },
	{ 10062,39005,J_SKULL,5 },
	{ 10577,38951,VIAL,5 },
	{ 11062,39514,MONEY,5 },
	{ 8845,39494,WHITE_KEY,5 },
	{ 6542,39494,G_JEWEL,5 },
	{ 7313,38992,RED_KEY,5 }
},
ob_list9[] = {			/* underground areas */
	{ 7540,38528,M_WAND,1},
	{ 9624,36559,M_WAND,1},
	{ 9624,37459,M_WAND,1},
	{ 8337,36719,M_WAND,1},
	{ 8154,34890,CHEST,1},
	{ 7826,35741,CHEST,1},
	{ 3460,37260,0,3},		/* wizard */
	{ 8485,35725,MONEY,1},
	{ 3723,39340,(128+10),1},	/* king's bone */
},
*ob;

struct object *ob_table[10] = {
	ob_list0, ob_list1,
	ob_list2, ob_list3,
	ob_list4, ob_list5,
	ob_list6, ob_list7,
	ob_list8, ob_list9 };

short	mapobs[10] = { 3, 1, 5,12, 3, 5, 1, 1,61+16, 9 };	/* count */
short	dstobs[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 };	/* distributed */
short	glbobs = 11;

short j1;

do_objects()
{	j1 = 2;
	set_objects(ob_listg,glbobs,0x80);
	set_objects(ob_table[region_num],mapobs[region_num],0);
	if (j1>3) anix = j1;
}

leave_item(i,object) short i,object;
{	ob_listg[0].xc = anim_list[i].abs_x;
	ob_listg[0].yc = anim_list[i].abs_y + 10;
	ob_listg[0].ob_id = object;
	ob_listg[0].ob_stat = 1;
}

/* flag - 1=show, 2=take */

change_object(id,flag) register long id, flag;
{	register struct shape *an;
	register struct object *ob;
	an = anim_list + id;
	id = an->vitality & 0x07f;
	if (an->vitality & 0x080) ob = ob_listg + id;
	else ob = ob_table[region_num] + id;

	if (ob->ob_id==CHEST) ob->ob_id = 0x1d; else ob->ob_stat = flag;
}

extern struct {
	BYTE	cfile_entry,image_base,can_talk;
} setfig_table[];

extern UBYTE *nextshape;
extern char witchflag;

set_objects(list,length,f) struct object *list; short length; long f;
{	short xstart, ystart, i, k;
	BYTE	cf, id;

	db1 = length;
	db2 = region_num;
	turtle_eggs = witchflag = FALSE;
	if (dstobs[region_num] == 0 && new_region >= 10)
	{	register struct object *l2;
		l2 = ob_table[region_num];
		for (i=0; i<10; i++)
		{	do
			{	xstart = bitrand(0x3fff) + ((region_num & 1) * 0x4000);
				ystart = bitrand(0x1fff) + ((region_num & 6) * 0x1000);
			} while (px_to_im(xstart,ystart));
			k = mapobs[region_num]++;
			l2[k].xc = xstart;
			l2[k].yc = ystart;
			l2[k].ob_id = rand_treasure[bitrand(15)];
			l2[k].ob_stat = 1;
		}
		dstobs[region_num] = 1;		/* mark this region as distributed */
	}
	i = 0;
	while (length > 0)
	{	length--;
		if (anix2 >= 20) return;
		xstart = list->xc - map_x - 8;
		ystart = list->yc - map_y - 8;
		if (list->ob_stat == 3 || list->ob_stat == 4)
		{	if (xstart > -100 && xstart < 400 && ystart > -100 && ystart < 300)
			{	id = list->ob_id;
				cf = setfig_table[id].cfile_entry;
				if (cfiles[cf].file_id != cfiles[set_file].file_id)
				{	set_file = cf;
					nextshape = seq_list[SETFIG].location;
					read_shapes(cf);
					prep(SETFIG);
					motor_off();
				}
				if (list->ob_id==9 && list->ob_stat==3) witchflag = TRUE;
			}
			else goto loopend;
		}
		if (list->ob_stat == 2 || list->ob_stat == 0) goto loopend;
		if (xstart > -20 && xstart < 340 && ystart > -20 && ystart < 190)
		{	register struct shape *an;
			if (list->ob_stat == 3 || list->ob_stat == 4)
			{	an = &(anim_list[j1++]);
				if (an->abs_x == list->xc && an->abs_y == list->yc)
				{	if (an->state == DEAD) list->ob_stat = 4;
					goto loopend;
				}
				an->type = SETFIG;
				an->index = setfig_table[id].image_base;
				an->weapon = 0;
				an->vitality = 2+id+id;
				an->goal = i;
				an->environ = 0;
				if (list->ob_stat == 4) an->state = DEAD;
				else an->state = STILL;
				an->race = id + 0x80;
				ystart -= 18;
				if (j1 > anix2) anix2 = j1;
			}
			else
			{	if (list->ob_id == TURTLE) turtle_eggs = anix2;
				an = &(anim_list[anix2++]);
				an->type = OBJECTS;
				an->index = list->ob_id;
				an->weapon = 0;
				an->vitality = i + f;
				if (list->ob_stat == 6) an->race = 2;
				else if (list->ob_stat == 5) an->race = 0;
				else an->race = 1;
			}
			an->abs_x = list->xc;
			an->abs_y = list->yc;
			an->rel_x = xstart;
			an->rel_y = ystart;
		}
		loopend: list++;	/* next object */
				 i++;
	}
}

char answr[10], i;
short xx, yy;
char *answers[] = {
	"LIGHT","HEED","DEED","SIGHT","FLIGHT","CREED","BLIGHT","NIGHT" };

copy_protect_junk()
{	register char key; register long i, j;
	register char *a, *b;
	long h;

	SetDrMd(rp,JAM2);
	for (h=0; h<3; h++)
	{	do { j = rand8(); } while ((a = answers[j])==0);
		SetAPen(rp,1); move(10,125+(h*10)); question(j);
		xx = rp->cp_x; yy = rp->cp_y;
		i = 0;
		cursor(0,4);
		while (TRUE)
		{	key = getkey();
			if (key == '\b' && i > 0) {	i--; cursor(i,4); }
			else if (key == '\r') { cursor(i,0); break; }
			else if (key>=' ' && key<0x7f && i<9)
			{	answr[i++] = key; cursor(i,4); }
			rand();
		}
		b = answr;
#ifndef NO_PROTECT
		while (*a) if (*a++ != *b++) return FALSE;
#endif
		answers[j] = NULL;
	}
	return TRUE;
}

#asm

Move		EQU	$FFFFFF10
Text		EQU	$FFFFFFC4
SetAPen		EQU	$FFFFFEAA
SetBPen		EQU	$FFFFFEA4

			public	_GfxBase

_cursor
			movem.l	a0-a6/d0-d7,-(sp)

			clr.l	d0					; set B pen color = 0
			jsr		SetBPen(a6)

			clr.l	d0					; d0 already clear
			clr.l	d1
			move.w	_xx,d0
			move.w	_yy,d1
			jsr		Move(a6)

			lea		_answr,a0			; string = answr
			move.l	60+4(sp),d0			; length of string = arg 1
			jsr		Text(a6)

			move.l	_rp,a1
			move.l	_GfxBase,a6
			move.l	60+8(sp),d0			; set B pen color = arg 2
			jsr		SetBPen(a6)

			move.w	#$2020,-(sp)		; push spaces

			move.l	sp,a0				; address to print = stack
			moveq	#1,d0
			move.l	_rp,a1
			jsr		Text(a6)			; length = 1

			move.l	_rp,a1
			move.l	_GfxBase,a6
			clr.l	d0					; set B pen color = 0
			jsr		SetBPen(a6)

			move.l	sp,a0				; address to print = stack
			moveq	#1,d0
			jsr		Text(a6)			; length = 1

			addq.l	#2,sp				; pop string from stack

			movem.l	(sp)+,a0-a6/d0-d7
			rts
#endasm

BYTE	svflag;
long	svfile, sverr;
char	savename[] = "df1:A.faery";

extern char endload[2];

extern struct in_work handler_data;
extern struct  TextFont *tfont, *afont;
/* struct FileLock *Lock(), *flock; */
BPTR flock;

locktest(name,access) char *name; long access;
{	flock = Lock(name,access);
	if (flock) UnLock(flock);
	return (int)flock;
}

#define ADDR(ptr) (void *)((int)ptr << 2)

cpytest()
{	BOOL IsHardDrive(void);

	if (IsHardDrive() == FALSE)
	{	struct DeviceList *fdev;
		struct FileLock *fl;

		flock = Lock("df0:",ACCESS_READ);
		if (flock)
		{	fl = ADDR(flock);
			fdev = ADDR(fl->fl_Volume);
#ifndef NO_PROTECT
			if (fdev->dl_VolumeDate.ds_Tick != 230) cold();
#endif
			UnLock(flock);
		}
		return (int)flock;
	}
	else
	{	ULONG	buffer[512/4];

		load_track_range(880,1,buffer,0);
		if (buffer[123] != 230) close_all();
	}
}

#asm

		public	_cold
_cold	jmp		-4

#endasm

waitnewdisk()
{	short i;
	for (i=0; i<300; i++)
	{	if (handler_data.newdisk)
		{	handler_data.newdisk = 0; return TRUE; }
		Delay(5);
	}
	return FALSE;
}

extern short cheat1;
extern char quitflag;

struct extent {
	UWORD x1, y1, x2, y2;
	UBYTE etype, v1, v2, v3;
};

extern struct extent extent_list[1];
extern short encounter_type;
extern char encounter_number, actors_loading;
extern short wt;

extern struct encounter {
	char	hitpoints,
			agressive,
			arms,
			cleverness,
			treasure,
			file_id;
} encounter_chart[5];

savegame(hit) short hit;
{	long	i;
	char	*name;
	BOOL	hdrive = FALSE;

	sverr = 0;

	SetFont(rp,tfont);

stest:
	name = savename;

	if (hdrive == FALSE && locktest("image",ACCESS_READ))
	{	name += 4;
		hdrive = TRUE;
	}
	else if (locktest("df1:",ACCESS_WRITE))
	{	savename[2] = '1';
	}
	else if (locktest("df0:",ACCESS_WRITE) &&
		!locktest("df0:winpic",ACCESS_READ) )
	{	savename[2] = '0';
	}
	else
	{	print("Insert a writable disk in ANY drive.");
		if (waitnewdisk()==0) { print("Aborted."); goto nosave; }
		goto stest;
	}
	savename[4] = 'A' + hit;
	if (svflag) svfile = Open(name,1006);
	else svfile = Open(name,1005);
	if (svfile)
	{	/* save misc variables */
		saveload((void *)&map_x,80);

		/* save region num */
		saveload((void *)&region_num,2);

		/* save anim_list & length */
		saveload((void *)&anix,6);
		saveload((void *)anim_list,anix * (sizeof (struct shape)));

		mod1save();

		/* save extents */
		saveload((void *)extent_list,2 * sizeof (struct extent));

		/* save object lists */
		saveload((void *)ob_listg,glbobs * (sizeof (struct object)));
		saveload((void *)mapobs,20);
		saveload((void *)dstobs,20);
		for (i=0; i<10; i++)
			saveload((void *)ob_table[i],mapobs[i] * (sizeof (struct object)));

		Close(svfile);
	}
	else sverr = IoErr();
	if (sverr)
	{	if (svflag) print("ERROR: Couldn't save game.");
		else print("ERROR: Couldn't load game.");
	}
	if (hdrive == FALSE) while (TRUE)
	{	flock = Lock("df0:winpic",ACCESS_READ);
		if (flock) { UnLock(flock); break; }
		print("Please insert GAME disk.");
		waitnewdisk();
	}
	if (svflag==0)
	{	wt = encounter_number = 0;
		/* actor_file = encounter_chart[encounter_type].file_id; */
		shape_read(); set_options(); viewstatus = 99;
		prq(4); prq(7);
		print(""); print(""); print("");
		encounter_type = actors_loading = 0;
	}
nosave:
	SetFont(rp,afont);
}

saveload(buffer,length) char *buffer; long length;
{	short err;
	if (svflag) err = Write(svfile,buffer,length);
	else err = Read(svfile,buffer,length);
	if (err < 0) sverr = IoErr();
}

move_extent(e,x,y) short e,x,y;
{	register struct extent *ex;
	ex = extent_list + e;
	ex->x1 = x - 250;
	ex->y1 = y - 200;
	ex->x2 = x + 250;
	ex->y2 = y + 200;
}

short sun_colors[] = {
 0x000, 0x000, 0x001, 0x002, 0x002, 0x012, 0x112, 0x113,
 0x113, 0x214, 0x224, 0x225, 0x326, 0x326, 0x337, 0x338,
 0x438, 0x439, 0x449, 0x54A, 0x54B, 0x54B, 0x55C, 0x65C,
 0x65D, 0x66E, 0x76E, 0x76F, 0x86F, 0x97E, 0xA7E, 0xB7E,
 0xB8D, 0xC8D, 0xD8D, 0xD8D, 0xE9C, 0xE9C, 0xE9B, 0xE9B,
 0xFAA, 0xFAA, 0xF99, 0xF98, 0xF98, 0xF97, 0xF86, 0xF85,
 0xF84, 0xF84, 0xF93, 0xF92, 0x76F };

extern short blackcolors[32];

extern short princess;

extern struct TextFont *afont;

rescue()
{	register long i;
	map_message(); SetFont(rp,afont);
	i = princess*3;
	placard_text(8+i); name(); placard_text(9+i); name(); placard_text(10+i);
	placard(); Delay(380);
	SetAPen(rp,0); RectFill(rp,13,13,271,107); Delay(10); SetAPen(rp,24);
	placard_text(17); name(); placard_text(18); Delay(380);
	message_off();

	princess++;
	xfer(5511,33780,0); /* do princess transfer */
	move_extent(0,22205,21231);
	ob_list8[2].ob_id = 4;
	stuff[28] = 1;
	speak(18);
	wealth += 100;
	ob_list8[9].ob_stat = 0;
	for (i=16; i<22; i++) stuff[i] += 3;
}

win_colors()
{	register long i, j;
	placard_text(6); name(); placard_text(7); placard(); Delay(80);
	bm_draw = fp_drawing->ri_page->BitMap;
	unpackbrush("winpic",bm_draw,0,0);
	LoadRGB4(&vp_page,(void *)blackcolors,32);
	LoadRGB4(&vp_text,(void *)blackcolors,32);
	vp_text.Modes = HIRES | SPRITES | VP_HIDE;
	screen_size(156);
	for (i=25; i> -30; i--)
	{	fader[0] = fader[31] = 0;
		fader[1] = fader[28] = 0xfff;
		for (j=2; j<28; j++)
		{	if (i+j > 0) fader[j] = sun_colors[i+j];
			else fader[j] = 0;
		}
		if (i > -14)
		{	fader[29] = 0x800;
			fader[30] = 0x400;
		}
		else
		{	j = (i+30)/2;
			fader[29] = 0x100 * j;
			fader[30] = 0x100 * (j/2);
		}
		LoadRGB4(&vp_page,fader,32);
		if (i==25) Delay(60);
		Delay(9);
	}
	Delay(30);
	LoadRGB4(&vp_page,(void *)blackcolors,32);
}

#asm
		public	_stuff_flag,_stuff
_stuff_flag
		moveq	#8,d0
		move.l	_stuff,a0
		add.l	4(sp),a0
		tst.b	(a0)
		beq.s	1$
		moveq	#10,d0
1$		rts
#endasm

extern USHORT daynight, lightlevel;
extern short light_timer;

day_fade()
{	register long ll;
	if (light_timer) ll = 200; else ll = 0;
	if ((daynight & 3) == 0 || viewstatus > 97)
		if (region_num < 8) /* no night inside buildings */
			 fade_page(lightlevel-80+ll,lightlevel-61,lightlevel-62,TRUE,pagecolors);
		else fade_page(100,100,100,TRUE,pagecolors);
}

extern short leader;

do_tactic(i,tactic) register long i,tactic;
{	register long r, f; register struct shape *an;
	r = !(rand()&7);
	an = &(anim_list[i]);
	an->tactic = tactic;
	if (an->goal == ATTACK2) r = !(rand()&3);
	if (tactic == PURSUE) { if (r) set_course(i,hero_x,hero_y,0); }
	else if (tactic == SHOOT)
	{	short xd,yd;
		xd = an->abs_x - anim_list[0].abs_x;
		yd = an->abs_y - anim_list[0].abs_y;
		if (xd < 0) xd = -xd;
		if (yd < 0) yd = -yd;
		if ((rand()&1) && 
			(xd < 8 || yd < 8 || ( xd>(yd-5) && xd<(yd+7) ) ))
		{	set_course(i,hero_x,hero_y,5);
			if (an->state < SHOOT1) an->state = SHOOT1;
		}
		else set_course(i,hero_x,hero_y,0);
	}
	else if (tactic == RANDOM)
	{	if (r) an->facing = rand()&7; an->state = WALKING; }
	else if (tactic == BUMBLE_SEEK) { if (r) set_course(i,hero_x,hero_y,4); }
	else if (tactic == BACKUP) { if (r) set_course(i,hero_x,hero_y,3); }
	else if (tactic == FOLLOW)
	{	f = leader; /* follow the leader, of course! */
		if (i == f) an->tactic = RANDOM; /* can't follow self!! */
		if (r) set_course(i,anim_list[f].abs_x,anim_list[f].abs_y+20,0);
	}
	else if (tactic == EVADE)
	{	if (i == anix) f = i-1; else f = i+i;
		if (r) set_course(i,anim_list[f].abs_x,anim_list[f].abs_y+20,2);
	}
	else if (tactic == EGG_SEEK)
/*	{	if (r) set_course(i,23297,5797,0); an->state = WALKING; } */
	{	if (r) set_course(i,23087,5667,0); an->state = WALKING; }
}

extern short hunger;

eat(amt)
{	hunger -= amt;
	if (hunger < 0) { hunger = 0; event(13); }
	else print("Yum!");
}

short minimap[120];

extern short encounter_x, encounter_y;

set_loc()
{	register long d,j;
	j = rand8();			/* direction */
	d = 150 + rand64();		/* distance */
	encounter_x = newx(hero_x,j,d);
	encounter_y = newy(hero_y,j,d);
}



