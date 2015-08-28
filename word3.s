;
;	Game specific code
;
;
;	Pick right routine for word size. This one does 4 chars
;	Check wordbuf matches our word. Both are zero padded for simplicity
;
wordeq:
	pshx
	ldd ,x
	anda #$7f
	cmpa wordbuf
	bne notmatch
	cmpb wordbuf+1
	bne notmatch
	ldaa 2,x
	cmpa wordbuf+2
notmatch:
	pulx
	rts

;
;	Copy a word into the wordbuf (max 4 letter matches)
;
copy_word:
	jsr skip_spaces
	stx nounbuf		; we always scan noun second so this is ok
	ldd ,x
	std wordbuf
	ldaa 2,x
	staa wordbuf+2
	pshx
	bsr word_clear		; clear end of word buf to zero for matching
	pulx
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
	cpx #wordbuf+3
	bne wordclr4l
	; >=3 letters
	rts
padrest:
	ldaa #32
padrestl:
	staa ,x
	inx
	cpx #wordbuf+3
	bne padrestl
	rts
