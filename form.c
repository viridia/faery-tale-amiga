/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *												 *
 *                 edform -------- Created: Mar 22 1986		 *
 *                             By Talin				 *
 *												 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* #include "exec/types.h"
#include "functions.h"
#include "graphics/gfx.h"
#include "graphics/gfxbase.h"
#include "intuition/intuition.h" */
#include "ctype.h"

#include "devices/console.h"
#include "devices/keymap.h"

#define	SETFN(n)	openflags |= n
#define TSTFN(n)	openflags & n

struct Library *IntuitionBase, *GfxBase;
struct Window *Window;
struct Screen *Screen;

struct NewScreen NewScreen =
   { 0,0,640,200, 3, 0,1, HIRES, CUSTOMSCREEN,NULL,NULL,NULL, };

struct NewWindow back_ground =
{	0,0,640,200, 0,1,
	RAWKEY | MENUPICK | MOUSEBUTTONS,
	NOCAREREFRESH | ACTIVATE | SMART_REFRESH | WINDOWDEPTH | 
		BORDERLESS | BACKDROP,
	NULL,NULL,NULL,NULL,NULL,640,200,640,200,CUSTOMSCREEN,
};


struct IOStdReq ConsoleReq;
struct Device *ConsoleDevice;
unsigned char  console_buf[255];
short length;
/* struct MsgPort *writeport; */

struct InputEvent key_event;
struct IntuiMessage *message;
long 	class;
short	code, mx, my;
ULONG	secs, mics;
USHORT	qualifier;
APTR 	address;
char	clicktimes;

short	pick_x, pick_y, old_pick_x, old_pick_y;
ULONG	pick_secs, pick_mics;

struct	RastPort *rp;
struct  ViewPort *vp;
UWORD	openflags;

/* program specific variables */

/* text buffer address */

char	*memstart;
long	memsize;

main()
{	memsize = 20000;	/* allocate 20K bytes */

	if (open_all()) close_all();

	test();
	while (1) wait_event();
/*	close_all(); */
}

open_all()
{	openflags = 0;
	NewScreen.DefaultTitle = (UBYTE *)"Talin's Program";
/*	voxx_tx.IText = (UBYTE *)"By David Joiner";
	mapp_tx.IText = (UBYTE *)"Edit Timbre";
	quit_tx.IText = (UBYTE *)"Quit";
	menu1.MenuName = (BYTE *)"Control"; */

	if ((IntuitionBase = OpenLibrary("intuition.library",0L)) == NULL)
		return 1;
	SETFN(0x01);
	if ((GfxBase = OpenLibrary("graphics.library",0L)) == NULL) return 2;
	SETFN(0x02);
	if ((Screen = OpenScreen(&NewScreen)) == NULL) return 3;
	vp = &(Screen->ViewPort);
	back_ground.Screen = Screen;
/*	textplane = (char *)Screen->BitMap.Planes[0]+240;
	highplane = (char *)Screen->BitMap.Planes[1]+240; */
	SETFN(0x80);
	if ((Window = OpenWindow(&back_ground)) == NULL) return 4;
	rp = Window->RPort; SETFN(0x04);
/*	SetMenuStrip(Window,&menu1); */ SETFN(0x08);

	ConsoleReq.io_Data = (APTR) Window;
	ConsoleReq.io_Length = sizeof (char *);
	if ( OpenDevice("console.device",0L,&ConsoleReq,0L) != 0) return 5;
	ConsoleDevice=ConsoleReq.io_Device;
	SETFN(0x10);

/*	if ((writeport = CreatePort(0,0)) == NULL) return 6;
	SETFN(0x20);
	ConsoleReq.io_Message.mn_Node.ln_Type = NT_MESSAGE;
	ConsoleReq.io_Message.mn_Node.ln_Pri = 0;
	ConsoleReq.io_Message.mn_ReplyPort = writeport; */

	if ( (memstart = AllocMem(memsize,0)) == NULL) return 7;
	SETFN(0x40);

	return 0;
}

close_all()
{	if (TSTFN(0x40)) FreeMem(memstart,memsize);
/*	if (TSTFN(0x20)) DeletePort(writeport); */
	if (TSTFN(0x10)) CloseDevice(&ConsoleReq);
	if (TSTFN(0x08)) ClearMenuStrip(Window);
	if (TSTFN(0x04)) CloseWindow(Window);
	if (TSTFN(0x80)) CloseScreen(Screen);
	if (TSTFN(0x02)) CloseLibrary(GfxBase);
	if (TSTFN(0x01)) CloseLibrary(IntuitionBase);
	Exit(10);
}

/* 1. need a way to check for the presence of events without waiting. */
/* 2. need to pre-process events as much as possible */
/* 3. sort editor functions into immediate, immediate cancel, normal and
		background tasks. ---

		background --- window update
		normal -- line update
		immediate - keypress
		immediate cancel - open file
*/	

wait_event()
{	Wait((long)(1L << Window->UserPort->mp_SigBit));
	next_event();
}

/* get and process next event */

next_event()
{	while (message = (struct IntuiMessage *) GetMsg(Window->UserPort))
	{	class = message->Class;
		code = message->Code;
		address = (APTR)message->IAddress;
		mx = message->MouseX;
		my = message->MouseY;
		secs = message->Seconds;
		mics = message->Micros;
		qualifier = message->Qualifier;
		ReplyMsg(message);

		if (class == RAWKEY)
		{	if ((code & 0x80)==0)
			{	short keydown;
				key_event.ie_Class=IECLASS_RAWKEY;
				key_event.ie_Code=code;
				key_event.ie_Qualifier=qualifier;
				length=RawKeyConvert(&key_event,console_buf,15L,0L);
				if (length)
				{	if (keydown = multistroke(code))
					{	switch (keydown) {
						case 130 :
						case 131 :
						case 132 :
						case 133 : edit_hook(keydown);
								  break;
						default: close_all();
								 break;
						}
					}
					else 
					{	keydown = console_buf[0];
						edit_hook(keydown);
					}
				}
			}
		}
		else if (class == MOUSEBUTTONS)
		{	if (code == SELECTDOWN) 
			{	mouse_hook(mx,my);
/*				pick_x = (mx+2)/8; 
				pick_y = (my-11)/8; */ /* - winupper */
/*				if (pick_x == old_pick_x && pick_y == old_pick_y &&
					DoubleClick(pick_secs,pick_mics,secs,mics) ) clicktimes++;
				else clicktimes = 0;
				if (clicktimes == 3) clicktimes = 0;
				old_pick_x = pick_x;
				old_pick_y = pick_y;
				cpos (pick_x, pick_y); */
			}
			else if (code == SELECTUP)
			{	;	}
		}
	}
}

/* a general subroutine for detecting multiple stroke keys */
/*	arrows = 130 131 132 133
	help = 128
	functions = 140-149
*/

multistroke(key) short key;
{	if (key >= 0x50 && key <= 0x59) return key+(140-0x50); /* funtions */
	if (key == 0x5f) return 128; /* help */
	if (key >= 0x4c && key <= 0x4f) return key+(130-0x4c); /* arrs */
	return 0;
}

/* a collection of subroutines for forms */

#define INTEGER	0x01	/* integer field */
#define REAL	0x02	/* real numbers */
#define RETNEX	0x04	/* exit field only on return */
#define HEX		0x10	/* hex numbers */
#define PAD		0x20	/* pad with zeroes */
#define INSERT  0x40	/* insert mode this field */

#define	ALPHA	'A'
#define APLANUM 'X'
#define UCONLY  'U'
#define LCONLY  'L'
#define NUMONLY '9'
/* any other characters printed literally */

struct edit_form {
	short buffer, buffer_length;
	short picture, picture_length;
	short title, title_length;	/* offset into string area */
	char edit_type, precision;
	char textcolor;
	char backcolor;
	char bordercolor;		/* 0 = none */
	char titlecolor;
};

struct edit_list {
	struct edit_form *eform;
	short xpos, ypos;			/* offset into string area */
};

short 	index_v,				/* visual index */
	  	index_b=4,				/* buffer index */
		picton,					/* current picture character */
		pictcolor = 5;			/* color for pict stff */

/* test data */

char buffer_area[100] = "12345678901234567890                          ";
char string_area[] = 
"(999)999-9999Test Field:  UUUTest2:   Fgd Bgd Box Ttl Picture:Command:Color:R G B Size Title ";

struct edit_form kitty   = { 10,3,  26,3, 29,7,  0,0, 1,2,0,4 };
struct edit_form puppy   = { 0,10,  0,13, 13,12, 0,0, 1,2,3,4 };
struct edit_form field10 = { 13,1,  1,1,  38,4,  0,0, 1,2,1,4 };
struct edit_form field11 = { 14,1,  1,1,  42,4,  0,0, 1,2,1,4 };
struct edit_form field12 = { 15,1,  1,1,  46,4,  0,0, 1,2,1,4 };
struct edit_form field13 = { 16,1,  1,1,  50,4,  0,0, 1,2,1,4 };
struct edit_form picfld  = { 17,20, 0,0,  54,8,  1,0, 1,2,1,4 };

struct edit_list main_form[] = {
	{ &kitty, 20,30 },
	{ &puppy, 20,20 },
	{ &field10,20,45 },
	{ &field11,120,45 },
	{ &field12,220,45 },
	{ &field13,320,45 },
	{ &picfld ,20,60 }
};

struct edit_form commnd  = { 38,1, 26,1, 62,8, 0,0, 5,0,0,1 };
struct edit_list step_form[] = {
	{ &commnd, 20,180 } };

struct edit_form color_reg  = { 39,2, 1,2, 70,6, 1,0, 5,0,3,1 };
struct edit_list color_ask[] = {
	{ &color_reg, 20,180 } };

struct edit_form red_ask    = { 41,2, 1,2, 76,2, 0x21,0, 1,0,3,3 };
struct edit_form green_ask  = { 43,2, 1,2, 78,2, 0x21,0, 1,0,3,3 };
struct edit_form blue_ask   = { 45,2, 1,2, 80,2, 0x21,0, 1,0,3,3 };
struct edit_list color_set[] = {
	{ &red_ask,   20,20 },
	{ &green_ask, 20,40 },
	{ &blue_ask,  20,60 } };

struct edit_form blen_ask    = { 47,3, 1,3, 82,5, 0,0, 1,0,3,3 };
struct edit_form tlen_ask    = { 50,20,0,0, 87,6, 0,0, 1,0,3,3 };
struct edit_list form_ask[] = {
	{ &blen_ask, 20,20 },
	{ &tlen_ask, 20,32 } };

struct edit_form build_field = { 70,3, 1,3, 82,5, 0,0, 1,0,3,3 };

struct edit_list *form_ptr;
short current_field, last_field;

test()
{	SetRGB4(vp,0L,0L,0L,0L);
	start_form(main_form,7,0);
}

start_form(form,count,start) struct edit_list *form; short count,start;
{	form_ptr = form;
	draw_form(count);
	edit_field(start,0);
}

draw_form(n) short n;
{	short i, x, y; struct edit_form *ef;
	index_b = -1;
	for (i=0; i<n; i++)
	{	ef = form_ptr[i].eform;
		x = form_ptr[i].xpos;
		y = form_ptr[i].ypos;
		draw_title(ef,x,y);
		print_box(ef,x,y);
		print_field(ef,x,y);
	}
	last_field = n;
	index_b = 0;
}

struct	edit_form *ct_frm;
short	ct_x, ct_y;

edit_field(n,where) short n; short where;
{	current_field = n;
	if (n < 0 || n >= last_field)
	{	ct_frm = NULL;
		end_form(form_ptr);
	}
	else
	{	struct edit_list *el;
		el = &(form_ptr[n]);
		ct_frm = el->eform;
		ct_x = el->xpos;
		ct_y = el->ypos;
		index_b = where;
		if (index_b >= ct_frm->buffer_length)
			index_b = ct_frm->buffer_length - 1;
		print_field(ct_frm,ct_x,ct_y);
	}
}

long atol();
long int_value(ef) struct edit_form *ef;
{	char *buff, cs; long ival;
	buff = buffer_area + ef->buffer;
	cs = buff[ef->buffer_length];
	buff[ef->buffer_length] = 0;	/* wanky stuff here */
	ival = atol(buff);
	buff[ef->buffer_length] = cs;
	return ival;
}

clean_up_digits()
{	short i;
	float rval;
	if (ct_frm->edit_type & INTEGER) set_value(ct_frm,int_value(ct_frm));
	else if (ct_frm->edit_type & REAL)
	{	;	}
}

set_value(ef,val) struct edit_form *ef; long val;
{	char *buff, cs;
	buff = buffer_area + ef->buffer;
	cs = buff[ef->buffer_length];
	if (ef->edit_type & PAD)
		sprintf(buff,"%0*ld",ef->buffer_length,val);
	else sprintf(buff,"%*ld",ef->buffer_length,val);
	buff[ef->buffer_length] = cs;
}

set_contents(ef,c) struct edit_form *ef; char *c;
{	short i; char *buff;
	buff = buffer_area + ef->buffer;
	for (i=0; i<ef->buffer_length; i++)
		if (*c) buff[i] = *c++; else buff[i] = ' ';
}

change_fld(p,key) short p; char key; /* key that got us out of last field */
{	index_b = -1;
	clean_up_digits();
	print_field(ct_frm,ct_x,ct_y);
	if (p > 0) current_field++;
	else current_field--;

	if (key == 8) edit_field(current_field,1000);
	else edit_field(current_field,0);
}

char last_key;
edit_hook(key)
{	char *buff; short i;
	last_key = key;
	if (ct_frm)
	{	buff = buffer_area + ct_frm->buffer;
		if (key == '\r')
		{	change_fld(1,key);
		}
		else if (key == 8)
		{	if (index_b > 0)
			{	index_b--;
				buff[index_b] = ' ';	/* or zero?? */
				print_field(ct_frm,ct_x,ct_y);
			}
		}
		else if (key == 9)
		{	for (i=ct_frm->buffer_length-1; i > index_b; i--)
				buff[i]=buff[i-1];
			buff[index_b] = ' ';	/* or zero?? */
			print_field(ct_frm,ct_x,ct_y);
		}
		else if (key == 27)
		{	edit_field(-1,0);	/* escape from form */
			clean_up_digits();
			print_field(ct_frm,ct_x,ct_y);
		}
		else if (key == 127)
		{	for (i=index_b; i < ct_frm->buffer_length-1; i++)
				buff[i]=buff[i+1];
			buff[ct_frm->buffer_length-1] = ' ';
			print_field(ct_frm,ct_x,ct_y);
		}
		else if (key == 133)
		{	if (index_b > 0)
			{	index_b--;
				print_field(ct_frm,ct_x,ct_y);
			}
			else if ((ct_frm->edit_type & RETNEX)==0) change_fld(-1,8);
		}
		else if (key == 132)
		{	if (index_b+1 < ct_frm->buffer_length)
			{	index_b++;
				print_field(ct_frm,ct_x,ct_y);
			}
			else if ((ct_frm->edit_type & RETNEX)==0) change_fld(1,0);
		}
		else if (key == 130) change_fld(-1,0);
		else if (key == 131) change_fld(1,0);
		else
		{	switch (picton)
			{	case 'U' :
				case 'u' : key = toupper(key);
						   break;
				case 'L' :
				case 'l' : key = tolower(key);
						   break;
				case '9' : if (isdigit(key) || key == '.' || key == ' ') ;
						   else key = 0;
						   break;
				case 'A' : key = toupper(key);
				case 'a' : if (!isalpha(key)) key = 0;
						   break;
				default : break;
			}
			if (isprint(key))
			{	buff[index_b] = key;
				if (index_b+1 < ct_frm->buffer_length) 
				{	index_b++;
					print_field(ct_frm,ct_x,ct_y);
				}
				else if ((ct_frm->edit_type & RETNEX)==0) change_fld(1,0);
				else print_field(ct_frm,ct_x,ct_y);
			}
			else DisplayBeep(Screen);
		}
	}
}

mouse_hook(mx,my) short mx,my;
{	short i, x, y, l; struct edit_form *ef;
	index_b = -1;
	print_field(ct_frm,ct_x,ct_y);
	for (i=0; i<last_field; i++)
	{	ef = form_ptr[i].eform;
		x = form_ptr[i].xpos + (ef->title_length)*8;
		y = form_ptr[i].ypos - 7;
		if (mx >= x && my >= y && my <= (y+8))
		{	if (ef->picture_length)
			{	if (mx <= x + 1 + (ef->picture_length)*8)
				{	y=0;
					for (l=0; l<(mx-x)/8; l++)
					{	switch (string_area[ ef->picture + l ])
						{	case 'A' :
							case 'a' :
							case '9' :
							case 'X' :
							case 'x' :
							case 'U' :
							case 'u' :
							case 'L' :
							case 'l' : y++;
								break;
							default: break;
						}
					}
					edit_field (i,y);
				}
			}
			else
			{	if (mx <= x + 1 + (ef->buffer_length)*8)
					edit_field(i,(mx-x)/8);
			}
		}
	}
}

draw_title(ef,x,y) struct edit_form *ef; short x,y;
{	char *titl;
	titl = string_area + ef->title;
	Move(rp,(long)x,(long)y);
	SetAPen(rp,(long)ef->titlecolor);
	SetBPen(rp,0L);
	if (ef->title_length) Text(rp,titl,(long)(ef->title_length));
}

print_box(ef,x,y) struct edit_form *ef; short x,y;
{	long x1,y1,x2,y2;
	if (ef->bordercolor)
	{	x1 = x + (ef->title_length)*8 - 1;
		y1 = y-8;
		if (ef->picture_length)	x2 = x1 + 2 + (ef->picture_length)*8;
			else x2 = x1 + 2 + (ef->buffer_length)*8;
		y2 = y+2;
		SetAPen(rp,(long)(ef->bordercolor));
		Move(rp,x1,y1);
		Draw(rp,x1,y2); Draw(rp,x2,y2); Draw(rp,x2,y1); Draw(rp,x1,y1);
	}
}

print_field(ef,x,y) struct edit_form *ef; short x,y;
{	short i, b, fl; char *pict, *buff;
	buff = buffer_area + ef->buffer;
	pict = string_area + ef->picture;
	Move(rp,(long)x + (ef->title_length)*8,(long)y);
	b = 0;
	SetDrMd(rp,(long)JAM2); 
	SetBPen(rp,(long)(ef->backcolor));
	x = rp->cp_x;
	y = rp->cp_y - 7; 
	if (ef->picture_length)
	{	for (i = 0; i<ef->picture_length; i++)
		{	SetDrMd(rp,(long)JAM2);
			switch (pict[i])
			{	case 'A' :
				case 'a' :
				case '9' :
				case 'X' :
				case 'x' :
				case 'U' :
				case 'u' :
				case 'L' :
				case 'l' :
					if (b == index_b)
					{	picton = pict[i];
						SetDrMd(rp,(long)JAM2+INVERSVID);
					}
					SetAPen(rp,(long)(ef->textcolor));
					Text(rp,buff+(b++),1L);
					break;
				default :
					SetAPen(rp,(long)(pictcolor));
					Text(rp,pict+i,1L);
					break;
			}
		}
		fl = ef->picture_length*8;
	}	
	else
	{	SetAPen(rp,(long)(ef->textcolor));
		for (i = 0; i<ef->buffer_length; i++)
		{	if (b == index_b) SetDrMd(rp,(long)JAM2+INVERSVID);
			else SetDrMd(rp,(long)JAM2);
			Text(rp,buff+(b++),1L);
		}
		picton = 'X';
		fl = ef->buffer_length*8;
	}	
	SetDrMd(rp,(long)JAM2);
	Move(rp,(long)x,(long)y);
	SetAPen(rp,(long)ef->backcolor);
	Draw(rp,(long)x+fl,(long)y);
	Draw(rp,(long)x+fl,(long)y+8);
}

cls()
{	SetRast(rp,0l);	}

/* this function supplied by the user */
/* it is called by the form routines every time a form finishes */

long GetRGB4();

end_form(form) struct edit_list *form;
{	char c; static long color;
	if (form == form_ask)
	{	start_form(step_form,1,0);
	}
	else if (form == color_set)
	{	SetRGB4(vp,color,
			int_value(&red_ask),
			int_value(&green_ask),
			int_value(&blue_ask));
		start_form(step_form,1,0);
	}
	else if (form == step_form)
	{	SetAPen(rp,0L);
		RectFill(rp,0L,170L,639L,199L);
		c = buffer_area[commnd.buffer];
		set_contents(&commnd," ");	/* set to blank */
		switch (c) {
		case 'E' : cls(); start_form(main_form,7,0); break;
		case 'C' : cls(); start_form(color_ask,1,0); break;
		case 'Q' : close_all();
		case 'F' : cls(); start_form(form_ask,1,0); break;
		default : start_form(step_form,1,0); /* what a wierd loop!! */
		}
	}
	else if (form == color_ask)
	{	short c,r,g,b;
		color = int_value(&color_reg);
		c = GetRGB4(vp->ColorMap,color);
		cls();
		b = c & 15;
		g = (c>>4) & 15;
		r = (c>>8) & 15;
		set_value(&red_ask,(long)r);
		set_value(&green_ask,(long)g);
		set_value(&blue_ask,(long)b);
		start_form(color_set,3,0);
	}
	else if (form == main_form)
	{	SetAPen(rp,0L);
		RectFill(rp,0L,150L,639L,170L);
		blen_ask.textcolor = int_value(&field10),
		blen_ask.backcolor = int_value(&field11),
		blen_ask.bordercolor = int_value(&field12),
		blen_ask.titlecolor = int_value(&field13),
		draw_title(&blen_ask,20,160);
		print_box(&blen_ask,20,160);
		print_field(&blen_ask,20,160);
		if (last_key == 27)	{ cls(); start_form(step_form,1,0); }
		else start_form(main_form,7,0);
	}
	else start_form(step_form,1,0); /* if nothing else, start a command */
}
