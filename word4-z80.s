;
;	Game specific code
;
;
;	Pick right routine for word size. This one does 4 chars
;	Check wordbuf matches our word. Both are zero padded for simplicity
;
wordeq:
	ld bc, (wordbuf)
	ld a (hl)
	and #0x7F
	cp c
	ret nz
	inc hl
	ld a, (hl)
	cp b
	ret nz
	inc hl
	ld bc, (wordbuf+2)
	ld a, (hl)
	cp c
	ret nz
	inc hl
	ld a, (hl)
	cp b
	ret

;
;	Copy a word into the wordbuf (max 4 letter matches)
;
copy_word:
	call skip_spaces
	push hl
	ld (nounbuf), hl	; we always scan noun second so this is ok
	ld c, (hl)
	inc hl
	ld b, (hl)
	ld (wordbuf), bc
	inc hl
	ld c, (hl)
	inc hl
	ld b, (hl)
	ld (wordbuf+2), bc
	call word_clear		; clear end of word buf to zero for matching
	pop hl
copymove:
	ld a, (hl)
	or a			; End (watch space = 0 on ZX81 ?? process
	ret z			; ASCII internally!)
	cp 32			; Space
	ret z
	inc hl
	jr copymove

;
;	Clear tail of 4 char word
;
word_clear:
	ld hl, wordbuf
	ld b, 4
wordclr4l:
	ld a (hl)
	or a
	jr z, padrest
	cp 32
	jr z, padrest
	inc hl
	djnz wordclr4l
	ret			; >= 4 letters
padrest:
	ld a, 32
padrestl:
	ld (hl), a
	inc hl
	djnz padrestl
	ret

copy_abbrev:
	ld de, linebuf
	ldi
	ldi
	ldi
	ldi
	xor a
	ld (de), a
	ret
