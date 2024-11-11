	public	_colorplay
_colorplay
	movem.l	d4/d5,-(sp)
	move.l	#31,d5
1$
	lea		_fader,a0
	moveq.l	#31,d4
2$
	jsr	_rand				; get a random color
	and.l	#$0fff,d0		; bottom three nibbles
	move.w	d0,(a0)+
	dbra.l	d4,2$

	pea	32					; 32 colors
	pea	_fader				; from fader
	pea	_vp_page			; into viewwport
	jsr	_LoadRGB4			; load 'em
	pea	1					; 1/60th sec
	jsr	_Delay				; delay
	lea	16(sp),sp			; clean up from both calls

	dbra.l	d5,1$			; do 32 times

	movem.l	(sp)+,d4/d5
	rts

	public	_stillscreen
_stillscreen
	move.l	_fp_drawing,a0	; get fp_page
	move.l	(a0),a1			; get rasinfo
	clr.l	8(a1)			; clear dx and dyoffset
	jsr		_pagechange		; change pages
	rts

	public	_skipint
_skipint
	jsr	_getkey				; get a key
	cmp.l	#32,d0			; if = space
	seq		d0				; set d0 to all 1's
	move.b	d0,_skipp		; put into skipp
	rts						; d0 is now 000000ff or 00000000

