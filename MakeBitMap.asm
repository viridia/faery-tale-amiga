* Allocate space for bit map rasters *

		include 'exec/types.i'
		include 'graphics/gfx.i'
		include 'macros.i'

		xref		_GfxBase

* allocate the rasters for this bit map *

		DECLARE 	MakeBitMap			; MakeBitMap(bm,depth,width,height)

		SaveM		a2-a3/a6/d2/d4-d7			; 24 bytes

		move.l		4+32(sp),d0			; if bm = NULL, error
		beq.s		bye

		move.l		d0,a2				; save bm pointer
		move.l		8+32(sp),d4			; get depth

		move.l		a2,a0				; bm pointer
		move.l		d4,d0				; depth
		move.l		12+32(sp),d6		; width
		move.l		d6,d1
		move.l		16+32(sp),d7		; height
		move.l		d7,d2

		CallGfx		InitBitMap			; init that bitmap

		move.l		a2,a0				; bm pointer
		addq.w		#bm_Planes,a0		; bm_Planes = 8
		move.l		d4,d1				; depth
		bra.s		1$
2$
		clr.l		(a0)+
1$
		dbra		d1,2$

		move.l		a2,a3				; set-up to allocate planes
		addq.w		#bm_Planes,a3
		bra.s		3$
4$
		move.l		d6,d0				; allocate a plane
		move.l		d7,d1
		CallGfx		AllocRaster
		tst.l		d0					; if zero, error
		beq.s		error
		move.l		d0,(a3)+			; else, copy into bitmap
3$
		dbra		d4,4$				; decrement depth count

		moveq		#1,d0
		bra.s		bye

error
		move.l		a2,-(sp)
		jsr			_UnMakeBitMap
		addq.w		#4,sp
		moveq		#0,d0
bye
		RestoreM	a2-a3/a6/d2/d4-d7
		rts

		DECLARE		UnMakeBitMap			; UnMakeBitMap(bitmap)

		SaveM		a2/a6/d4-d6				; 16 bytes

		move.l		4+20(sp),a2				; bitmap
		moveq		#0,d4
		move.w		bm_BytesPerRow(a2),d4	; bytes per row
		lsl.l		#3,d4					; pixels per row
		moveq		#0,d5		
		move.w		bm_Rows(a2),d5			; rows
		moveq		#0,d6
		move.b		bm_Depth(a2),d6			; depth

		clr.b		bm_Depth(a2)			; set depth = 0
		addq.w		#bm_Planes,a2			; adjust pointer to planes start
		bra.s		1$
2$
		move.l		(a2),d0
		beq.s		1$
		move.l		d0,a0
		move.l		d4,d0
		move.l		d5,d1
		CallGfx		FreeRaster
		clr.l		(a2)+
1$
		dbra		d6,2$

		RestoreM	a2/a6/d4-d6
		rts
		
		end

MakeBitMap(b,depth,width,height)
	register struct BitMap *b; LONG width, height, depth;
{	register short i, success=TRUE; PLANEPTR AllocRaster();

	if (!b) return FALSE;
	for (i=0; i<depth; i++) b->Planes[i] = NULL;	/* set all planes = nonexistant */
	InitBitMap(b,depth,width,height);				/* initialize map */

	for (i=0; i<depth && success; i++)				/* allocate all planes */
	{	if (!(b->Planes[i] = AllocRaster(width,height)) ) success = FALSE;  }
	if (!success) UnMakeBitMap(b);					/* if failure, de-allocate */

	return success;									/* return TRUE or FALSE */
}

/* de-allocate the rasters for this bit map */

UnMakeBitMap(b) register struct BitMap *b;
{	register short i;

	for (i=0; i<b->Depth; i++)						/* for each plane */
	{	if (b->Planes[i])							/* if that plane exists */
		{	FreeRaster(b->Planes[i],b->BytesPerRow,b->Rows);	/* free it */
			b->Planes[i] = NULL;					/* mark as free */
		}
		b->Depth = 0;								/* mark bitmap as empty */
	}
}
