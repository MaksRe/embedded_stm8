;	MOVEABLE CODE INITIALIZATION
;	Copyright (c) 2006 by COSMIC Software
;
	xdef	__fctcpy
	xref	__idesc__
	xref.b	c_x
	.dcall	"3,0,__fctcpy"
;
__fctcpy:
	and	a,#$1F			; convert byte code
	ld	c_x+2,a			; keep it
	ldw	x,#__idesc__		; start address
ibcl:
	ld	a,(2,x)			; test flag byte
	jreq	cend			; no more segment
	bcp	a,#$60			; test for moveable code segment
	jrne	next			; no, continue
	cp	a,c_x+2			; compare with argument
	jreq	cseg			; yes, copy it
next:
	addw	x,#5			; next descriptor
	jra	ibcl			; and loop
cseg:
	ldw	y,x			; move end
	ldw	y,(5,y)			; address in
	ldw	c_x,y			; memory
	ldw	y,x
	ldw	y,(3,y)			; ram address
	ldw	x,(x)			; start address
dbcl:
	ld	a,(x)			; transfer
	ld	(y),a			; byte
	incw	y			; next
	incw	x			; byte
	cpw	x,c_x			; source
	jrne	dbcl			; address
	ld	a,#1			; non zero result
cend:
	clrw	x			; complete
	ld	xl,a			; result
	ret				; and return
;
	end
