;
;	Game specific code
;
;
;	Pick right routine for word size. This one does 4 chars
;	Check wordbuf matches our word. Both are zero padded for simplicity
;
wordeq:
	clrb			; 0 - match
	stx wordeq_x+1		; save the old x by patching the return
	ldaa ,x			; ldx. Handy 6800ism
	anda #$7f
	cmpa wordbuf
	bne notmatch
	ldaa 1,x
	cmpa wordbuf+1
	bne notmatch
	ldaa 2,x
	cmpa wordbuf+2
	bne notmatch
	ldaa 3,x
	cmpa wordbuf+3
	beq wordeq_x
notmatch:
	incb			; non zero - fail
wordeq_x:
	ldx #0
	tstb			; set flags (ldx trashes them)
	rts
	
;
;	Copy a word into the wordbuf (max 4 letter matches)
;
copy_word:
	jsr skip_spaces
	stx nounbuf		; we always scan noun second so this is ok
	ldaa ,x
	staa wordbuf
	ldaa 1,x
	staa wordbuf+1
	ldaa 2,x
	staa wordbuf+2
	ldaa 3,x
	staa wordbuf+3
	stx copyword_x+1
	bsr word_clear		; clear end of word buf to zero for matching
copyword_x:
	ldx #0
	ldaa #32
copymove:
	cmpa ,x			; Space
	beq nextsp		; Word over
	tst ,x			; End
	beq nextsp		; Word over
	inx			; Move on a byte
	bra copymove		; Keep looking
nextsp:
	rts

;
;	Clear tail of 4 char word
;
word_clear:
	ldx #wordbuf
wordclr4l:
	ldaa ,x
	beq padrest
	cmpa #32
	beq padrest
	inx
	cpx #wordbuf+4
	bne wordclr4l
	; >= 4 letters
	rts
padrest:
	ldaa #32
padrestl:
	staa ,x
	inx
	cpx #wordbuf+4
	bne padrestl
	rts

copy_abbrev:
	ldaa ,x
	staa linebuf
	ldaa 1,x
	staa linebuf+1
	ldaa 2,x
	staa linebuf+2
	ldaa 3,x
	staa linebuf+3
	clr linebuf+4
	rts
