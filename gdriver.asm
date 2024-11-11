*	This will be my attempt to create audio interrupt handlers for the
*	four-voice music program.

			cseg

RemIntServer	EQU	$FFFFFF52
AddIntServer	EQU	$FFFFFF58
SetIntVector	EQU	$FFFFFF5E

INTENA		equ		$9a
INTENAR		equ		$1c
INTREQ		equ		$9c
INTREQA		equ		$1e

			public		_vblank_data

nosound		equ	0 			; sound inhibit byte
flag_codes	equ 1			; sync byte
tempo		equ	2			; tempo counter
ins_handle	equ	4
vol_handle	equ	8
wav_handle	equ	12
timeclock	equ	16			; we can still use this for music timing
vbase		equ	24

wave_num	equ 0+vbase		; char index to waveform number
vol_num		equ 1+vbase		; char index to envelope number
vol_delay	equ 2+vbase		; char status volume
vce_stat	equ 3+vbase		; effect/rest status
event_start equ 4+vbase		; timer for next event
event_stop  equ 8+vbase		; time to stop this event
vol_list	equ	12+vbase	; pointer to volume table
trak_ptr	equ	16+vbase	; pointer to trak (playback)
trak_beg	equ 20+vbase	; pointer to trak (begin)
trak_stk	equ	24+vbase	; pointer to loop stack
voice_sz	equ	28

; bits for vce_stat

TIE			equ		4		; tied note
CHORD		equ		8		; chorded note
REST		equ		16		; rest
ENDTK		equ		32		; trak ended

voice1		equ	0			; voice offsets for above
voice2		equ	voice_sz
voice3		equ	voice_sz*2
voice4		equ	voice_sz*3

; register usage - 
;	a0 - pointer to sysregs
;	a1 - pointer to data area
;	a2 - score pickup
;	a3 - pointer to voice data
;	a4 - pointer to audio registers

_vblank_server
			movem.l	d0-d5/a0-a5,-(sp)

			clr.b	flag_codes(a1)	; tell if we have reached vblank

; we have to update tempo clock regardless of what else is happening, because
;	other parts of the program use it for a timer.

			clr.l	d0
			move.w	tempo(a1),d0
			add.l	d0,timeclock(a1)	; add tempo to time clock counter

; if there is no score currently playing, then exit

			tst.b	nosound(a1)	; NOSOUND byte
			bne	exit1		; inhibits sound playing

			move.l	a1,a3	; a3 points to voice now
;			move.l	a0,a4	; a4 points to sysregs (audio regs)
			move.l	#$00dff000,a4	; a4 points to sysregs (audio regs)
			bsr		dovoice
			bsr		dovoice
			bsr		dovoice
			bsr		dovoice
exit1		moveq	#0,d0			; continue server chain
			movem.l	(sp)+,d0-d5/a0-a5
			rts						; nothing more to do, really

dovoice		tst.l	trak_ptr(a3)	; check read head for presence of trak
			beq		vocx

			move.l	timeclock(a1),d0	; current time reading
			cmp.l	event_start(a3),d0	; next event scheduled
			bpl.s	newnote

			tst.b	vce_stat(a3)	; if sample going on,
			bne		vocx			; don't interrupt it

			cmp.l	event_stop(a3),d0	; end of event scheduled
			bpl.s	rest_env

			tst.b	vol_delay(a3)	; delay value
			bne.s	vocx
			move.l	vol_list(a3),a2	; pointer to envelope
			move.b	(a2)+,d0		; d0 holds next volume value
			bmi.s	vocx			; if negative, leave alone
			move.b	d0,$a8(a4)		; set volume register
			move.l	a2,vol_list(a3)	; reset pointer
			bra.s	vocx			; next voice

rest_env	
			move.b	#0,$a8(a4)		; set volume register to zero
			bra.s	vocx			; next trak

dorestnote	
			move.b	#0,$a8(a4)		; clear volume
			move.l	timeclock(a1),event_stop(a3)	; KLUGE!!
			bra.s	vocx
 
newnote
			move.l	trak_ptr(a3),a2	; read head
			clr.l	d2				; clear top d2
			clr.l	d3
			move.b	(a2)+,d3		; get command byte
			move.b	(a2)+,d2		; get value byte
			move.l	a2,trak_ptr(a3)	; save read head

			btst	#7,d3			; is it a note command?
			beq		note_comm

			cmp.b	#128,d3			; rest
			beq		note_comm

			cmp.b	#129,d3			; instrument
			beq		set_voice

			cmp.b	#$90,d3			; NOT SMUS STANDARD
			beq		set_tempo		; set tempo number

			cmp.b	#255,d3			; endscore
			beq		endtrak

			bra		newnote			; otherwise end trak

note_comm
			bclr	#7,d2			; ignore chorded notes
			bclr	#6,d2			; don't ignore ties
			; set flags if tied ???
			lea		notevals(pc),a5
			add.w	d2,d2			; times 2
			clr.l	d5
			move.w	(a5,d2),d5		; note length in counts
			move.l	d5,d4
			sub.l	#300,d4			; gap size
			bpl.s	nc1				; if note smaller than gap
			move.l	d5,d4			; then no gap
nc1			add.l	event_start(a3),d4	; event stop
			move.l	d4,event_stop(a3)
			add.l	d5,event_start(a3)	; event start recalculated

			tst.b	vce_stat(a3)	; if sample, don't mess with it
			bne.s	vocx

; first get address of waveform

			clr.l	d4
			move.b	wave_num(a3),d4		; d4 = waveform number
			lsl.w	#7,d4
			add.l	wav_handle(a1),d4		; d4 = pointer to waveform

			clr.l	d5
			move.b	vol_num(a3),d5		; d5 = volume number
			lsl.w	#8,d5
			add.l	vol_handle(a1),d5		; d5 = pointer to volume
			move.l	d5,a5				; a5 = pointer to volume
			clr.w	d5
			move.b	(a5)+,d5			; d5 = starting vol level
			move.l	a5,vol_list(a3)		; set volume read pointer
			clr.w	vol_delay(a3)		; clear volume delay, vce_stat

			tst.b	d3				; 128 = rest
			bmi.s	dorestnote		; do a rest
;			bra.s	dorestnote		; do a rest

 			lsl.w	#2,d3			; note value * 4
			move.w	ptable+2(pc,d3),d2	; d2	= offset value
			move.w	ptable(pc,d3),d1	; d1	= period value
			move.w	#32,d3			; d3 = 32 - offset (words)
			sub.w	d2,d3			; d3 = length of waveform
			lsl.w	#2,d2			; d2 = offset * 4
			and.l	#$ffff,d2		; clear upper half d2
			add.l	d2,d4			; add offset to d4 ( = wave)

			move.w	d5,$a8(a4)		; set initial volume
			move.l	d4,$a0(a4)		; move waveform to register
			move.w	d3,$a4(a4)		; move wavelength to register
			move.w	d1,$a6(a4)		; move period to register
	
;			bra	vocx				; next note

vocx		add.w	#voice_sz,a3	; next voice
			add.w	#16,a4			; next audio reg

			rts

ptable
*			dc.w	1016,00,960,00,906,00,856,00,808,00,762,00
			dc.w	1440,00,1356,00,1280,00,1208,00,1140,00,1076,00

			dc.w	1016,00,960,00,906,00,856,00,808,00,762,00
			dc.w	720,00,678,00,640,00,604,00,570,00,538,00

			dc.w	508,00,480,00,453,00,428,00,404,00,381,00
			dc.w	360,00,339,00,320,00,302,00,285,00,269,00

			dc.w	508,16,480,16,453,16,428,16,404,16,381,16
			dc.w	360,16,339,16,320,16,302,16,285,16,269,16
	
			dc.w	508,24,480,24,453,24,428,24,404,24,381,24
			dc.w	360,24,339,24,320,24,302,24,285,24,269,24

			dc.w	508,28,480,28,453,28,428,28,404,28,381,28
			dc.w	360,28,339,28,320,28,302,28,285,28,269,28

			dc.w	254,28,240,28,226,28,214,28,202,28,190,28
			dc.w	180,28,170,28,160,28,151,28,143,28,135,28

; these are the timing values for interpreting an SMUS score.
;

notevals
			dc.w	26880,13440,6720,3360,1680,840,420,210
			dc.w	40320,20160,10080,5040,2520,1260,630,315
			dc.w	17920,8960,4480,2240,1120,560,280,140
			dc.w	26880,13440,6720,3360,1680,840,420,210
			dc.w	21504,10752,5376,2688,1344,672,336,168
			dc.w	32256,16128,8064,4032,2016,1008,504,252
			dc.w	23040,11520,5760,2880,1440,720,360,180
			dc.w	34560,17280,8640,4320,2160,1080,540,270

set_voice
			and.l	#$000f,d2			; clear top byte of word
			add.w	d2,d2				; * 2
			move.l	ins_handle(a1),a2		; instrument handle
			move.w	(a2,d2),wave_num(a3)
			bra		newnote				; process more code

set_tempo	
			and.l	#$00ff,d2			; clear top byte of word
			move.w	d2,tempo(a1)
			bra		newnote				; process more code

endtrak
			tst.b	d2
			bne.s	repeat
			clr.l	trak_ptr(a3)	; no trak
			move.b	#-1,vol_delay(a3)	; no volume changes
			move.w	#0,$a8(a4)		; clear out volume
			bra		vocx			; next trak

repeat		move.l	trak_beg(a3),trak_ptr(a3)
			bra		newnote

; interrupt routine for write

audio_int
			movem.l	a2/d1,-(sp)
			move.w	#%0000000100000000,INTENA(a0) ; reset bit 0
			move.w	#%0000000100000000,INTREQ(a0) ; reset bit 0 of req

			lea		vce_stat+voice2(a1),a2	; track status byte

			tst.b	(a2)
			beq.s	audx1	; if no sample, continue and turn int off
			subq.b	#1,(a2)				; decrement vce_status
			bne.s	audx	; if sample not done, continue play

			move.w	#0,$b8(a0)			; shut off sample (reps?)
			move.w	#2,$b6(a0)			; move period to register
			beq.s	audx1				; don't re-enable interrupt
audx
			move.w	#%1000000100000000,INTENA(a0) ; set bit 0
audx1
			movem.l	(sp)+,a2/d1
			rts

;	non-interrupt routines

			public	_stopscore
			public	_playscore
			public	_setscore
			public	_init_music
			public	_wrap_music
			public	_playsample
			public	_stopsample
			public	_set_tempo

; playsample(effect,length,rate)

_playsample	
			move.l	d2,-(sp)
			move.l	#$dff000,a0			; pointer to sysregs
			move.w	#$0002,$96(a0)		; turn it off

			move.w	#2,$b6(a0)			; move period to register

			move.l	4+4(sp),d0			; effect address
			move.l	8+4(sp),d1			; effect length
			move.l	12+4(sp),d2			; effect rate

			lea		_vblank_data,a1

			move.b	#2,vce_stat+voice2(a1)	; disable note playing
			move.b	#-1,vol_delay+voice2(a1)	; no volume change till next note
			move.w	#64,$b8(a0)			; set initial volume
			move.l	d0,$b0(a0)			; move waveform to register
			move.w	d1,$b4(a0)			; move wavelength to register
			move.w	d2,$b6(a0)			; move period to register

			move.w	#%0000000100000000,INTREQ(a0) ; clear pending interrupt
			move.w	#%1000000100000000,INTENA(a0) ; enable interrupt

			move.w	#$8202,$96(a0)	; turn it on
			move.l	(sp)+,d2

			rts

_stopsample	
			lea		_vblank_data,a1
			and.b	#$fe,vce_stat+voice4(a1)
			move.l	#$dff000,a0		; pointer to sysregs
			move.w	#0,$b8(a0)		; zero volume
			move.w	#2,$b6(a0)		; move period to register
			rts

_set_tempo	
			lea		_vblank_data,a1
			move.l	4(sp),d0
			move.w	d0,tempo(a1)
			rts

_setscore
			lea		_vblank_data,a1	; address of vblank data

			move.l	4(sp),d0
			move.l	d0,voice1+trak_beg(a1) ; set trak pointers
			move.l	8(sp),d0
			move.l	d0,voice2+trak_beg(a1) ; set trak pointers
			move.l	12(sp),d0
			move.l	d0,voice3+trak_beg(a1) ; set trak pointers
			move.l	16(sp),d0
			move.l	d0,voice4+trak_beg(a1) ; set trak pointers

			rts

_playscore
			movem.l	d0/a0/a1/a2,-(sp)
			move.l	#$dff000,a0		; pointer to sysregs
			lea		_vblank_data,a1	; address of vblank data

			move.l	4+16(sp),d0
			move.l	d0,voice1+trak_beg(a1) ; set trak pointers
			move.l	d0,voice1+trak_ptr(a1)
			move.l	8+16(sp),d0
			move.l	d0,voice2+trak_beg(a1) ; set trak pointers
			move.l	d0,voice2+trak_ptr(a1)
			move.l	12+16(sp),d0
			move.l	d0,voice3+trak_beg(a1) ; set trak pointers
			move.l	d0,voice3+trak_ptr(a1)
			move.l	16+16(sp),d0
			move.l	d0,voice4+trak_beg(a1) ; set trak pointers
			move.l	d0,voice4+trak_ptr(a1)

			clr.l	d0
			move.w	d0,$a8(a0)			; set volumes to zero
			move.w	d0,$b8(a0)
			move.w	d0,$c8(a0)
			move.w	d0,$d8(a0)

			move.l	ins_handle(a1),a2
			move.w	(a2)+,voice1+wave_num(a1)	; init voices to trak #
			move.w	(a2)+,voice2+wave_num(a1)
			move.w	(a2)+,voice3+wave_num(a1)
			move.w	(a2)+,voice4+wave_num(a1)

			move.w	#-1,d0
			move.b	d0,voice1+vol_delay(a1)	; disable envelopes
			move.b	d0,voice2+vol_delay(a1)
			move.b	d0,voice3+vol_delay(a1)
			move.b	d0,voice4+vol_delay(a1)
			clr.l	d0
			move.l	d0,timeclock(a1)		; reset timer
			move.l	d0,voice1+event_start(a1)	; reset event_start, event_stop
			move.l	d0,voice2+event_start(a1)
			move.l	d0,voice3+event_start(a1)
			move.l	d0,voice4+event_start(a1)
			move.l	d0,voice1+event_stop(a1)
			move.l	d0,voice2+event_stop(a1)
			move.l	d0,voice3+event_stop(a1)
			move.l	d0,voice4+event_stop(a1)
			move.b	d0,voice1+vce_stat(a1)
			move.b	d0,voice2+vce_stat(a1)
			move.b	d0,voice3+vce_stat(a1)
			move.b	d0,voice4+vce_stat(a1)
			move.b	#0,nosound(a1)	; reset nosound -- all systems go
			move.w	#$820f,$96(a0)	; ENERGIZE!!

			movem.l	(sp)+,d0/a0/a1/a2
			rts

_stopscore	move.l	#$dff000,a0		; pointer to sysregs
			lea		_vblank_data,a1	; address of vblank data
			moveq	#0,d0
			move.w	d0,$a8(a0)			; set volumes to zero
			move.w	d0,$b8(a0)
			move.w	d0,$c8(a0)
			move.w	d0,$d8(a0)
			move.w	d0,voice1+vol_delay(a1)	; clear volume delay, vce_stat
			move.w	d0,voice2+vol_delay(a1)
			move.w	d0,voice3+vol_delay(a1)
			move.w	d0,voice4+vol_delay(a1)

			move.w	#$000f,$96(a0)	; DMACON - kill it;
			move.b	#-1,nosound(a1)
			rts

_init_music
			move.l	a6,-(sp)			; save lib pointer
			move.l	#_vblank_data,a1		; address of vdata
			move.l	8(sp),ins_handle(a1)	; set handles
			move.l	12(sp),wav_handle(a1)
			move.l	16(sp),vol_handle(a1)
			move.w	#150,tempo(a1)		; set initial tempo
			move.b	#-1,nosound(a1)		; set no sound

			lea		server_node,a1
			move.l	#0,0(a1)
			move.l	#0,4(a1)
			move.w	#1,8(a1)
			move.l	#int_name,10(a1)
			move.l	#_vblank_data,14(a1)
			move.l	#_vblank_server,18(a1)

			move.l	4,a6				; abs exec base
			move.l	#5,d0
			jsr		AddIntServer(a6)		; define?

; install audio interrupt handler

			lea		aud_node,a1
			move.l	#0,0(a1)
			move.l	#0,4(a1)
			move.w	#1,8(a1)
			move.l	#aud_name,10(a1)
			move.l	#_vblank_data,14(a1)
			move.l	#audio_int,18(a1)

			move.l	4,a6				; abs exec base
			move.l	#8,d0				; channel 2 interrupt
			jsr		SetIntVector(a6)		; define?
			move.l	d0,old_handler3

			move.l	#$dff000,a0		; pointer to sysregs
			move.w	#%0000000100000000,INTREQ(a0) ; shut off bit 7 reqs
			move.w	#%0000000100000000,INTENA(a0) ; set bit 7

			move.l	(sp)+,a6
			rts

_wrap_music
			jsr		_stopscore
			move.l	a6,-(sp)			; save lib pointer
			move.l	4,a6				; abs exec base
			lea		server_node,a1
			move.l	#5,d0
			jsr		RemIntServer(a6)		; define?

			move.w	#%0000000100000000,INTENA(a0) ; reset bit 7
			move.l	4,a6				; abs exec base
			move.l	old_handler3,a1
			move.l	#8,d0
			jsr		SetIntVector(a6)		; define?

			move.l	(sp)+,a6
			rts

			dseg

int_name	dc.b	"music vblank"
			dc.b	0
			dc.l	0
aud_name	dc.b	"audio 4"
			dc.b	0
			dc.l	0,0,0

_vblank_data
			ds.l	2
			ds.b	vbase+(4*28)+16

server_node
sv1			ds.l	1
sv2			ds.l	1
sv3			ds.b	1
sv4			ds.b	1
sv5			ds.l	1	; name

sv6			ds.l	1	; data
sv7			ds.l	1	; code

aud_node	ds.l	6			; midi write node

old_handler1 ds.l	1
old_handler2 ds.l	1
old_handler3 ds.l	1

			end
