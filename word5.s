;
;	Game specific code
;
;
;	Pick right routine for word size. This one does 5 chars
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
	ldd 2,x
	cmpa wordbuf+2
	bne notmatch
	cmpb wordbuf+3
	ldaa 4,x
	cmpa wordbuf+4
	bne notmatch
notmatch:
	pulx
	rts

;
;	Copy a word into the wordbuf (max 5 letter matches)
;
copy_word:
	jsr skip_spaces
	stx nounbuf		; we always scan noun second so this is ok
	ldd ,x
	std wordbuf
	ldd 2,x
	std wordbuf+2
	ldaa 4,x
	staa wordbuf+4
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
wordclr5l:
	ldaa ,x
	beq padrest
	cmpa #32
	beq padrest
	inx
	cpx #wordbuf+5
	bne wordclr5l
	; >= 5 letters
	rts
padrest:
	ldaa #32
padrestl:
	staa ,x
	inx
	cpx #wordbuf+5
	bne padrestl
	rts

copy_abbrev:
	ldd ,x
	std linebuf
	ldd 2,x
	std linebuf+2
	ldaa 4,x
	staa linebuf+4
	rts
