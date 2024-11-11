;		assembly language subroutines for faerie tale adventure 
;		written august 86 by Talin

;		AmigaDOS subroutine vectors

Write		EQU	$FFFFFFD0
Read		EQU	$FFFFFFD6
Close		EQU	$FFFFFFDC
Open		EQU	$FFFFFFE2
Move		EQU	$FFFFFF10
Text		EQU	$FFFFFFC4
SetAPen		EQU	$FFFFFEAA
Draw		EQU	$FFFFFF0A
BltBitMap	EQU	$FFFFFFE2
SetRGB4		EQU	$FFFFFEE0
WaitBlit	EQU	$FFFFFF1C
OwnBlitter	EQU	$FFFFFE38
DisownBlitter EQU $FFFFFE32


ie_X			equ		10
ie_Y			equ		12
ie_NextEvent	equ		0


xwrap		macro
			btst	#6,\1				; if 0-64, process normally
			beq.s	98$
			btst	#5,\1				; else wrap in various directions
			seq		d2					; if zero, set to all 1's (mask 63)
			and.w	#63,\1				; else set to all zero's
98$
			endm

ywrap		macro
			btst	#5,\1
			beq.s	99$
			btst	#4,\1
			seq		\1
			and.w	#31,\1
99$
			endm


			public _trapper
_trapper
			move.l	(sp)+,4(a1)
			rte

			public	_HandlerInterface,_ion,_numbuf,_getkey

RAWKEY			equ		1
RAWMOUSE		equ		2
POINTERPOS		equ		4
TIMER			equ		6
DISKIN			equ		$10

;	a0 contains inputevent stream
;	a1 contains data area

xsprite		equ		0
ysprite		equ		2
qualifier	equ		4

		public	_LVOMoveSprite

_HandlerInterface:
		movem.l	d1-d2/a1-a3,-(sp)
		move.l	a0,a2				; save list address
		lea		keytrans,a3
handloop
; first we check what the actual event was before we mess with it
		move.b	4(a0),d0			; check event type

		cmp.b	#TIMER,d0			; is it a raw key?
		bne.s	checkkey
		cmp.w	#16,22+128(a1)
		beq.s	bang
		addq.w	#1,22+128(a1)
		bra.s	nomouse
bang	move.b	#RAWKEY,4(a0)		; make an up shift
		move.w	#($80+$60),6(a0)
		move.w	#0,22+128(a1)
		bra.s	nomouse

checkkey
		cmp.b	#RAWKEY,d0			; is it a raw key?
		bne.s	notrawkey			; check other possibilities

		move.w	8(a0),d0			; get qualifier
		btst	#9,d0				; a repeat?
		bne.s	nomouse				; is so, ignore

		move.w	6(a0),d0			; get raw key code
		move.b	d0,d1				; save high bit
		and.b	#$7f,d0				; turn off high bit
		cmp.b	#$5a,d0				; ignore undefined keys
		bhi.s	nomouse				; is so, ignore for now

		move.w	#$0000,4(a0)		; change to null event

		move.b	(a3,d0),d0			; translate
		and.b	#$80,d1				; isolate old upper bit (up/down)
		or.b	d1,d0				; and mark key as up or down
		clr.w	d1					; clear upper half d1
		move.b	6(a1),d1			; get laydown pointer
		move.b	d0,22(a1,d1.w)		; move converted code into buffer

		addq.b	#1,d1				; increment buffer
		and.b	#$7f,d1				; and limit it
		cmp.b	7(a1),d1			; same as pickup pointer?
		beq.s	nomouse				; is so, overflow
		move.b	d1,6(a1)			; otherwise, update buffer
		bra.s	nomouse

notrawkey
		cmp.b	#RAWMOUSE,d0
		bne.s	domouse

		move.w	8(a0),d1			; get qualifier
		move.w	4(a1),d0
		eor.w	d1,d0				; old xor new
		and.w	#$4000,d0			; isolate left button bit
		beq.s	nobutn				; left button not changed

		and.w	#$4000,d1			; test new bit value
		bne.s	butndown			; if down, goto down trans

		move.b	9(a1),d1			; get old menu value
		beq.s	nobutn				; if none, no action
		or.b	#$80,d1				; set up bit
		clr.b	9(a1)				; clear old action
		bra.s	butn10				; put it in the que

butndown
		clr.l	d1
		move.w	xsprite(a1),d0		; get current x/y coords
		move.w	ysprite(a1),d1

		cmp.w	#215,d0				; in range for menu activation?
		blo.s	nobutn
		cmp.w	#265,d0
		bhi.s	nobutn

		sub.w	#144,d1				; (my - 144)/9 + 'a'
		bmi.s	nobutn
		divu.w	#9,d1
		add.w	d1,d1
		add.w	#$61,d1

		cmp.w	#240,d0
		blo.s	butn10
		addq	#1,d1
butn10
		move.w	d1,d0
		clr.w	d1
		move.b	6(a1),d1			; get laydown pointer
		move.b	d0,22(a1,d1.w)		; move converted code into buffer

		addq.b	#1,d1				; increment buffer
		and.b	#$7f,d1				; and limit it
		cmp.b	7(a1),d1			; same as pickup pointer?
		beq.s	nobutn				; is so, overflow
		move.b	d1,6(a1)			; otherwise, update buffer
		move.b	d0,9(a1)			; save menu choice
nobutn
		move.w	8(a0),4(a1)			; move qualifier to in_work

domouse
		cmp.b	#DISKIN,d0
		bne.s	domouse1
		move.b	#1,8(a1)
domouse1
		move.w	xsprite(a1),d0		; get current coords
		move.w	ysprite(a1),d1

		add.w	ie_X(a0),d0			; add to xsprite
		add.w	ie_Y(a0),d1			; add to ysprite

		cmp.w	#315,d0
		blt.s	xhi
		move.w	#315,d0
xhi
		cmp.w	#5,d0
		bgt.s	xlo
		move.w	#5,d0
xlo
		cmp.w	#195,d1
		blt.s	yhi
		move.w	#195,d1
yhi
		cmp.w	#147,d1
		bgt.s	ylo
		move.w	#147,d1
ylo
		move.w	d0,xsprite(a1)
		move.w	d1,ysprite(a1)
		tst.l	14(a1)				; do we have simplesprite?
		beq.s	nosprite

		movem.l	a0/a1/a6,-(sp)
		add.w	d0,d0
		sub.w	#143,d1				; vp_text offset
		move.l	10(a1),a6			; Gfx Base
		move.l	18(a1),a0			; ViewPort
		move.l	14(a1),a1			; simplesprite
		jsr		_LVOMoveSprite(a6)
		movem.l	(sp)+,a0/a1/a6
nosprite

nomouse
		move.l	ie_NextEvent(a0),a0	; next event in chain
		move.l	a0,d0				; set zero flag -- if null, quit
		bne.s	handloop			; else process next

		clr.l	d0

		movem.l	(sp)+,d1-d2/a1-a3
		rts

keytrans	dc.b	"`1234567890-=\?0"
			dc.b	"QWERTYUIOP{}?",26,25,24
			dc.b	"ASDFGHJKL:???",27,29,23
			dc.b	"?ZXCVBNM,.??.",20,21,22
			dc.b	$20,$08,$09,$0D,$0D,$1B,$7F,0,0,0,$2D,0,1,2,3,4
			dc.b	10,11,12,13,14,15,16,17,18,19,0,0,0,0,0,0

XY			equ		128		; then x/2 then y
ETX			equ		0

			public	_ssp,_titletext,_GfxBase

			dseg

			public	_titletext
_titletext	dc.b	XY,(160-26*4)/2,33,$22,"The Faery Tale Adventure",$22
			dc.b	XY,(160-30*4)/2,79,"Animation, Programming & Music"
			dc.b	XY,(160-2*4)/2,90,"by"
			dc.b	XY,(160-12*4)/2,101,"David Joiner"
			dc.b	XY,(160-30*4)/2,160,"Copyright 1986 MicroIllusions "
;_titletext	dc.b	XY,168/2,33,$22,"The Faery Tale Adventure",$22
;			dc.b	XY,153/2,74,"Animation, Programming & Music"
;			dc.b	XY,292/2,90,"by"
;			dc.b	XY,250/2,101,"David Joiner"
;			dc.b	XY,168/2,160,"Copyright (C) 1986 MicroIllusions "
			dc.b	ETX

			public _hinor,_hivar
_hinor
			dc.l	$01FFF8FF,$FC038000,$01FF0007,$FC038000
			dc.l	$07E0F078,$3F038000,$190FE03F,$C4C38000
			dc.l	$FC7FC01F,$F1FF8000,$F19F800F,$CC7F8000
			dc.l	$E7E70007,$3F3F8000,$CFF9800C,$FF9F8000
			dc.l	$9FFE6033,$FFCF8000,$3F0798CF,$C3E78000
			dc.l	$7001E79F,$00338000,$0000783C,$00038000
			dc.l	$00001930,$00000000,$0000780C,$00038000
			dc.l	$7801C7F3,$00338000,$3F87383C,$C3E78000
			dc.l	$9FFCF01F,$3FCF8000,$CFF3C00F,$CF9F8000
			dc.l	$E7CF8007,$F33F8000,$F13F800F,$FC7F8000
			dc.l	$FC7FC01F,$F1308000,$331FE03F,$C7C08000
			dc.l	$0FE0F078,$3F008000,$03FF0007,$FF008000
			dc.l	$03FFF8FF,$FFFF0000
_hivar
			dc.l	$01FFF8FF,$FC038000,$79FF0207,$FCF38000
			dc.l	$67E0F278,$3F338000,$190FE73F,$C4C38000
			dc.l	$FC7FCF9F,$F1FF8000,$F19F9FCF,$CC7F8000
			dc.l	$E7E73FE7,$3F3F8000,$CFF99FCC,$FF9F8000
			dc.l	$9FFE6733,$FFCF8000,$3F0798CF,$C3E78000
			dc.l	$70F9E79F,$3C338000,$0FFE783C,$FFC38000
			dc.l	$FFFF9933,$FFF80000,$07FE780C,$FFC38000
			dc.l	$7879C7F3,$3C338000,$3F87383C,$C3E78000
			dc.l	$9FFCF39F,$3FCF8000,$CFF3CFCF,$CF9F8000
			dc.l	$E7CF9FE7,$F33F8000,$F13F9FCF,$FC7F8000
			dc.l	$FC7FCF9F,$F1308000,$331FE73F,$C7CC8000
			dc.l	$CFE0F278,$3F3C8000,$F3FF0207,$FF008000
			dc.l	$03FFF8FF,$FFFF0000

			cseg

			public	_handler_data
_getkey
			movem.l	a1/d1,-(sp)
			lea		_handler_data,a1
			clr.l	d0
			clr.w	d1
			move.b	7(a1),d1			; get pickup pointer
			cmp.b	6(a1),d1			; if same as laydown
			beq.s	getkeyx
			move.b	22(a1,d1.w),d0		; get key from buffer
			addq	#1,d1
			and.b	#$7f,d1				; buffer is 128 long
			move.b	d1,7(a1)
getkeyx		movem.l	(sp)+,a1/d1
			rts

			public	_rand,_seed1,_bitrand,_rnd
			public	_rand2,_rand4,_rand8,_rand64,_rand256

_rand
			move.l	_seed1,d0
			mulu.w	#45821,d0
			addq.l	#1,d0
			move.l	d0,_seed1
			ror.l	#6,d0
			and.l	#$7fffffff,d0
			rts

_bitrand	bsr.s	_rand		; rand() & x
			and.l	4(sp),d0
			rts

_rand2		bsr.s	_rand		; rand() & 1
			and.l	#1,d0
			rts

_rand4		bsr.s	_rand		; rand() & 3
			and.l	#3,d0
			rts

_rand8		bsr.s	_rand		; rand() & 7
			and.l	#7,d0
			rts

_rand64		bsr.s	_rand		; rand() & 63
			and.l	#63,d0
			rts

_rand256	bsr.s	_rand		; rand() & 255
			and.l	#255,d0
			rts

_rnd		bsr.s	_rand		; rand() % x
			move.l	4(sp),d1
			and.l	#$0000ffff,d0
			divu.w	d1,d0		; d0 / n
			clr.w	d0			; clear bottom half
			swap	d0			; get top half
			rts

			public	_prdec

_prdec		
			movem.l	a0-a6/d0-d7,-(sp)
			move.l	60+4(sp),d0
			bsr.s	ion6

			add		#10,a0
			move.l	60+8(sp),d0		; length
			sub.w	d0,a0			; numbuf+10-p
			add		#1,d0

			move.l	_rp,a1		; Rast Port (rp)
			move.l	_GfxBase,a6
			jsr		Text(a6)
			movem.l	(sp)+,a0-a6/d0-d7
			rts
_ion
			move.l	4(sp),d0
ion6
			lea		_numbuf,a0
			moveq.l	#9,d1
			bra.s	ion5
ion1
			tst.w	d0
			beq.s	ion2
ion5
			divu.w	#10,d0
			swap	d0
			add.b	#$30,d0
			move.b	d0,(a0,d1)
			clr.w	d0
			swap	d0
			dbf		d1,ion1
			rts

ion2		move.b	#$20,(a0,d1)	; space fill until end
			dbf		d1,ion2
			rts

			public	_move,_rp,_text,_placard,_rp_map

MOD		equ		4

xmod	dc.b	-MOD,-MOD,-MOD,0,0,0,MOD,MOD,0,-MOD,0,MOD,MOD,0,0,0
ymod	dc.b	0,0,0,MOD,MOD,MOD,0,0,-MOD,0,-MOD,0,0,MOD,MOD,MOD

_placard
		movem.l	d0-d7/a0-a6,-(sp)
		lea		_rp_map,a1
		move.l	_GfxBase,a6

		moveq	#12,d6			; xorg
		moveq	#0,d7			; yorg

		moveq	#16,d4			; i
iiloop	move.l	d4,-(sp)
		moveq	#15,d5			; j
		lea		xmod(pc),a2		; table of adders
jloop	
		clr.l	d0
		move.l	d7,d3			; dy
		move.b	16(a2),d0		; dy + ymod[j]
		ext.w	d0
		ext.l	d0
		add.w	d0,d3

		move.l	d6,d2			; dx
		move.b	(a2)+,d0		; dx + xmod[j]
		ext.w	d0
		ext.l	d0
		add.w	d0,d2

		moveq	#4,d4			; k loop
kloop	moveq	#1,d0			; color
		tst.w	d4
		bne.s	kloop2
		add.w	#23,d0
kloop2	jsr		SetAPen(a6)

		move.l	(sp),d0
		cmp.l	#9,d0
		bls.s	kloop3

		move.l	d6,d0			; xorg
		move.l	d7,d1			; yorg
		jsr		Move(a6)
		move.l	d2,d0			; dx
		move.l	d3,d1			; dy
		jsr		Draw(a6)

		move.w	#284,d0
		sub.l	d6,d0			; 287-xorg
		move.w	#124,d1
		sub.l	d7,d1			; 124-yorg
		jsr		Move(a6)
		move.w	#284,d0
		sub.l	d2,d0			; 287-dx
		move.w	#124,d1
		sub.l	d3,d1			; 124-dy
		jsr		Draw(a6)
kloop3
		moveq	#16,d0
		add.l	d7,d0			; 16+yorg
		moveq	#12,d1
		sub.l	d6,d1			; 12-xorg
		jsr		Move(a6)
		moveq	#16,d0
		add.l	d3,d0			; 16+dy
		moveq	#12,d1
		sub.l	d2,d1			; 12-dx
		jsr		Draw(a6)

		move.w	#268,d0
		sub.l	d7,d0			; 268-yorg
		moveq	#112,d1
		add.l	d6,d1			; 112+xorg
		jsr		Move(a6)
		move.w	#268,d0
		sub.l	d3,d0			; 268-dy
		moveq	#112,d1
		add.l	d2,d1			; 112+dx
		jsr		Draw(a6)

		dbra	d4,kloop

		move.l	d2,d6			; xorg = dx
		move.l	d3,d7			; yorg = dy

		dbra	d5,jloop

		move.l	(sp)+,d4
		dbra	d4,iiloop

		movem.l	(sp)+,d0-d7/a0-a6
		rts

_move
			movem.l	a0-a2/d0-d1,-(sp)
			move.l	4+20(sp),d0
			move.l	8+20(sp),d1
			move.l	_rp,a1		; Rast Port (rp)
			move.l	_GfxBase,a6
			jsr		Move(a6)
			movem.l	(sp)+,a0-a2/d0-d1
			rts

_text
			movem.l	a0-a6/d0-d7,-(sp)
			move.l	60+4(sp),a0
			move.l	60+8(sp),d0
			move.l	_rp,a1		; Rast Port (rp)
			move.l	_GfxBase,a6
			jsr		Text(a6)
			movem.l	(sp)+,a0-a6/d0-d7
			rts

_ssp
			move.l	4(sp),a0
			movem.l	a0-a2/d0-d1,-(sp)
			move.l	_rp,a1		; Rast Port (rp)
ssp10
			move.l	a0,a2
			move.b	(a0)+,d0
			beq.s	sspx
			cmp.b	#XY,d0
			beq.s	setxy

; here's where we actually print the text

			move.l	a2,a0
			clr.l	d0
ssp20
			tst.b	(a2)+
			beq.s	endstring
			bmi.s	endstring
			addq	#1,d0		; add 1 to length
			bra.s	ssp20
endstring
			movem.l	a0-a6/d0-d7,-(sp)
			move.l	_GfxBase,a6
			jsr		Text(a6)
			movem.l	(sp)+,a0-a6/d0-d7
			add.w	d0,a0		; add length to pointer
			bra.s	ssp10

setxy		clr.l	d0
			clr.l	d1
			move.b	(a0)+,d0
			move.b	(a0)+,d1
			add.w	d0,d0		; x * 2
			move.l	_GfxBase,a6
			jsr		Move(a6)
			bra.s	ssp10
sspx
			movem.l	(sp)+,a0-a2/d0-d1
			rts			


		public	_px_to_im,_xreg,_yreg,_map_mem,_sector_mem
		public	_terra_mem

_px_to_im
		move.l	4(sp),d0			; x coord
		move.l	8(sp),d1			; y coord
px_to_im
		movem.l	d2-d4/a1,-(sp)

		move.b	#$80,d4				; 1 bit, high position
		btst	#3,d0				; test bit 3 of x
		beq.s	px01
		lsr.b	#4,d4				; select 1-4, 5-8
px01
		btst	#3,d1				; test bit 3 of y
		beq.s	px02
		lsr.b	#1,d4				; select 1/2
px02
		btst	#4,d1				; test bit 4 of x
		beq.s	px03
		lsr.b	#2,d4				; select 1-2, 3-4
px03

		lsr.w	#4,d0				; x / 16 = imx
		lsr.w	#5,d1				; y / 32 = imy

		move.w	d0,d2
		lsr.w	#4,d2				; secx = imx / 16 - xreg
		sub.w	_xreg,d2

		btst	#6,d2				; if 0-64, process normally
		beq.s	px20

		btst	#5,d2				; else wrap in varios directions
		seq		d2					; if zero, set to all 1's (mask 63)
		and.w	#63,d2				; else set to all zero's
px20

		move.w	d1,d3
		lsr.w	#3,d3				; secy = imy / 8 - yreg
		sub.w	_yreg,d3
		bpl.s	px30
		clr.w	d3					; if secy < 0, secy = 0;
px30	cmp.w	#32,d3
		blt.s	px40
		moveq	#31,d3				; if secy > 31, secx = 31
px40
		lsl.w	#7,d3				; sec_num = secy * 128 + secx + xreg
		add.w	d2,d3
		add.w	_xreg,d3			; add xreg

		move.l	_map_mem,a0
		move.l	_sector_mem,a1

		clr.w	d2
		move.b	(a0,d3),d2			; d2 = sec_num

		and.w	#15,d0				; imx & 15
		and.w	#7,d1				; imy & 7

		lsl.w	#7,d2				; sec_num * 128 + imy * 16 + imx
		lsl.w	#4,d1
		or.w	d1,d2
		or.w	d0,d2
		clr.w	d1
		move.b	(a1,d2.w),d1		; sector_mem[offset]

		clr.l	d0					; prepare zero result
		add.w	d1,d1
		add.w	d1,d1
		move.l	_terra_mem,a1
		and.b	2(a1,d1.w),d4		; tbit & tiles[image]
		beq.s	px99				; if no tile, return zero

		move.b	1(a1,d1.w),d0
		lsr.b	#4,d0				; terrain tile type
px99
		movem.l	(sp)+,d2-d4/a1	; pop 3 regs
		rts


;-----------------------------------------------------------------------
;		iff subroutines
;-----------------------------------------------------------------------
;	xdefs
		public	_ReadLength,_ReadHeader
;	xrefs
		public	_file_length,_blocklength,_myfile,_DOSBase,_header

_ReadHeader
		movem.l		d1-d3/a0-a1,-(sp)
		move.l		_DOSBase,a6
		move.l		_myfile,d1
		move.l		#_header,d2
		move.l		#4,d3
		jsr			Read(a6)
		sub.l		#4,_file_length
		movem.l		(sp)+,d1-d3/a0-a1
		rts

_ReadLength
		movem.l		d1-d3/a0-a1,-(sp)
		move.l		_DOSBase,a6
		move.l		_myfile,d1
		move.l		#_blocklength,d2
		move.l		#4,d3
		jsr			Read(a6)
		move.l		_blocklength,d1
		add.l		#4,d1
		sub.l		d1,_file_length
		movem.l		(sp)+,d1-d3/a0-a1
		rts

; this routine blits the map characters onto the screen
;		d1-d4 are offsets
;		d5 is screen plane offset
;		d7 is character number
;		d6 is height and scanline counter
;		a6 is address of char data
;		a5 is source image data
;		a0-a4 are addresses of bitplane areas

IPLAN_SZ	equ		16384			; 16K = 256 * 64

		public	_map_draw,_image_mem,_minimap,_planes
_map_draw
		movem.l	d0-d7/a0-a6,-(sp)

; initial register setups

		move.l	_image_mem,a6
		move.l	a6,-(sp)
		lea		_minimap,a6
		move.l	#IPLAN_SZ,d1		; 2nd plane offset
		move.l	#IPLAN_SZ*2,d2		; 3rd plane offset
		move.l	#IPLAN_SZ*3,d3		; 4th plane offset
		move.l	#IPLAN_SZ*4,d4		; 5th plane offset
		move.l	_planes,a4
		move.l	0(a4),a0			; plane 1
		move.l	4(a4),a1			; plane 2
		move.l	8(a4),a2			; plane 3
		move.l	12(a4),a3			; plane 4
		move.l	16(a4),a4			; plane 5

		clr.l	d7					; clear upper part

		move.w	#0,d5
		bsr.s	next_strip
		move.w	#2,d5
		bsr.s	next_strip
		move.w	#4,d5
		bsr.s	next_strip
		move.w	#6,d5
		bsr.s	next_strip
		move.w	#8,d5
		bsr.s	next_strip
		move.w	#10,d5
		bsr.s	next_strip
		move.w	#12,d5
		bsr.s	next_strip
		move.w	#14,d5
		bsr.s	next_strip
		move.w	#16,d5
		bsr.s	next_strip
		move.w	#18,d5
		bsr.s	next_strip
		move.w	#20,d5
		bsr.s	next_strip
		move.w	#22,d5
		bsr.s	next_strip
		move.w	#24,d5
		bsr.s	next_strip
		move.w	#26,d5
		bsr.s	next_strip
		move.w	#28,d5
		bsr.s	next_strip
		move.w	#30,d5
		bsr.s	next_strip
		move.w	#32,d5
		bsr.s	next_strip
		move.w	#34,d5
		bsr.s	next_strip
		move.w	#36,d5
		bsr.s	next_strip

		addq.l	#4,sp
		movem.l	(sp)+,d0-d7/a0-a6
		rts


; this will be a long set of in-line code

next_strip
		move.w	(a6)+,d7			; character number
		bsr.s	next_image
		move.w	(a6)+,d7			; character number
		bsr.s	next_image
		move.w	(a6)+,d7			; character number
		bsr.s	next_image
		move.w	(a6)+,d7			; character number
		bsr.s	next_image
		move.w	(a6)+,d7			; character number
		bsr.s	next_image
		move.w	(a6)+,d7			; character number
		bsr.s	next_image
		rts

next_image
;		rts
; check for zero (no update character)
; 		beq.s	no_image
		lsl.l	#6,d7				; char number * 64 bytes per char
		move.l	8(sp),a5			; a5 = image_mem + d7
;		add.l	d7,a5
		lea		(a5,d7.w),a5

		move.w	#15,d6
iloop
		move.w	(a5,d4.l),(a4,d5.w)	; move 5th bit plane
		move.w	(a5,d3.l),(a3,d5.w)	; move 4th bit plane
		move.w	(a5,d2.l),(a2,d5.w)	; move 3rd bit plane
		move.w	(a5,d1.l),(a1,d5.w)	; move 2nd bit plane
		move.w	(a5)+,(a0,d5.w)		; move 1st bit plane
		add.l	#40,d5				; next scan line

		move.w	(a5,d4.l),(a4,d5.w)	; move 5th bit plane
		move.w	(a5,d3.l),(a3,d5.w)	; move 4th bit plane
		move.w	(a5,d2.l),(a2,d5.w)	; move 3rd bit plane
		move.w	(a5,d1.l),(a1,d5.w)	; move 2nd bit plane
		move.w	(a5)+,(a0,d5.w)		; move 1st bit plane
		add.l	#40,d5				; next scan line

		dbra	d6,iloop

		clr.l	d7
		rts

no_image
		add.l	#1280,d5
		clr.l	d7
		rts

		public	_strip_draw
_strip_draw
		movem.l	d0-d7/a0-a6,-(sp)

		move.l	64(sp),d5			; strip number to draw

; initial register setups

		move.l	_image_mem,a6
		move.l	a6,-(sp)
		lea		_minimap,a6
		add.l	d5,a6				; add 6*d5 to a6
		add.l	d5,a6
		add.l	d5,a6
		add.l	d5,a6
		add.l	d5,a6
		add.l	d5,a6
		move.l	#IPLAN_SZ,d1		; 2nd plane offset
		move.l	#IPLAN_SZ*2,d2		; 3rd plane offset
		move.l	#IPLAN_SZ*3,d3		; 4th plane offset
		move.l	#IPLAN_SZ*4,d4		; 5th plane offset
		move.l	_planes,a4
		move.l	0(a4),a0			; plane 1
		move.l	4(a4),a1			; plane 2
		move.l	8(a4),a2			; plane 3
		move.l	12(a4),a3			; plane 4
		move.l	16(a4),a4			; plane 5

		clr.l	d7					; clear upper part

;		move.w	#0,d5
		bsr.s	next_strip

		addq.l	#4,sp
		movem.l	(sp)+,d0-d7/a0-a6
		rts

		public	_row_draw
_row_draw
		movem.l	d0-d7/a0-a6,-(sp)

; initial register setups

		move.l	64(sp),d0			; row number to draw

		move.l	_image_mem,a6
		move.l	a6,-(sp)
		lea		_minimap,a6
		add.l	d0,a6				; add row # to a6
		mulu.w	#640,d0				; offset into bit plane

		move.l	#IPLAN_SZ,d1		; 2nd plane offset
		move.l	#IPLAN_SZ*2,d2		; 3rd plane offset
		move.l	#IPLAN_SZ*3,d3		; 4th plane offset
		move.l	#IPLAN_SZ*4,d4		; 5th plane offset
		move.l	_planes,a4
		move.l	0(a4),a0			; plane 1
		move.l	4(a4),a1			; plane 2
		move.l	8(a4),a2			; plane 3
		move.l	12(a4),a3			; plane 4
		move.l	16(a4),a4			; plane 5

		clr.l	d7					; clear upper part

		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char
		addq.l	#2,d0
		bsr.s	next_char

		addq.l	#4,sp
		movem.l	(sp)+,d0-d7/a0-a6
		rts


; this will be a long set of in-line code

next_char
		move.l	d0,d5				; index the index
		move.w	(a6),d7				; character number
		bsr		next_image
		add.w	#12,a6				; next character in row
		rts

		public	_maskit,_bmask_mem,_shadow_mem

;		maskit(x,y,mod,char);

			public	_bigdraw,_secx,_secy
			public	_dbg
_bigdraw
			movem.l  d1-d7/a0-a6,-(sp) ; save regs
			move.l	_map_mem,a0
			move.l	_sector_mem,a1
			move.l	60(sp),d0		; map_x
			move.l	64(sp),d1		; map_y

;			move.l	#1,_dbg
			lsr.w	#8,d0
			sub.w	#9,d0			; map_x>>8 - 9
			sub.w	_xreg,d0

			lsr.w	#8,d1			; map_y>>8 - 4
			sub.w	#4,d1
			sub.w	_yreg,d1

			cmp.w	#32,d0
			bge.s	big10
			add.w	#128,d0
big10		cmp.w	#96,d0
			ble.s	big20
			sub.w	#128,d0
big20		cmp.w	#32,d1
			bge.s	big30
			add.w	#128,d1
big30		cmp.w	#96,d1
			ble.s	big40
			sub.w	#128,d1
big40		cmp.w	#0,d0
			bge.s	big50
			clr.w	d0
big50		cmp.w	#64-18,d0
			ble.s	big60
			move.w	#64-18,d0
big60		cmp.w	#0,d1
			bge.s	big70
			clr.w	d1
big70		cmp.w	#32-9,d1
			ble.s	big80
			move.w	#32-9,d1
big80
			move.l	d0,_secx
			move.l	d1,_secy

			clr.l	d7			; d7 is offset
			clr.w	d2			; d2 is j
big100
			move.w	d2,d4		; d4 is n = (secy+j)*128
			add.w	d1,d4
			lsl.w	#7,d4
			clr.w	d3			; d3 is i
big110
			move.w	_xreg,d5	; d5 is secx+i+xreg+n
			add.w	d0,d5		; +secx
			add.w	d3,d5		; +i
			add.w	d4,d5		; +n
			clr.l	d6			; d6 is map_mem[d5]
			move.b	(a0,d5.w),d6
			lsl.w	#7,d6		; * 128
			add.l	a1,d6		; d6 is address of sector in memory
			bsr		plotsect
			addq.l	#2,d7		; offset += 2

			addq	#1,d3
			cmp.w	#18,d3
			blt.s	big110

			add.l	#(40*15)+4,d7	; add to offset
			addq	#1,d2
			cmp.w	#9,d2
			blt.s	big100

			movem.l  (sp)+,d1-d7/a0-a6 ; restore regs
			rts

plotsect	movem.l  d0-d7/a0-a6,-(sp) ; save regs

			move.l	d6,a6			; sector data address
			move.l	d7,d0			; plane offset

			move.l	_planes,a0
			move.l	16(a0),a5		; plane 5
			move.l	12(a0),a3		; plane 4
			move.l	8(a0),a2		; plane 3
			move.l	4(a0),a1		; plane 2
			move.l	(a0),a0			; plane 0
			move.l	a0,_dbg
			add.l	d0,a0
			add.l	d0,a1
			add.l	d0,a2
			add.l	d0,a3
			add.l	d0,a5
			move.l	_terra_mem,a4

			move.w	#7,d7
scanlop
			move.w	#15,d6
colop
			clr.w	d5
			move.b	(a6)+,d5		; get image number
			add.w	d5,d5
			add.w	d5,d5
			move.b	3(a4,d5.w),d5	; get color used
			roxr.b	#1,d5			; bplane 1 color
			roxl.w	#1,d0
			roxr.b	#1,d5			; bplane 2 color
			roxl.w	#1,d1
			roxr.b	#1,d5			; bplane 3 color
			roxl.w	#1,d2
			roxr.b	#1,d5			; bplane 4 color
			roxl.w	#1,d3
			roxr.b	#1,d5			; bplane 5 color
			roxl.w	#1,d4

			dbra	d6,colop
			move.w	d0,(a0)
			move.w	d0,40(a0)
			add.w	#80,a0
			move.w	d1,(a1)
			move.w	d1,40(a1)
			add.w	#80,a1
			move.w	d2,(a2)
			move.w	d2,40(a2)
			add.w	#80,a2
			move.w	d3,(a3)
			move.w	d3,40(a3)
			add.w	#80,a3
			move.w	d4,(a5)
			move.w	d4,40(a5)
			add.w	#80,a5

			dbra	d7,scanlop

			movem.l  (sp)+,d0-d7/a0-a6 ; restore regs
			rts

; takes shape mask and adds character masks from background
; a5 = address of shape mask buffer mask
; d2 = modulus of mask
; d3 = x offset (in characters)
; d4 = y offset (in characters)
; d0 = character number
; a3 = image mask area

_maskit
		movem.l	d2-d6/a1/a0,-(sp)

		move.l	_bmask_mem,a0
		add.w	#2,a0
		move.l	_shadow_mem,a1

		move.l	32(sp),d3			; x offset
		move.l	36(sp),d4			; y offset
		move.l	40(sp),d2			; modulus
		move.l	44(sp),d0			; character
		add.l	d2,d2				; mod now in bytes

		lsl.l	#6,d0				; char number * 64 bytes per char
		lea		(a1,d0.w),a1

		mulu.w	d2,d4				; y*(32*mod) + x*2
		lsl.w	#5,d4				; times 32 scan lines
		add.w	d3,d4
		add.w	d3,d4
		lea		(a0,d4.w),a0		; place to do the masking

		move.w	#7,d6
mit10
		move.w	(a1)+,(a0)
		add.w	d2,a0
		move.w	(a1)+,(a0)
		add.w	d2,a0
		move.w	(a1)+,(a0)
		add.w	d2,a0
		move.w	(a1)+,(a0)
		add.w	d2,a0

		dbra	d6,mit10

		movem.l	(sp)+,d2-d6/a1/a0
		rts

		public	_mapxy
_mapxy
		move.l	4(sp),d0			; x coord (in images)
		move.l	8(sp),d1			; y coord (in images)
mapxy
		movem.l	d2-d4/a1,-(sp)

		move.w	d0,d2
		lsr.w	#4,d2				; secx = imx / 16 - xreg
		sub.w	_xreg,d2

		btst	#6,d2				; if 0-64, process normally
		beq.s	mxy20

		btst	#5,d2				; else wrap in varios directions
		seq		d2					; if zero, set to all 1's (mask 63)
		and.w	#63,d2				; else set to all zero's
mxy20
		move.w	d1,d3
		lsr.w	#3,d3				; secy = imy / 8 - yreg
		sub.w	_yreg,d3
		btst	#5,d3
		beq.s	mxy30
		btst	#4,d3
		seq		d3
		and.w	#31,d3
mxy30
		lsl.w	#7,d3				; sec_num = secy * 128 + secx + xreg
		add.w	d2,d3
		add.w	_xreg,d3			; add xreg

		move.l	_map_mem,a0
		move.l	_sector_mem,a1

		clr.w	d2
		move.b	(a0,d3),d2			; d2 = sec_num

		and.w	#15,d0				; imx & 15
		and.w	#7,d1				; imy & 7

		lsl.w	#7,d2				; sec_num * 128 + imy * 16 + imx
		lsl.w	#4,d1
		or.w	d1,d2
		or.w	d0,d2
		add.w	d2,a1				; sector_mem + offset
		move.l	a1,d0				; into return reg

		movem.l	(sp)+,d2-d4/a1
		rts

		public	_genmini,_hero_x,_hero_y,_hero_sector,_region_num
_genmini
		move.l	4(sp),d1		; parameter 1 - img_x
		move.l	8(sp),d0		; parameter 2 - img_y
		movem.l	d0-d7/a0-a2,-(sp)
		move.l	d0,d2
		move.l	_map_mem,a1
		move.l	_sector_mem,a2
		lea		_minimap,a0		; address of minimap
		clr.w	d0				; store register

		clr.w	d6				; for (i=0; i<19; i++)
gen10
		and.w	#$07ff,d1		; x = (img_x + i) & 0x7fff;
		move.w	d1,d3			; xs
		lsr.w	#4,d3
		sub.w	_xreg,d3		; xs = (x>>4) - xreg;
		btst	#6,d3				; if 0-64, process normally
		beq.s	gen16

		btst	#5,d3				; else wrap in varios directions
		seq		d3					; if zero, set to all 1's (mask 63)
		and.w	#63,d3				; else set to all zero's

gen16
		add.w	_xreg,d3		; xs += xreg;

		clr.w	d7				; for (j=0; j<6; j++)
gen20
		move.w	d2,d4			; y = img_y
		add.w	d7,d4			; y = (img_y + j) & 0x7fff;
		and.w	#$7fff,d4

		move.w	d4,d5			; ys = y
		lsr.w	#3,d5			; ys = (y>>3) - yreg;
		sub.w	_yreg,d5
		bpl.s	gen25
		clr.w	d5
gen25
		cmp.w	#32,d5
		blo.s	gen26			; if (ys > 31) ys = 31;
		move.w	#31,d5
gen26
		lsl.w	#7,d5			; sec_num = map_mem[xs + (ys<<7)];
		add.w	d3,d5
		move.b	(a1,d5),d0		; d5 = sec_num
		move.w	d0,d5

		lsl.w	#7,d5			; sec_num * 128

		and.w	#7,d4			; y & 7
		lsl.w	#4,d4			; y * 16
		add.w	d4,d5

		move.w	d1,d4			; d4 = x
		and.w	#$0f,d4			; & 15
		add.w	d4,d5			; d5 = sec_offset

		move.b	(a2,d5),d0		; map byte
		move.w	d0,(a0)+		; *mp++ = map byte

;			sec_offset = (sec_num<<7) + ((y & 7)<<4) + (x & 15);
;			*mp++ = sector_mem[sec_offset];

		addq	#1,d7
		cmp.w	#6,d7			; next j
		blt.s	gen20

		addq	#1,d1			; img_x++
		addq	#1,d6
		cmp.w	#19,d6			; next i
		blt.s	gen10

		clr.l	d0
		clr.l	d1
		clr.l	d2
		move.b	_hero_x,d0
		move.b	_hero_y,d1
		sub.w	_yreg,d1
		lsl.w	#7,d1
		add.w	d1,d0
		move.b	(a1,d0),d2
		move.w	_region_num,d0
		cmp.b	#7,d0
		bls.s	loc10
		add.w	#256,d2
loc10	move.w	d2,_hero_sector

		movem.l	(sp)+,d0-d7/a0-a2
		rts

		public	_unpack_line,_compress,_bytecount,_packdata

_unpack_line
		move.l	d2,-(sp)
		tst.b	_compress
		bne.s	unpack_line2
;_unpack_line1
		move.l	4+4(sp),a0	; a0 = dest
		move.l	_bytecount,d1
		move.l	_packdata,a1

		subq	#1,d1		; bytecount -1
		bmi.s	upl20
upl10	move.b	(a1)+,(a0)+
		dbra	d1,upl10
upl20	move.l	a1,_packdata
		move.l	(sp)+,d2
		rts

unpack_line2
		move.l	4+4(sp),a0		; a0 = dest
		move.l	_packdata,a1

		clr.l	d2				; j=0

upl30	clr.w	d0				; clear upper half
		move.b	(a1)+,d0		; d0 = upc
		bmi.s	upl35			; if j < 0 repeat else lit
upl32	addq.l	#1,d2			; add 1 to j
		move.b	(a1)+,(a0)+		; *des++ = *packdata++
		dbra	d0,upl32

		bra.s	upl39

upl35	neg.b	d0				; upc = -upc /* branch overflow?? */
		move.b	(a1)+,d3		; d3 = byte run byte
upl36	addq.l	#1,d2			; j++
		move.b	d3,(a0)+		; *dest++ = byte
		dbra	d0,upl36		; while (upc >= 0) loop

upl39	cmp.l	_bytecount,d2	; if (j<bytecount) continue;
		blo.s	upl30
upl40
		move.l	a1,_packdata
		move.l	(sp)+,d2
		rts

; newx(x,dir,speed)
; newy(y,dir,speed)

xdir	dc.w	-2,0,2,3,2,0,-2,-3,0,0
ydir	dc.w	-2,-3,-2,0,2,3,2,0,0,0

		public	_newx,_newy
_newx
		movem.l	d2/d3,-(sp)
		move.l	4+8(sp),d0	; x coord
		move.l	8+8(sp),d2	; direction
		cmp.b	#7,d2		; if direction greater than 7, exit
		bhi.s	newxx
		move.l	12+8(sp),d3	; speed

		lea		xdir,a0		; a0 = xdir
		add.b	d2,d2		; speed *2
		move.w	(a0,d2),d2	; xdir into d2
		muls.w	d2,d3		; *d3 (signed)
		lsr.w	#1,d3		; /2
		add.w	d3,d0		; + old x
		and.l	#$07fff,d0	; keep it on the map
newxx	movem.l	(sp)+,d2/d3
		rts

_newy
		movem.l	d2/d3,-(sp)
		move.l	4+8(sp),d0	; y coord
		move.l	8+8(sp),d2	; direction
		cmp.b	#7,d2		; if direction greater than 7, eyit
		bhi.s	newyy
		move.l	12+8(sp),d3	; speed

		move.l	d0,d1		; save high bit

		lea		ydir,a0		; a0 = ydir
		add.b	d2,d2		; speed *2
		move.w	(a0,d2),d2	; ydir into d2
		muls.w	d2,d3		; *d3 (signed)
		lsr.w	#1,d3		; /2
		add.w	d3,d0		; + old y
		and.l	#$07fff,d0	; keep it on the map
		and.w	#$8000,d1	; get high bit
		or.w	d1,d0		; and set it if need be
newyy	movem.l	(sp)+,d2/d3
		rts

;		public	_joyread,_xjoy,_yjoy

;_joyread
;		lea		$dff000,a0
;		move.b	12(a0),d0	L1
;		move.b	d0,d1
;		lsr.b	#1,d0
;		and.b	#1,d0
;		and.b	#1,d1		F1
;		eor.b	d0,d1

;		move.b	13(a0),d2	R1
;		move.b	d2,d3
;		lsr.b	#1,d2
;		and.b	#1,d2
;		and.b	#1,d3		B1
;		eor.b	d2,d3

;		sub.w	d0,d2
;		move.b	d2,_xjoy
;		sub.w	d1,d3
;		move.b	d3,_yjoy		

;		or.b	d2,d3
;		clr.l	d0
;		move.b	d3,d0
;		rts

		public	_wrap
_wrap
		move.l	4(sp),d0
		btst	#14,d0
		beq.s	wrap1
		or.w	#$8000,d0
		rts
wrap1	and.w	#$7fff,d0
		rts

		public	_map_adjust,_map_x,_map_y
_map_adjust
		move.l	4(sp),d0
		move.l	8(sp),d1
		movem.l	d2/d3,-(sp)
		sub.w	#144,d0
		sub.w	#70,d1
		move.w	d0,d2
		move.w	d1,d3

		sub.w	_map_x,d2
		sub.w	_map_y,d3

		lsl.w	#1,d2		; set sign bit according to bit 14
		asr.w	#1,d2
		lsl.w	#1,d3		; set sign bit according to bit 14
		asr.w	#1,d3

checkx
		cmp.w	#-70,d2
		bge.s	cx10
		add.w	#70,d0
		move.w	d0,_map_x
		bra.s	checky
cx10
		cmp.w	#70,d2
		ble.s	cx20
		sub.w	#70,d0
		move.w	d0,_map_x
		bra.s	checky
cx20
		cmp.w	#-20,d2
		bge.s	cx30
		sub.w	#1,_map_x
		bra.s	checky
cx30
		cmp.w	#20,d2
		ble.s	checky
		add.w	#1,_map_x
		bra.s	checky
checky
		cmp.w	#-24,d3
		bge.s	cy10
		add.w	#24,d1
		move.w	d1,_map_y
		bra.s	check99
cy10
		cmp.w	#44,d3
		ble.s	cy20
		sub.w	#44,d1
		move.w	d1,_map_y
		bra.s	check99
cy20
		cmp.w	#-10,d3
		bge.s	cy30
		sub.w	#1,_map_y
		bra.s	check99
cy30
		cmp.w	#10,d3
		ble.s	check99
		add.w	#1,_map_y
		bra.s	check99
check99
		movem.l	(sp)+,d2/d3
		rts

		public	_do_error
errms	dc.b	"ERROR "
_do_error
		move.l	4(sp),d0
		cmp.w	#2,d0		; can't print if GfxBase not open
		beq.s	doerrx
		pea		90
		pea		200
		bsr		_move

		pea		6
		pea		errms
		bsr		_text

		add.w	#16,sp

		move.l	4(sp),d0
		pea		2
		move.l	d0,-(sp)
		bsr		_prdec
		add.w	#8,sp

doerrx	rts

		public	_page_det
_page_det
		move.l	d1,-(sp)
		move.l	8(sp),d1
		move.l	a0,8(sp)

		lea		pd10(pc),a0
		clr.l	d0
		move.b	(a0,d1.w),d0
		cmp.w	#11,d1
		blt.s	pagex

		moveq	#10,d0
		cmp.w	#136,d1
		bgt.s	pagex

		moveq	#7,d0
		cmp.w	#135,d1
		bgt.s	pagex

		moveq	#6,d0
		cmp.w	#123,d1
		bgt.s	pagex

		moveq	#5,d0
		cmp.w	#98,d1
		bgt.s	pagex

		moveq	#4,d0
		cmp.w	#71,d1
		bgt.s	pagex

		moveq	#3,d0
pagex	
		move.l	8(sp),a0
		move.l	(sp)+,d1
		rts
pd10	dc.b	9,9,8,7,6,5,5,5,4,4,4
com2	dc.b	0,1,2,7,9,3,6,5,4

		public	_decode_mouse,_handler_data,_keydir,_oldir,_drawcompass
_decode_mouse
		movem.l	a0/d0-d4,-(sp)

		lea		_handler_data,a0
		move.w	qualifier(a0),d2
		and.w	#$6000,d2			; test for buttons down
		beq.s	decodejoy

		move.w	xsprite(a0),d0		; mouse pointer x
		move.w	ysprite(a0),d1		; mouse pointer y

		moveq	#9,d2				; no direction
		cmp.w	#265,d0
		ble.s	setcomp

; left column
		moveq	#0,d2				; up
		moveq	#7,d3				; center
		moveq	#6,d4				; down
		cmp.w	#292,d0
		blt.s	findy
; right column
		moveq	#2,d2				; up
		moveq	#3,d3				; center
		moveq	#4,d4				; down
		cmp.w	#300,d0
		bgt.s	findy
; middle column
		moveq	#1,d2				; up
		moveq	#9,d3				; center
		moveq	#5,d4				; down
findy
		cmp.w	#166,d1				; d2 already equals upper
		blt.s	setcomp
		move.w	d4,d2				; d2 now equals lower
		cmp.w	#174,d1
		bgt.s	setcomp
		move.w	d3,d2
		bra.s	setcomp

decodejoy						; read joystick
		lea		$dff000,a0		; sysregs
		move.b	12(a0),d0	L1	; joy1 register
		move.b	d0,d1			; make copy
		lsr.b	#1,d0			; get left bit
		and.b	#1,d0
		and.b	#1,d1		F1	; get forward bit
		eor.b	d0,d1			; combine the bits (1,0,-1)

		move.b	13(a0),d2	R1	; joy2 register - right bit
		move.b	d2,d3
		lsr.b	#1,d2
		and.b	#1,d2
		and.b	#1,d3		B1	; back bit
		eor.b	d2,d3			; combine the bits (1,0,-1)

		sub.b	d0,d2			d2 = xjoy
		sub.b	d1,d3			d3 = yjoy

		move.b	d3,d0			; (if joystick center)
		or.b	d2,d0			; if both zeroes then try reading keyboard
		beq.s	decodekey

		moveq	#4,d0			; d0 = 4 + xjoy*3 + yjoy
		add.b	d3,d0			; this formula gives a unique number
		add.b	d3,d0			;	for each joystick position
		add.b	d3,d0			;	between 0 and 7 - indexed to:
		add.b	d2,d0			;		0 1 2
		ext.w	d0				;		7 9 3
		lea		com2(pc),a0		;		6 5 4
		clr.l	d2
		move.b	(a0,d0.w),d2	; joystick direction
		move.w	#1,_keydir		; into keydir
		bra.s	setcomp

decodekey
		clr.l	d2
		move.w	_keydir,d2	; if key => 20 && key < 30 then dir = key-20
		sub.w	#20,d2
		bmi.s	decodenull
		cmp.w	#10,d2
		bge.s	decodenull
		bra.s	setcomp
		
decodenull
		moveq	#9,d2
		clr.w	_keydir
		
setcomp
		cmp.w	_oldir,d2			; if dir != oldir
		beq.s	setcompx
		move.w	d2,_oldir			; oldir = dir
		move.l	d2,-(sp)
		jsr		_drawcompass		; drawcompass(dir)
		addq	#4,a7
setcompx
		movem.l	(sp)+,a0/d0-d4
		rts

		public	_prox

_prox
		move.l	4(sp),d0
		move.l	8(sp),d1
		addq	#4,d0				; x + 4
		addq	#2,d1				; y + 2
		bsr		px_to_im
		cmp.w	#1,d0				; if blocked
		beq.s	prox99
		cmp.w	#10,d0				; if n.a. blocked
		bge.s	prox99

		move.l	4(sp),d0
		move.l	8(sp),d1
		subq	#4,d0				; x + 4
		addq	#2,d1				; y + 2
		bsr		px_to_im
		cmp.w	#1,d0				; if blocked
		beq.s	prox99
		cmp.w	#8,d0				; if n.a. blocked
		bge.s	prox99
		clr.l	d0
prox99	rts

		public	_make_mask

; dynamically creates shape mask from shape data
;	assumes color 31 = transparent
;	make_mask(source,dest,words,scans,chars);

_make_mask
		movem.l	d1-d4/a0-a5,-(sp)

		move.l	44(sp),a4		; source data
		move.l	48(sp),a5		; dest data
		move.l	52(sp),d4		; width in words
		move.l	56(sp),d3		; length in scanlines
		move.l	60(sp),d2		; = char count

		subq.w	#1,d2			; count - 1
		mulu.w	d3,d4			; d5 = width * scans
		move.w	d4,d1			; get width
		add.l	d1,d1			; plane length in bytes

mm00
		move.l	a4,a0			; start at pointer from last
		lea		(a0,d1.l),a1	; pointer to next plane
		lea		(a1,d1.l),a2	; pointer to next plane
		lea		(a2,d1.l),a3	; pointer to next plane
		lea		(a3,d1.l),a4	; pointer to next plane

		move.w	d4,d3			; get width
		subq.w	#1,d3			; = word count of plane
mm05
		move.w	(a0)+,d0		; take AND of all planes
		and.w	(a1)+,d0
		and.w	(a2)+,d0
		and.w	(a3)+,d0		
		and.w	(a4)+,d0
		not.w	d0				; it's an inverted mask
		move.w	d0,(a5)+		; and store in destination

		dbra	d3,mm05
		dbra	d2,mm00

		movem.l	(sp)+,d1-d4/a0-a5
		rts

; this routine scrolls the screen very fast using only two blit channels
;		on entry, a0 contains the harware regs, a1 address of bltnode

		public	_scrollmap

bltnode		equ		0
pl0			equ		18
plcount		equ		22
plmode		equ		24

ishape		equ		18		; pointer to sshape struct
planes		equ		22
pcount		equ		26		; plane counter
savemem		equ		28		; backsave area
mbuff		equ		28		; mask area
shapedata	equ		28
maskdata	equ		32

bc0		equ		$09F0		; 
;bc1 = (mode type>>1) & 2   ; no shift, no fill/line, maybe descending

DMACONR		equ		2

BLTAPT		equ		$50
BLTBPT		equ		$4C
BLTCPT		equ		$48
BLTDPT		equ		$54

BLTCON0		equ		$40
BLTCON1		equ		$42

BLTAFWM		equ		$44
BLTALWM		equ		$46

BLTSIZE		equ		$58

BLTAMOD		equ		$64
BLTBMOD		equ		$62
BLTCMOD		equ		$60
BLTDMOD		equ		$66

vmod		equ		4			; modulus for vertical & diagonal
hmod		equ		2			; modulus for horizontal
hdelta		equ		2			; horizontal movement offset (bytes)
vsc			equ		32			; number of scanlies to a character
sbytes		equ		40			; number of bytes per scan
swords		equ		20			; number of words per scan
vdelta		equ		vsc*sbytes	; vertical movement offset (bytes)
len5		equ		5*vdelta	; vertical offset of 5 chars
len6		equ		6*vdelta	; vertical offset of 6 chars
scan5		equ		5*vsc		; number of scanlines to 5 characters
scan6		equ		6*vsc		; number of scanlines to 6 characters

scroll_table					; table of constants for blitting
;	constants are Aoffset, Doffset, modulus, blitsize --- entry is 8 bytes	
			dc.w	len6-6,len6+2-6
			dc.w	vmod,(scan6*64)+18

			dc.w	len5-6,len5+1282-6
			dc.w	vmod,(scan5*64)+18

			dc.w	len5-4,len5+vdelta-4
			dc.w	hmod,(scan5*64)+19

			dc.w	len5+2-6,len5+vdelta-6
			dc.w	vmod,(scan5*64)+18

			dc.w	2,0
			dc.w	vmod,(scan6*64)+18

			dc.w	1282,0
			dc.w	vmod,(scan5*64)+18

			dc.w	vdelta,0
			dc.w	hmod,(scan5*64)+19

			dc.w	vdelta,2
			dc.w	vmod,(scan5*64)+18

_scrollmap
		movem.l	a2-a6/d1-d2,-(sp)

		move.l	_GfxBase,a6
		jsr		OwnBlitter(a6)

		move.l	#$dff000,a0
		moveq	#4,d2				; 5 bitplanes

; get plane pointers
2$		move.w	d2,d0
		add.w	d0,d0				; d0 times 4
		add.w	d0,d0				; eaddress = sdata + pl0 + plcount*4
		move.l	_planes,a2			; plane = pointer to Planes
		move.l	(a2,d0.w),a2		; a2 = pointer to source plane
		move.l	a2,a3				; a3 = pointer to dest plane

		btst	#6,DMACONR(a0)		; wait for blitter free
		btst	#6,DMACONR(a0)
		beq.s	1$
		move.l	_GfxBase,a6
		jsr		WaitBlit(a6)
1$

; set bltcon1 * 0
		move.l	8*4(sp),d0			; direction of scroll, 0-7

		move.w	d0,d1				; make a copy
		lsr.w	#1,d1				; divide by two
		not.w	d1					; complement
		and.w	#$0002,d1			; bit 1 is ascending/descending
		move.w	d1,BLTCON1(a0)		; set BLTCON1
		move.w	#bc0,BLTCON0(a0)	; set BLTCON0 to constant

; set masks
		move.w	#$FFFF,BLTAFWM(a0)	; set masks wide open
		move.w	#$FFFF,BLTALWM(a0)

; get mode data
		lsl.w	#3,d0				; * 8
		lea		scroll_table,a5		; waste yet another register
		add.w	d0,a5				; a4 now points to const data
		add.w	(a5)+,a2			; source pointer offset
		add.w	(a5)+,a3			; dest pointer offset
		move.l	a2,BLTAPT(a0)		; set A pointer
		move.l	a3,BLTDPT(a0)		; set B pointer
		move.w	(a5),BLTAMOD(a0)	; set A modulus
		move.w	(a5)+,BLTDMOD(a0)	; set D modulus
		move.w	(a5),BLTSIZE(a0)	; DO IT NOW!!!

		dbf		d2,2$

		move.l	_GfxBase,a6
		jsr		DisownBlitter(a6)
		movem.l	(sp)+,a2-a6/d1-d2	; restore regs
		rts

		public	_clear_blit

; blit clear an area of memory

cmem		equ		18
csiz		equ		22

_clear_blit
		move.l	#$dff000,a0
		move.w	#$0100,BLTCON0(a0)	; use D only, miniterm = 0
		move.w	#$0000,BLTCON1(a0)	; no options
		move.w	#0,BLTDMOD(a0)		; no modulus
		move.l	4(sp),BLTDPT(a0)	; set destination to cmem
		move.w	10(sp),BLTSIZE(a0)	; set size and DO IT!
		move.w	#0,d0
		rts							; look ma! no registers!

; character blitting - 4 blits per character
;	back restore	(5 planes)
;	backsave		(5 planes)
;	mask create		(1 plane)
;	shape draw		(5 planes)

;	total 			(16 planes) per character drawn.

; definition of sshape structure

; backsave takes 4 bytes
savesize	equ		4
blitsize	equ		6
Coff		equ		8
Cmod		equ		10

Aoff		equ		38
Boff		equ		40
Bmod		equ		42
planesize	equ		44
shift		equ		36
wmask		equ		46

		public		_shape_blit,_shp,_shapedata,_mask_buffer
		public		_planesize,_wmask,_aoff,_boff,_bmod,_shift

_shape_blit
		movem.l	a2-a4/a6/d1-d3,-(sp)

		move.l	#$dff000,a0
		move.l	_shp,a3				; a3 points to sshape struct

		moveq	#4,d3				; 5 bitplanes to do

; get plane pointer
sb05
		move.w	d3,d0
		add.w	d0,d0				; d0 times 4
		add.w	d0,d0				; eaddress = sdata + pl0 + pcount*4
		move.l	_planes,a2
		move.l	(a2,d0.w),a2		; a2 = pointer to dest plane

		btst	#6,DMACONR(a0)		; wait for blitter free
		btst	#6,DMACONR(a0)
		beq.s	1$
		move.l	_GfxBase,a6
		jsr		WaitBlit(a6)
1$
		move.w	_shift,d1			; d1 = x starting loc & 15
		move.w	d1,d2				; save shift
		lsl.w	#6,d1
		lsl.w	#6,d1				; d1 << 12
		move.w	d1,BLTCON1(a0)		; set B shift value
		move.w	#$0FCA,BLTCON0(a0)	; set miniterm to D = B

; set masks
		move.w	#$FFFF,d1
		lsr.w	d2,d1
		move.w	d1,d2
		and.w	_wmask,d2
		move.w	d2,BLTAFWM(a0)
		not.w	d1
		bne.s	sb10
		move.w	#-1,d1
sb10
		move.w	d1,BLTALWM(a0)

		add.w	Coff(a3),a2			; BLTDPT = plane + Coff
		move.l	a2,BLTDPT(a0)
		move.l	a2,BLTCPT(a0)

		move.l	_shapedata,a2
		add.w	_boff,a2
		move.w	_planesize,d1
		mulu.w	d3,d1
		add.w	d1,a2
		move.l	a2,BLTBPT(a0)

		move.l	_bmask_mem,a2
		addq	#2,a2				; a2 = pointer to bmask_mem + 2
		add.w	_aoff,a2
		move.l	a2,BLTAPT(a0)		; BLTAPT = mask + Boff

		move.w	#0,BLTAMOD(a0)		; set mods
		move.w	_bmod,BLTBMOD(a0)
		move.w	Cmod(a3),BLTCMOD(a0)
		move.w	Cmod(a3),BLTDMOD(a0)

		move.w	blitsize(a3),BLTSIZE(a0)	; DO IT NOW!!!

		dbf		d3,sb05
		movem.l	(sp)+,a2-a4/a6/d1-d3		; restore regs
		rts

		public	_mask_blit

; this routine creates the composite backround mask

_mask_blit
		movem.l	a2-a4/a6/d1-d3,-(sp)

		move.l	#$dff000,a0
		move.l	_shp,a3				; a3 points to sshape struct

		btst	#6,DMACONR(a0)		; wait for blitter free
		btst	#6,DMACONR(a0)
		beq.s	1$
		move.l	_GfxBase,a6
		jsr		WaitBlit(a6)
1$
		move.l	_bmask_mem,a2
		addq	#2,a2				; a2 = pointer to bmask_mem + 2
		add.w	_aoff,a2
		move.l	a2,BLTDPT(a0)		; BLTDPT = dest + Aoff
		move.l	a2,BLTCPT(a0)		; BLTCPT = same

		move.l	_mask_buffer,a2		; a2 = pointer to character masks
		add.w	_boff,a2
		move.l	a2,BLTAPT(a0)		; BLTAPT = mask + Boff

		move.w	_shift,d1			; d1 = x starting loc & 15
		move.w	d1,d2				; save shift
		lsl.w	#6,d1
		lsl.w	#6,d1				; d1 << 12
		or.w	#$0b50,d1			; set miniterm to D = Ac
		move.w	d1,BLTCON0(a0)
		move.w	#0,BLTCON1(a0)		; set B shift value = 0

		move.w	#$FFFF,d1
		lsr.w	d2,d1
		move.w	#$FFFF,BLTAFWM(a0)	; set masks wide open
		move.w	#$FFFF,BLTALWM(a0)

		move.w	_bmod,BLTAMOD(a0)			; set mods
		move.w	#0,BLTDMOD(a0)
		move.w	#0,BLTCMOD(a0)
		move.w	blitsize(a3),BLTSIZE(a0)	; DO IT NOW!!!

		movem.l	(sp)+,a2-a4/a6/d1-d3		; restore regs
		rts

		public		_save_blit

_save_blit
		movem.l	a2-a4/a6/d1-d3,-(sp)

		move.l	#$dff000,a0
		move.l	_shp,a3				; a3 points to sshape struct
		moveq	#4,d2				; 5 bitplanes to do

; get plane pointers
2$
		move.w	d2,d0
		add.w	d0,d0				; d0 times 4
		add.w	d0,d0				; eaddress = sdata + pl0 + pcount*4
		move.l	_planes,a2
		move.l	(a2,d0.w),a2		; a2 = pointer to dest plane

		btst	#6,DMACONR(a0)	; wait for blitter free
		btst	#6,DMACONR(a0)
		beq.s	1$
		move.l	_GfxBase,a6
		jsr		WaitBlit(a6)
1$
		move.w	#0,BLTCON1(a0)		; set B shift value
		move.w	#$05CC,BLTCON0(a0)	; set miniterm to D = B

; set masks
		move.w	#$ffff,BLTAFWM(a0)	; set masks wide open
		move.w	#$ffff,BLTALWM(a0)

		add.w	Coff(a3),a2			; BLTDPT = plane + Coff
		move.l	a2,BLTBPT(a0)

		move.l	8*4(sp),a2			; BLTDPT = savemem
		move.w	savesize(a3),d1
		mulu.w	d2,d1
		add.w	d1,a2
		move.l	a2,BLTDPT(a0)

		move.w	Cmod(a3),BLTBMOD(a0)
		move.w	#0,BLTDMOD(a0)
		move.w	blitsize(a3),BLTSIZE(a0)	; DO IT NOW!!!

		dbf		d2,2$
		movem.l	(sp)+,a2-a4/a6/d1-d3		; restore regs

		rts

		public		_rest_blit

_rest_blit
		movem.l	a2-a4/a6/d1-d3,-(sp)

		move.l	#$dff000,a0
		move.l	_shp,a3				; a3 points to sshape struct

		moveq	#4,d2				; 5 bitplanes to do

; get plane pointers
2$
		move.w	d2,d0
		add.w	d0,d0				; d0 times 4
		add.w	d0,d0				; eaddress = sdata + pl0 + pcount*4
		move.l	_planes,a2
		move.l	(a2,d0.w),a2		; a2 = pointer to dest plane

		move.l	_shp,a3			; a3 points to sshape struct

		btst	#6,DMACONR(a0)	; wait for blitter free
		btst	#6,DMACONR(a0)
		beq.s	1$
		move.l	_GfxBase,a6
		jsr		WaitBlit(a6)
1$
		move.w	#0,BLTCON1(a0)		; set B shift value
		move.w	#$05CC,BLTCON0(a0)	; set miniterm to D = B

; set masks
		move.w	#$ffff,BLTAFWM(a0)	; set masks wide open
		move.w	#$ffff,BLTALWM(a0)

		add.w	Coff(a3),a2			; BLTDPT = plane + Coff
		move.l	a2,BLTDPT(a0)

		move.l	8*4(sp),a2			; BLTDPT = savemem
		move.w	savesize(a3),d1
		mulu.w	d2,d1
		add.w	d1,a2
		move.l	a2,BLTBPT(a0)

		move.w	Cmod(a3),BLTDMOD(a0)
		move.w	#0,BLTBMOD(a0)
		move.w	blitsize(a3),BLTSIZE(a0)	; DO IT NOW!!!

		dbf		d2,2$

		movem.l	(sp)+,a2-a4/a6/d1-d3		; restore regs
		rts

		end
