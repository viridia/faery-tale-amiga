/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *			 CAD experiment - Created: Jan 69 1986		 *
 *                         By Dave Joiner			 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "exec/types.h"
#include "exec/memory.h"
#include "graphics/gfx.h"
#include "graphics/text.h"
#include "libraries/diskfont.h"
#include "intuition/intuition.h"

struct Library
	*IntuitionBase, *GfxBase, *LayersBase, *DiskfontBase, *OpenLibrary();
struct NewScreen NewScreen =
	{ 0,0,320,200, 2, 0,1, NULL, CUSTOMSCREEN,NULL,NULL,NULL, };

struct NewWindow back_ground =
{	0,0,320,200, 0,1,
	GADGETUP | GADGETDOWN | MOUSEBUTTONS | MENUPICK | MOUSEMOVE | VANILLAKEY,
	NOCAREREFRESH | ACTIVATE | SMART_REFRESH | BORDERLESS | REPORTMOUSE | BACKDROP,
	NULL,NULL,NULL,NULL,NULL,320,200,320,200,CUSTOMSCREEN,
};

struct Window *Window, *OpenWindow();
struct Screen *Screen, *OpenScreen();
struct RastPort *rp;
struct IntuiMessage *message, *GetMsg();

long	class;
short	qualifier, code, mx, my;
APTR 	address;

struct RastPort one_rast;
struct BitMap one_map;

#define DF0	ITEMTEXT | ITEMENABLED | HIGHCOMP
#define DF2	ITEMTEXT | ITEMENABLED | HIGHCOMP | CHECKIT
#define DF3	ITEMTEXT | ITEMENABLED | HIGHCOMP | COMMSEQ
#define DF4	ITEMTEXT | ITEMENABLED | HIGHCOMP | COMMSEQ | CHECKIT
#define CF2	DF2 | CHECKED | COMMSEQ
#define OFF_F	ITEMTEXT | HIGHCOMP | CHECKIT

struct IntuiText
	samp_tx = { 0,1,JAM1,18,1,NULL,(UBYTE *)"Bold", NULL, },
	voxx_tx = { 0,1,JAM1,18,1,NULL,(UBYTE *)"Gfx Test", NULL, },
	text_tx = { 0,1,JAM1,18,1,NULL,(UBYTE *)"Repeat Test", NULL, },
	imge_tx = { 0,1,JAM1,18,1,NULL,(UBYTE *)"Test", NULL, },
	quit_tx = { 0,1,JAM1,18,1,NULL,(UBYTE *)"Quit",NULL, };

struct MenuItem
	samp_mi = { NULL,	 0,30,160,10,DF4,0xffef,(APTR)&samp_tx,0,'X' },
	voxx_mi = { &samp_mi,0,10,160,10,DF4,0xfff7,(APTR)&voxx_tx,0,'V' },
	text_mi = { &voxx_mi,0,20,160,10,CF2,0xfffb,(APTR)&text_tx,0,'S' },
	imge_mi = { &text_mi,0,00,160,10,DF4,0xfffd,(APTR)&imge_tx,0,'W' },
	quit_mi = { &imge_mi,0,40,160,10,DF3,0,(APTR)&quit_tx,0,'Q' };

struct Menu menu1 =
	{	NULL, 1,0,56,10, MENUENABLED | MIDRAWN,"Control", &quit_mi };

struct IntuiText
	lowr_tx = { 0,1,JAM1,18,1,NULL,(UBYTE *)"Low Resolution", NULL, },
	medr_tx = { 0,1,JAM1,18,1,NULL,(UBYTE *)"Hi Resolution", NULL, },
	hirs_tx = { 0,1,JAM1,18,1,NULL,(UBYTE *)"Interlaced", NULL, };

struct MenuItem
	lowr_mi = { NULL    ,0,00,140,10,CF2,0xfffb,(APTR)&lowr_tx },
	medr_mi = { &lowr_mi,0,10,140,10,DF2,0xfffd,(APTR)&medr_tx },
	hirs_mi = { &medr_mi,0,20,140,10,DF2,0xfffe,(APTR)&hirs_tx };

struct Menu menu2 =
	{	&menu1, 70,0,44,10, MENUENABLED | MIDRAWN, "Screen", &hirs_mi };

struct IntuiText
	load_tx = { 0,1,JAM1,18,1,NULL,(UBYTE *)"Load Item", NULL, },
	save_tx = { 0,1,JAM1,18,1,NULL,(UBYTE *)"Save Item", NULL, };

struct MenuItem
	load_mi = { NULL,	0,00,120,10,DF0,0,(APTR)&load_tx },
	save_mi = { &load_mi,0,10,120,10,DF0,0,(APTR)&save_tx };

struct Menu menu3 =
	{	&menu2, 130,0,34,10, MENUENABLED | MIDRAWN,"File", &save_mi };

struct IntuiText
	bold_tx = { 0,1,JAM1,5,1,NULL,(UBYTE *)"Bold", NULL, },
	ital_tx = { 0,1,JAM1,5,1,NULL,(UBYTE *)"Italic", NULL, },
	undr_tx = { 0,1,JAM1,5,1,NULL,(UBYTE *)"Underline", NULL, },
	font_tx = { 0,1,JAM1,5,1,NULL,(UBYTE *)"Load Font Dir", NULL, };

struct MenuItem
	font_mi = { NULL,    0,35,100,10,DF0,0,(APTR)&font_tx },
	bold_mi = { &font_mi,0,00,100,10,DF0,0,(APTR)&bold_tx },
	ital_mi = { &bold_mi,0,10,100,10,DF0,0,(APTR)&ital_tx },
	undr_mi = { &ital_mi,0,20,100,10,DF0,0,(APTR)&undr_tx };

struct Menu menu4 =
	{	&menu3, 170,0,40,10, MENUENABLED | MIDRAWN,"Style", &undr_mi };

/* temporary open requester */

struct IntuiText open_tx = { 0,1,JAM1,10,3,NULL,(UBYTE *)"Open File",  NULL, };
struct IntuiText ok_tx =   { 0,1,JAM1, 3,2,NULL,(UBYTE *)"Open",  NULL, };
struct IntuiText nix_tx =  { 0,1,JAM1, 3,2,NULL,(UBYTE *)"Cancel",  NULL, };

short req_points[] = { 0,0, 179,0, 179,35, 0,35, 0,0 };
short req2_points[] = { 0,0, 49,0, 49,11, 0,11, 0,0 };

struct Border req_bord = {0,0, 1,0,JAM1, 5,req_points, NULL };
struct Border req2_bord = {0,0, 1,0,JAM1, 5,req2_points, NULL };

UBYTE string_buf[50];
UBYTE int_buf[5] = "0   ";

struct Gadget ok_click =
	{	NULL, 10,22,50,12,
		GADGHCOMP,	GADGIMMEDIATE | ENDGADGET,  BOOLGADGET | REQGADGET,
		(APTR)&req2_bord, NULL, &ok_tx };

struct Gadget nix_click =
	{	&ok_click, 70,22,50,12,
		GADGHCOMP, GADGIMMEDIATE | ENDGADGET, BOOLGADGET | REQGADGET,
		(APTR)&req2_bord, NULL, &nix_tx };

struct StringInfo int_i = { int_buf,NULL, 0,3, 0, };
struct Gadget set_num =
	{	&nix_click, 143,3,20,16,
		GADGHCOMP, LONGINT, STRGADGET | REQGADGET,
		NULL, NULL, NULL, NULL, (APTR)&int_i, NULL,NULL, };

struct StringInfo string_i = { string_buf,NULL, 0,40, 0, };
struct Gadget file_name =
	{	&set_num, 10,12,170,10,
		GADGHCOMP, NULL, STRGADGET | REQGADGET,
		NULL, NULL, NULL, NULL, (APTR)&string_i, NULL,NULL, };

struct Requester open_req = 
{ NULL, 20,30,180,36, 0,0, &file_name,&req_bord,&open_tx, 0, 2,NULL,NULL };

short req_mode; /* set if we opened a requester */

char  *memstart;

APTR	Open();
char	*AllocMem();

short	drag_mode,
		screen_mode;

/* long	LoadSeg(), seg;
struct	DiskFontHeader *font; */
struct  TextFont *tfont, *mfont, *OpenDiskFont();

struct TextAttr infont = { (UBYTE *)"sapphire.font",19,0,0 };

#define AFTSIZE	2000
struct AvailFontsHeader *afh;
struct MenuItem fontmenus[20], *fontsubs;
struct IntuiText *fontnames;

main()
{	short i; char *c;
	long packlen;
	IntuitionBase = GfxBase = LayersBase = NULL; afh = NULL;
	fontnames = NULL; /* fontmenus = */ fontsubs = NULL;
	if ((IntuitionBase = OpenLibrary("intuition.library",0)) == NULL) goto term;
	if ((GfxBase = OpenLibrary("graphics.library",0)) == NULL) goto term;
	if ((LayersBase = OpenLibrary("layers.library",0)) == NULL) goto term;
	if ((DiskfontBase = OpenLibrary("diskfont.library",0)) == NULL) goto term;

	Screen = Window = NULL;
	open_screen(1);
	if (Window == NULL) goto term;

/*	if ((seg = LoadSeg("symbols")) == NULL) goto term;
	font = (struct DiskFontHeader *) ((seg<<2)+8);
	tfont = rp->Font;
	mfont = &(font->dfh_TF);
*/

/*	memstart = AllocMem(MEM_SIZE,MEMF_PUBLIC | MEMF_CLEAR);
	if (memstart == 0) goto term;
*/

	tfont = OpenDiskFont(&infont);

	font_test();
	while (TRUE)
	{
		Wait(1 << Window->UserPort->mp_SigBit);

		while (message = GetMsg(Window->UserPort))
		{	class = message->Class;
			code = message->Code;
			address = (APTR)message->IAddress;
			qualifier = message->Qualifier;
			mx = message->MouseX-1;
			my = message->MouseY;
			ReplyMsg(message);

			if (class == MOUSEMOVE) ;
			else if (class == VANILLAKEY)
			{	;
			}
			else if (req_mode && (class == GADGETDOWN || class == GADGETUP))
			{	if (address == (APTR)&ok_click)
				{	APTR myfile;
/*					switch (req_mode) {
					case LOAD_VOICE :
						if (myfile = Open(string_buf,1005))
						{	Read(myfile,memstart,VOICE_SZ);
							Close(myfile);
						}
						break;
					case SAVE_VOICE :
						if (myfile = Open(string_buf,1006))
						{	Write(myfile,memstart,VOICE_SZ);
							Close(myfile);
						}
						break;
					}
*/				}
				/* req_mode = FALSE; */
			}
			else if (class == MENUPICK)
			{	drag_mode = 0;
				if (code!=MENUNULL)
				{	if (MENUNUM(code)==3)
					{	switch (ITEMNUM(code)) {
						case 4: SetSoftStyle(rp,FSF_ITALIC,-1); break;
						case 3: SetSoftStyle(rp,FSF_BOLD,-1); break;
						case 2: SetSoftStyle(rp,FS_NORMAL,-1); break;
						case 1: font_test();
								/* screen_mode = WAVE_MODE;
								draw_wave(); */ break;
						case 0: /* FreeMem(memstart,MEM_SIZE);
								tone_off(); */
								goto term;
						}
					}
					else if (MENUNUM(code)==1)
					{	switch(ITEMNUM(code)) {
						case 0 : /* save */
/*							switch (screen_mode) {
							case VOICE_MODE:
							case WAVE_MODE :
								open_file_req("Save Voices",SAVE_VOICE); break;
							case SCORE_MODE:
								open_file_req("Save Score",SAVE_SCORE); break;
							}
*/							break;
						case 1 : /* load */
/*							switch (screen_mode) {
							case WAVE_MODE :
							case VOICE_MODE:
								open_file_req("Load Voices",LOAD_VOICE); break;
							case SCORE_MODE:
								open_file_req("Load Score",LOAD_SCORE); break;
							case SAMPLE_MODE:
								open_file_req("Load Samples",LOAD_SAMPLE); break;
							}
*/							break;
						}
					}
					else if (MENUNUM(code)==2)
					{	switch(ITEMNUM(code)) {
						case 0 : open_screen(3); break;
						case 1 : open_screen(2); break;
						case 2 : open_screen(1); break;
						}
						if (Window == NULL) goto term; /* else draw_all(); */
					}
					else if (MENUNUM(code)==0)
					{	switch(ITEMNUM(code)) {
						case 3:
							if (afh == NULL) getfonts(); break;
						}
					}
				}
			}
			else if (class == MOUSEBUTTONS)
			{	if (code == SELECTDOWN && drag_mode == 0)
				{	;
				}
				else if (code == SELECTUP && (drag_mode != 0) )
				{	;
				}
			}
		}
		/* if (drag_mode) { drag_object(); } */
	}
term:
	if (afh) FreeMem(afh,AFTSIZE);
	closefonts();
	if (tfont) CloseFont(tfont);
	close_screen();
	if (DiskfontBase)
	{	CloseLibrary(DiskfontBase);
		CloseLibrary(LayersBase);
		CloseLibrary(GfxBase);
		CloseLibrary(IntuitionBase);
	}
	Exit(0);
}

open_file_req(name,mode) char *name; short mode;
{	req_mode = mode;
	open_tx.IText = (UBYTE *)name;
	Request(&open_req,Window);
}

BYTE	resolution;
SHORT	screen_w, screen_h;

open_screen(r) BYTE r;
{	resolution = r;
	close_screen();
	if (r == 1)
	{	NewScreen.ViewModes = NULL; screen_w = 320; screen_h = 200; }
	else if (r == 2)
	{	NewScreen.ViewModes = HIRES; screen_w = 640; screen_h = 200; }
	else if (r == 3)
	{	NewScreen.ViewModes = HIRES|LACE; screen_w = 640; screen_h = 400; }
	else return;

	back_ground.Width = NewScreen.Width = screen_w;
	back_ground.Height = NewScreen.Height = screen_h;

	if ((Screen = OpenScreen(&NewScreen)) == NULL) return;
	ShowTitle(Screen,TRUE);
	back_ground.Screen = Screen;

	if ((Window = OpenWindow(&back_ground)) == NULL) return;

	/*	create a new rastport and bitmap (1 plane only) which point
			to the least significant plane of Screen */

	InitBitMap(&one_map,1,screen_w,screen_h);
	one_map.Planes[0] = Window->RPort->BitMap->Planes[0]; 
	InitRastPort(&one_rast);
	one_rast.BitMap = &one_map;
	one_rast.Layer = Window->RPort->Layer;
	rp = &one_rast;

	SetAPen(rp,1);
	SetMenuStrip(Window,&menu4);

/*	clip_rt.MinX = 0;
	clip_rt.MinY = 10;
	clip_rt.MaxX = screen_w - 20;
	clip_rt.MaxY = screen_h - 10;
	mid_x = screen_w/2;
	mid_y = (screen_h + 2)/2; */
}

close_screen()
{	if (Window)
	{	ClearMenuStrip(Window);
			/* InstallClipRegion(rp->Layer,NULL); */
		CloseWindow(Window);
		Window = NULL;
	}
	if (Screen)
	{	CloseScreen(Screen);
		Screen = NULL;
	}
	/* UnLoadSeg(seg); */
}

font_test()
{	short yline, i;

	SetFont(rp,tfont);
	for (i=0; i<20; i++)
	{	SetRast(rp,0);
		yline = 10 + tfont->tf_Baseline;
		while (yline < (screen_h - 5))
		{	Move(rp,0,yline);
			Text(rp,"This is a test of Sapphire font. How fast is it?",47);
			yline += tfont->tf_YSize;
		}
	}
}

#define	MNSIZE	(sizeof (struct MenuItem))
#define	ITSIZE	(sizeof (struct IntuiText))

getfonts()
{	short j, k, num_entries, menucount, subcount;
	struct AvailFonts *af;
	struct MenuItem *mi, *lastmenu;
	struct IntuiText *tx;
	afh = (struct AvailFontsHeader *) AllocMem(AFTSIZE,MEMF_CHIP);
	if (afh)
	{	/* fontmenus = (struct MenuItem *)AllocMem(MNSIZE * 20,NULL);
		if (!fontmenus) goto fail; */
		fontnames = (struct IntuiText *)AllocMem(ITSIZE * 20,MEMF_CHIP);
		if (fontnames == NULL) goto fail;
		AvailFonts(afh,AFTSIZE,0xff);
		num_entries = afh->afh_NumEntries;
		af = (struct AvailFonts *) &afh[1];
		lastmenu = NULL;
		menucount = subcount = 0;
		for (j=0; j<num_entries; j++)
		{	if ((af->af_Attr.ta_Flags & (FPF_REMOVED | FPF_REVPATH)) ||
				( (af->af_Type & AFF_MEMORY) && (af->af_Attr.ta_Flags && FPF_DISKFONT)))
				;
			else
			{	subcount++;
				/* see if this font's name has been used already */
				for (k=0; k<menucount; k++)
				{	if (strcmp(fontnames[k].IText,af->af_Attr.ta_Name)==0)
						break;
				}
				if (k==menucount) /* i.e. test failed */
				{	mi = &fontmenus[menucount];
					tx = &fontnames[menucount];
					if (lastmenu) lastmenu->NextItem = mi;
					mi->NextItem = NULL;
					lastmenu = mi;
					mi->LeftEdge = 0;
					mi->TopEdge = (menucount * 10) + 35;
					mi->Width = 120;
					mi->Height = 10;
					mi->Flags = DF0;
					mi->MutualExclude = 0;
					mi->ItemFill = (APTR) tx; /* IntuiText */
					mi->Command = 0;
					mi->SubItem = NULL;
					mi->NextSelect = NULL;

					tx->FrontPen = 1;
					tx->BackPen = 0;
					tx->DrawMode = JAM1;
					tx->LeftEdge = 5;
					tx->TopEdge = 1;
					tx->ITextFont = tx->NextText = NULL;
					tx->IText = af->af_Attr.ta_Name;
					printf("\nFont name found was: %ls - %ld",
						af->af_Attr.ta_Name,af->af_Attr.ta_YSize);
					menucount++;
					if (menucount > 16) break;
				}
			}
			af++;
		}

		if (menucount)
		{	ClearMenuStrip(Window);
			bold_mi.NextItem = fontmenus;
			SetMenuStrip(Window,&menu4);
		}
		fail: ;
	}
}

closefonts()
{	/* if (fontmenus) FreeMem(fontmenus,MNSIZE * 20); */
	if (fontnames) FreeMem(fontnames,ITSIZE * 20);
}
