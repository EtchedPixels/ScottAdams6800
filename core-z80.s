;
;	Major items to do
;	Replace 0 terminator with 255 (0 is space on ZX81), also use
;	different symbol for newline! (10 is a letter)
;
;	Save and Load
;	Vast amounts of deubugging
;	Encrypt text / compression if needed (upper only so should work
;	well with a simple ETAIONSHRDU style compressor or a 5bit code)
;	Replace zero termination with top bit set or length.8 and list run
;	to save memory if needed (length.7 + '*' bit may even work ?)
;	Condition <= >= 16bit check
;	Ramsave/load
;	MCX graphics ??? 8)
;
	org 17500

LIGHTOUT	equ	16
DARKFLAG	equ	15
LIGHT_SOURCE	equ	9

NUM_BITS	equ	32

VERB_GO		equ	1
VERB_GET	equ	10
VERB_DROP	equ	18

start:
	jp start_game

;
;	Wipe the screen
;
wipe_screen:
	ld c,24			; 32  x 24 display plus 1 char of end marker
wipe_screen_c:
	ld hl, (DISPLAY)
	xor a			; Space
wipeblock:
	ld b,32
wipeline:
	ld (hl),a
	inc hl
	djnz wipeline
	inc hl			; skip the end marker
	dec c
	jr nz, wipeblock
	ret

wipe_lower:
	ld hl, (lowtop)
	ld a, (lowlines)
	ld c,a
	jr wipeblock
;
;	Scroll the lower window
;
scroll_lower:
	push hl
	push de
	push bc
	ld hl, (lowtop)
	push hl
	ld de, 33
	add hl,de
	ex de,hl
	pop hl
	
	ld a, (lowlines)
lowscrl:
	ld bc,32
	ldir
	inc hl
	inc de
	dec a
	jr nz, lowscrl
lowclr:	; A will be 0 at this point
	ld b,32
	ld (nexttop), hl
lowclrl:
	ld (hl), a
	djnz lowclrl
	pop bc
	pop de
	pop hl
	ret
;
;	Start using the upper area. Clear the old upper zone
;
;	Assumption: upper is never empty
;
start_upper:
	call word_flush_l
	ld a, (highlines)
	ld c,a
	jr wipe_screen_c		; wipe the top area
	ld hl, (DISPLAY)
	ld (nextupper),hl
	xor a
	ld (highx), a
	ld (highlines),a
	ret
;
;	Finished using the upper area. Draw the divider and set up for
;	the low section
;
end_upper:
	call word_flush_u
	ld a, (highx)
	or a
	jr z, end_upper_0
	ld hl, (nextupper)
end_upper_w:				; Wipe the rest of the final line
	ld (hl), 0
	dec a
	jr nz, end_upper_w
	inc hl				; Past end marker
end_upper_0:
	ld a, LESSTHAN
	ld b, 32
upper_bar:
	ld (hl), a
	xor 2				; < <-> > 
	inc hl
	djnz upper_bar
	inc hl
	ld (lowtop), hl
	ret
;
;	Print to the upper screen area. We never handle characters wrapping
;	as the higher level justifier is responsible for that
;
chout_upper:
	or a			; Space
	jr z,is_space_u
	cp NEWLINE
	jr z, upper_nl
	;
	;	Normal text into the justify buffer
	;
chout_queue:
	ld hl, (justify)
	ld (hl), a
	inc hl
	ld (justify), hl
	ld a, (justlen)
	inc a
	ld (justlen), a
nonl_u:
	ret
	;
	;	Newline
	;
upper_nl:
	call word_flush_u
upper_nl_raw:
	ld hl, (nexttop)
	ld a, (highx)
	or a
	jr z, upper_nl_n
	ld b, a
	ld a, 32
	sub b			; See what we need to space fill
	ld b, a
upper_spc:			; Clear the rest of the line
	ld (hl), SPACE
	inc hl
	djnz upper_spc
	xor a
	ld (highx), a
	jr upper_st_out
	;
	;	Space is much like newline and we should probably
	;	merge them
	;
is_space_u:
	call word_flush_u	; Flush the word if any
	ld a, (highx)
	or a
	ret z			; No trailing spaces printed
	ld a, SPACE
	;
	;	Print an upper screen character directly
	;
chout_upper_raw:
	ld hl,(nextupper)
	ld (hl), a
	inc hl
	ld a, (highx)
	inc a
	ld (highx), a
upper_st_out:
	ld (nextupper), hl
	ret

word_flush_u:
	ld hl, (justify)
	ld de, justbuf
	or a
	sbc hl, de		; L now holds the size needed

	ld a, (highx)
	add l
	cp 32
	call nc, upper_nl_raw
no_wrap_u:
	ld hl, (justbuf)
	ld a, (justlen)
	ld b, a
wordfl_u_lp:
	ld a, (hl)
	push hl
	call chout_upper_raw
	pop hl
	inc hl
	djnz wordfl_u_lp
wordflush_u_done:
	ld hl, justbuf
	ld (justify), hl
	xor a
	ld (justlen), a
noscroll_l:
	ret
;
;	Print to the lower screen area
;
chout_lower:
	cp SPACE
	jr z, is_space_l
	cp NEWLINE
	jr z, lower_nl
	jr chout_queue
	;
	;	Newline
	;
lower_nl:
	call word_flush_l
	ld a, (lowx)
	or a
	ret z
lower_nl_raw:
	jp scroll_lower
	;
	;	Space is much like newline and we should probably
	;	merge them
	;
is_space_l:
	call word_flush_l	; Flush the word if any
	ld a,(lowx)
	or a
	jr z, no_output_l	; No trailing spaces printed
	ld a, SPACE
	;
	;	Print a lower screen character directly
	;
chout_lower_raw:
	push de
	ld e, a
	ld a, (lowx)
	cp 32
	call z, scroll_lower
not_scroll:
	ld hl, lowx
	inc (hl)
	ld hl, (nextchar)
	ld (hl), e
lower_st_out:
	ld (nextchar), hl
	pop de
	ret

word_flush_l:
	push bc
	ld a, (justlen)
	ld b, a
	ld a, (lowx)
	add b
	cp  32		; wrapping ?
	call nc, lower_nl_raw
no_wrap_l:
	ld hl, justbuf
	ld a, (justlen)
	ld b, a
wordfl_l_lp:
	ld a,(hl)
	push hl
	call chout_lower_raw
	pop hl
	inc hl
	djnz wordfl_l_lp
wordflush_l_done:
	ld hl, justbuf
	ld (justify), hl
	pop bc
	ret
;
;	Print a string of text to the lower window
;
strout_lower:
	push de
	push bc
strout_lower_l:
	ld a, (hl)
	cp 255
	jr z, strout_done
	push hl
	call chout_lower
	pop hl
	inc hl
	jr strout_lower_l
strout_done:
	pop bc
	pop de
	ret

;
;	Print a sring of text to the lower window followed by a space
;
strout_lower_spc:
	call strout_lower
	push hl
	xor a
	call chout_lower
	pop hl
	ret
;
;	Print a string of text to the upper window
;
strout_upper:
	push bc
	push de
strout_upper_l:
	ld a, (hl)
	cp 255
	jr z,strout_done
	push hl
	call chout_upper
	pop hl
	inc hl
	jr strout_upper_l

hexoutpair_lower:
	push hl
	ld a, h
	call hexout_lower
	pop hl
	ld a, l
;
;	Hexadecimal output to lower window (debug only)
;
hexout_lower:
	push af
	rrca
	rrca
	rrca
	rrca
	call hexdigit
	pop af
hexdigit:
	and 0x0F
	cmp 10
	jr c,hexout_digit
	add HEX_SHIFT			; 7 for ASCII
hexout_digit:
	add ZERO_CHAR			; 48 for ascii
	jp chout_lower

;
;	Decimal output to lower window (0-99 is sufficient)
;	
decout_lower:
	ld b, 0
decout_div:
	sub 10
	jr c, decout_mod
	inc b
	jr decout_div
decout_mod:
	add 10			; correct overrun
	push bc
	bsr hexdigit
	pop af
	jr hexdigit

;
;	Keyboard
;
read_key:
	ldx $FFDC
	call ,x
	beq read_key
	cp NEWLINE
	jr z, key_cr		; we don't want to stir in the newline
				; as it and the random query will be a fixed
				; number of clocks apart in many cases
	jp randseed		; add to the randomness

;
;	Wait for newline
;
wait_cr:
	call read_key
	cp NEWLINE
	jr nz, wait_cr
key_cr:
	ret

;
;	Yes or No. Returns A = 0 for Yes, 0xFF for no
;
yes_or_no:
	call word_flush_l
	call read_key
	cp KEY_Y
	jr z, is_yes
	cp KEY_J
	jr z, is_yes	; German Gremlins!
	cp KEY_N
	jr nz, yes_or_no
is_no:	ld a, 255
	ret
is_yes:	xor a
	ret

;
;	Line editor
;
line_input:
	ld hl, (nextchar)
	dec hl
	ld (nextchar), hl
	ld (hl), CURSOR
	ld hl, linebuf
	ld (lineptr), hl
	ld hl, lowx
	dec (hl)
line_loop:
	call read_key
	ld hl, (lineptr)
	cp DELETE
	beq delete_key
	cp NEWLINE
	beq enter_key
	cp SPACE
	jr c,line_loop
	ld c, a
	ld a, (leftx)
	cp 29		; 32 minus "> " and cursor
	jr z, line_loop		; didn't fit
	ld (hl), a
	inc hl
	ld (lineptr), hl
	ld hl, (nextchar)
	ld (hl), a
	inc hl
	ld (nextchar), hl
	ld (hl), CURSOR
	ld hl, leftx
	inc (hl)
	jr line_loop

delete_key:
	ld a, (leftx)
	or a
	jr z, line_loop
	dec a
	ld (leftx), a
	ld hl, (lineptr)
	dec hl
	ld (lineptr), hl
	ld hl, (nextchar)
	ld (hl), SPACE
	dec hl
	ld (hl), CURSOR
	ld (nextchar), hl
	ld hl, leftx
	dec (hl)
	jr line_loop
enter_key:
	ld (hl), 255		; Mark the end of the buffer
	ld hl, (nextchar)	; Clean up the cursor
	ld (hl), SPACE
	ld a, NEWLINE
	jp chout_lower		; Move on a line and return


;
;	Machine specific
;
random:
	ld b, a
	add a		;	a = 2 x chance
	srl b		;	b = 0.5 x chance
	add b		;	gives us 2.5 x the 1 in 100 chance
			;	versus 0-255
	ld b, a
	ld a, (randpool);	pool scrambling
	ld c, a
	ld a, r		;	stir in timer
	xor c
	rrca
	adc 0		;	rollover
	ld (randpool), a;	stirred
	cp b
	ret		;	C or NC
randpool:
	.db	0

randseed:
	push af
	push bc
	ld a, r
	ld b, a
	ld a, (randpool)
	rlca
	adc b
	ld (randpool), a
	pop bc
	pop af
	ret

;
;	Parser
;


;
;	Find a word in the verb table
;
whichverb:
	ld hl,verbs
	jr whichword
;
;	Find a word in the noun table
;
whichnoun:
	ld hl,nouns
;
;	Find a word. The approach used is weird but we copy it to keep the
;	numbering. Aliases are the same as the non alias word, but all words
;	are numbered. So if word 2 is an alias then word 2 returns 1 but
;	word 3 returns 3
;
whichword:
	ld de,0
	ld a,(wordbuf)
	cp SPACE
	jr z,foundword		; 0 - none
	cp 255
	jr z,foundword
	dec d			; -1	
whichwordl:
	inc d			; 0
	bit 7,(hl)
	jr nz,alias
	ld e,d			; take a copy of the code
alias:
	call wordeq
	jr z,foundword
	ld bc,WORDSIZE
	add hl,bc
	xor a
	cp (hl)
	jr nz,whichwordl
	dec a			; 255
	ret
foundword:
	ld a,e
	ret

;
;	HL points to the input. Skip any spaces
;
skip_spaces:
	ld a,32
skipl:
	cmp (hl)
	ret nz
	inc hl
	jr skipl

;
;	Scan the input for a verb / noun pair. German language games are a
;	bit different so this won't work for them
;
scan_input:
	ld hl,linebuf
	call copy_word		; Verb hopefully
	push hl
	call whichverb
	ld (verb),a
	pop hl
scan_noun:			; Extra entry point used for direction hacks
	call copy_word
	call whichnoun
	ld (noun),a
	ret

;
;	See if we have a relevant one word abbreviation
;
abbrevs:
	ld hl, linebuf
	cakk skip_spaces
	inc hl
	ld a,(hl)
	or a
	jr z,abbrev_ok
	cp SPACE
	ret nz
abbrev_ok:
	dec hl
	ld a,(hl)
	ld hl,a_nort
	cp KEY_N
	jr z,do_abbr
	ld hl,a_sout
	cp KEY_S
	jr z,do_abbr
	ld hl,a_east
	cp KEY_E
	jr z,do_abbr
	ld hl,a_west
	cp KEY_W
	jr z,do_abbr
	ld hl,a_up
	cp KEY_U
	jr z,do_abbr
	ld hl,a_down
	cp KEY_D
	jr z,do_abbr
	ld hl,a_inve
	cp KEY_I
	ret nz
do_abbr:
	jp copy_abbrev

;
;	Main game logic runs from here
;
main_loop:
	call look
	jr do_command_1

do_command:
;
;	Implement the builtin logic for the lightsource in these games
;
	ld a,(objloc+LIGHT_SOURCE)
	ld b,a
	or a
	jr z,do_command_1
	ld a,lighttime
	cp 255			; does  not expire
	jr z,do_command_1
	dec a
	jr nz,light_ok
	xor a
	ld (bitflags_LIGHTOUT),a ; light goes out
	ld a,255
	cp b
	jr z, seelight
	ld a,(location)
	jr nz, unseenl
seelight:
	ld hl,lightout
	call strout_lower
unseenl:
; Earliest engine only
;	clr objloc+LIGHT_SOURCE
	ld a,1
	ld (redraw),a
	jr do_command_1
light_ok:
	cp 25				; warnings start here
	jr nc,do_command_1		; FIXME: some games do a general
	ld hl,lightoutin		; warning every five instead
	push af
	call strout_lower
	pop af
	push af
	call decout_lower
	ld hl,turns
	pop af
	cp 1
	jr nz,turns_it_is
	ld hl,turn
turns_it_is:
	call strout_lower

; Fall through	
;
;	We start by running the status table. All lines in this table are
;	automatic actions or random %ages. We could possibly squash it a bit
;	more by not using the same format, but it's not clear the code
;	increase versus data decrease is a win
;
do_command_1:
	xor a
	ld (verb),a		; indicate status
	ld (noun), a
	ld hl,status
	call run_table		; run through the table
	ld a,(redraw)
	or a
	jr z,no_redraw		; anything in or out of view, or moved us
	call look
	xor a
	ld (redraw),a
no_redraw:
do_command_2:
	ld hl,whattodo		; prompt the user
	call strout_lower
do_command_l:
	ld hl,prompt
	call strout_lower
	call wordflush		; avoid buffering problems
	call line_input		; read a command
	call abbrevs		; do any abbreviation fixups
	ld hl,linebuf
	call skip_spaces
	xor a
	cp (hl)
	jr z, do_command_l	; round we go
	push hl
	call scan_noun		; take the first input word and see
	ld a,(noun)		; if its a direction
	jr z notdirn
	cp 6
	jr nc,notdirn
	ld a,VERB_GO
	ld (verb),a		; convert this into "go foo"
	pop hl
	jr parsed_ok
;
;	Try a normal verb / noun parse
;
notdirn:
	pop hl
	call scan_input
;	push hl
;	ld a,'V'
;	call chout_lower
;	ld a,(verb)
;	call hexout_lower
;	ld a,'N'
;	call chout_lower
;	ld a,(noun)
;	call hexout_lower
;	pop hl
	ld a,(verb)
	or a
	jr z,do_command_l	; no verb given ?
	inc a
	jr nz,parsed_ok		; 255
	ld hl,dontknow		; no verb, error and back to the user
	call strout_lower
	jp do_command_2


parsed_ok:
;
;	We have a verb noun pair
;
;	Hardcoded stuff first (yes this is a bad idea but it's how the
;	engine works)
;
	ld a,(verb)
	cp VERB_GO		; GO [direction]
	jr nz, not_goto
	ld a,(noun)
	or a
	jr nz,not_goto_null
	ld hl,givedirn		; Error "go" on it's own
	call strout_lower
	jr do_command_2
not_goto_null:
	cp 6			; 1 - 6 are compass
	jr nc,not_goto		; not a compass direction
	ld c,a			; save noun
	ld e,0
	call islight		; check if it is dark (uses B)
	jr z,not_dark_goto
	inc e			; temporary dark flag
	ld hl,darkdanger	; warn the user
	call strout_lower
not_dark_goto:
	push de
	call getloc_hl
	pop de
	ld b,0
	add hl,bc
	inc hl
	inc hl
	ld a,(hl)		; valid ?
	or a
	jr nz,can_do_goto
	bit 0,e
	jr z,movefail		; light so ok
	ld hl,brokeneck		; tell the user
	call strout_lower
	call act88		; do a delay
	call act61		; and die
	jp do_command
movefail:
	ld hl,cantgo		; tell the user they can't move
	call strout_lower
	jp do_command_2
can_do_goto			; a goto that works
	ld (location),a
	ld a,1
	ld (redraw),a		; will need to redraw
	jp do_command
;
;	Then the tables (the goto first is a design flaw in the Scott Adams
;	system)
;
not_goto:
	xor a
	ld (linematch),a	; match state for error reporing.
	ld (actmatch),a
	ld hl,actions		; run the action table
	call run_table
	ld a,(actmatch)
	or a			; we did actions, so all was good
	jr nz,do_command
	ld hl,dontunderstand
	ld a,(linematch)
	or a			; got as far as conditions
	jr z, notnotyet		; a match exists but conds failed ?
	ld hl,notyet		; give user a clue if so
notnotyet:
	call strout_lower	; display error
	jp do_command

;
;	Useful Helpers
;

;
;	Check if we are in the light (Z) or not (NZ)
;
islight:
	ld a,(bitflags+DARKFLAG)
	or a				; if it isn't dark then it's light
	ret z
	ld a,(objloc+LIGHT_SOURCE)	; get the lamp
	cp 255				; carried ?
	ret z
	ld b,a
	ld a, (location)		; in the room ?
	cp b
	ret

;
;	Given HL is the objloc pointer of an object return its text string.
;	This looks an odd interface but all our callers happen to have this
;	HLL value directly to hand.
;
getotext_hl:
	push de
	ld de,objloc
	or a
	sbc hl,de
	ld de, objtext	; Two byte pointer per object
	add hl,hl
	add hl,de
	ld l,(hl)
	inc hl
	ld h,(hl)
	pop de
	ret

set_argp:
	ld hl,args
	ld (argp),hl
	ret
;
;	Game Conditions
;
perform_line:
;	call debug
	call set_argp
	; HL points at args
	xor a
	ld b,10
perfw
	ld (hl),a
	inc hl
	djnz perfw
	ld a,(condacts)
	rrca
	rrca
	and #0x07
	jr z, noconds
	ld b,a
condl:
;	call debug2
	push hl
	push bc
	call cond		; run the condition
	pop bc
	pop hl
	or a			; did it fail
	ret nz			; if so we the line is done
	inc hl			; move on a condition
	inc hl
	djnz condl		; done yet
;
;	Now we can process the actions
;
noconds:
	ld a,1
	ld (actmatch),a
	call set_argp		; reset the arg pointer
	ld a,(condacts)		; see how many actions
	and 3
	inc a			; 1-4 not 0-3
	ld b,a
nextact:
;	call debug3
	ld a, (hl)		; get the action code
	inc hl			; move on
	push hl
	push bc
	call act		; run the action
	pop bc
	pop hl
	djnz nextact		; until done
	ret

;
;	?? Does this need to sign extend ??
;
arghigh:
	ld a,(argh)
	rlca			; FIXME check
	rlca
	rlca
	and 7			; High bits
	ret

;
;	Run conditions.
;	It's probably a mistake and we should use a table
;
cond:
	ld a,(hl)		; A = cond, E = value
	inc hl
	ld e,(hl)
	ld (argh),a		; High arg bits saved
	and 0x1f		; Low bits only
	jr nz,cond1
;
;	Condition 0 is "parameter", and always true. It saves a parameter
;	for the actions to use
;
	ld hl,argp
	ld (hl),e
	inc hl
	call arghigh
	ld (hl),a
	inc hl
	ld (argp),hl
;
;	True condition
;
condnp:
	xor a
	ret
cond1:
;
;	Many conditions want the object referenced by the argument. We
;	set hl up as a pointer to the argument
;
	ld hl,objloc
	ld d,0
	add hl,de
	dec a
;
;	Condition 1: true if the objct is carried
;
	jr nz,cond2
	ld a, (hl)
	cp 255
;
;	General purpose "true if eq"
;
cbrat:
	jr z, condnp
condfp:
	ld a,255
	ret
cond2:
	dec a
	jr nz,cond3
;
;	Condition 2: true if the object is in the same room
;
cond2c:
	ld a,(location)
	cp (hl)
	jr cbrat
cond3:
	dec a
	jr nz,cond4
;
;	Condition 3: true if the object is in the same room or carried
;
	ld a,(hl)
	cp 255
	jr z,condnp
	jr cond2c
cond4:
	dec a
	jr nz,cond5
;
;	Condition 4: true if the player is in a given place
;
	ld a,(location)
	cp e
	jr cbrat
cond5:
	dec a
	jr nz,cond6
;
;	Condition 5: true if the object is not in the same room
;
	ld a,(location)
	cp (hl)
;
;	General purpose true if not eq
;
cbraf:
	jr nz, condnp
	jr condfp
cond6:
	dec a
	jr nz, cond7
;
;	Condition 6: true if the object is not carried
;
	ld a,255
	cp (hl)
	jr cbraf
cond7:
	dec a
	jr nz,cond8
;
;	Condition 7: true if the player is not in a given place
;
	ld a,(location)
	cp  (hl)
	jr cbraf
cond8:
	dec a
	jr nz,cond9
;
;	Condition 8: True if bitflag n is set
;
	ld hl,bitflags
	add hl,de
	ld a,(hl)
	or a
	jr cbraf
cond9:
	dec a
	jr nz, cond10
;
;	Condition 9: True if bitflag n is clear
;
	ld hl,bitflags
	add hl,de
	ld a,(hl)
	or a
	jr cbrat
cond10:
	dec a
	jr nz,cond11
;
;	Condition 10: Carrying any objects
;
	ld a,(carried)
	or a
	jr cbraf
cond11:
	dec a
	jr nz,cond12
;
;	Condition 11: Not carrying any objects
;
	ld a,(carried)
	or a
	jr cbrat
cond12:
	dec a
	jr nz,cond13
;
;	Condition 12: Object not carried or in location
;
	ld a,255
	cp (hl)
	jr z,condfp
	ld a,(location)
	cp hl
	jr cbrat
cond13:
	dec a
	jr nz,cond14
;
;	Condition 13: Object not destroyed (room 0)
;
	ld a,(hl)
	or a
	jr cbraf
cond14:
	dec a
	jr nz,cond15
;
;	Condition 14: Object is destroyed (room 0)
;
	ld a,(hl)
	or a
	jr cbrat_f
cond15:
	dec a
	jr nz, cond16
;
;	Condition 15: Current counter is <= arg
;
	call arghigh
	ld d,a
	ld hl,(counter)
	or a
	sbc hl,de
	jr c, condnp
	jp condfp
cond16:
	dec a
	jr nz,cond17
;
;	Condition 16: Current counter is >= arg
;
	call arghigh
	ld d,a
	ld hl,(counter)
	or a
	sbc hl,de
	jr nc,condnp
	jr condfp
cond17:
	dec a
	jr nz,cond18
;
;	Condition 17: Object is in its original location
;
	ld a,(hl)		; location
	ld d,0
	ld hl,objinit		; initializer
	add hl,de
	cp (hl)
	jr cbrat
cond18:
	dec a
	jr nz,cond19
;
;	Condition 18: Object is not in its original location
;
	ld a,(hl)
	ld d,0
	ld hl,objinit
	add hl,de
	cp (hl)
	jr cbraf
cond19:
	dec a
	jr nz,condbad
;
;	Condition 19: Counter is equal to value.
;
	call arghigh
	ld d,a
	ld hl,(counter)
	or a
	sbc hl,de
	jp cbrat
;
;	If we get this far it is not a valid condition
;
condbad:
	ld hl,invcond
	call strout_lower
halted:
	di
	hlt
	jr halted

;
;	Game Actions
;

;
;	Print a message given by A-51
;
msg2:	
	sub 50
;
;	Print message given by A
;
msg:
	ld e,a
	ld d,0
	ld hl,msgptr
	add hl,de
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	jp strout_lower_spc

;
;	Action 0 (shouldn't appear)
;
noop:
	ret

;
;	Process the actions. We handle these via a jump table
;
act:
	or a
	jr z,noop
;
;	Codes < 52 are messages, codes >= 102 are the second lot of
;	messages. Historical bad planning ?
;
	cp 52
	jr c,msg
	cp 102
	jr nc,msg2
;
;	Real action
;
	ld e,a
	ld d,0
	ld hl,actab-104  ; First action is 52, 2 bytes each
	add hl,de
	add hl,de
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	ret

;
;	Collect a parameter argument (low 8bits) and put it into
;	E and A. Return with E = arg, HL = objloc ptr to object A and
;	B = location of object E. For convenience return D = 0
;
get_arg:
	ld hl,(argp)
	ld a,(hl)		; Low byte
	ld e,a
	inc hl
	inc hl
	ld (argp),hl
	ld hl,objloc
	ld d,0
	add hl,de
	ld b,(hl)
	ret

;
;	Retrieve a 16bit parameter in DE
;
get_arg16:
	ld hl,(argp)
	ld e,(hl)
	inc hl
	ld d,(hl)
	inc hl
	ld (argp),hl
	ret

;
;	Action 52: Get an object providing it can be carried. If not display an
;	error message.
;
act52:
	ld a,(carried)
	cp MAXCAR			; Full up ?
	jr c,carok
	call get_arg16			; Eat the argument
	ld hl, toomuch			; And error
	jp strout_lower
carok:
	call get_arg		; E argument, HL ptr to O(arg), B = O(arg)
	ld c,255		; Move to carried
	jp move_item

;
;	General purpose object move. Tracks redraw and carried counter
;	status.
;
move_item_b:
	FIXME ( -> HL ptr make up, get loc)
	
;	HL = object ptr, B = current loc, A = new loc
;	moves object and fixes carried/redraw
;
move_item:
	ld c, a			; save new location
	cp (hl)			; not moving ?
	jr z,noop
	ld a,(location)		; was visible
	cp (hl)
	jr nz, notrdrw
	ld a,1
	ld (redraw),a		; so should redraw
notrdrw:
	ld a,255		; was carried ?
	cp (hl)
	jr nz, notlost
	ld a,(carried)
	dec a
	ld (carried),a
notlost:
	ld (hl), c		; move it
	ld a, 255
	cp c			; now carried ?
	jr nz,chkloc2
	ld a,(carried)
	inc a
	ld (carried),a		; so raise count
chkloc2:
	ld a,(location)		; moved into view ?
	ret nz
	ld a,(carried)
	inc a
	ld (carried),a
	ret
;
;	Action 53: Drop an object into the current location
;
act53:
	call get_arg
	ld a,(location)
	jr move_item

;
;	Action 54: Move to a location
;
act54:
	call get_arg
	ld (location),a
redrawit:
	ld a,1
	ld (redraw),a
	ret
;
;	Action 55/9: Destroy an object
;
act55:
act59:
	call get_arg
	xor a
	jp move_item
;
;	Action 56: Set the dark flag
;
act56:
	ld a,255
	ld (bitflags+DARKFLAG),a
	ret
;
;	Action 57: Clear the dark flag
;
act57:
	xor a
	ld (bitflags+DARKFLAG),a
	ret
;
;	Action 58: Set bit flag
;
act58:
	call get_arg
	ld hl,bitflags
	add hl,de
	ld (hl),255
	ret
;
;	Action 60: Clear bit flag
;
act60:
	call get_arg
	ld hl,bitflags
	add hl,de
	ld (hl),0
	ret

;
;	Action 61: Die, move to end of game
;
act61:
	ld hl,dead
	call strout_lower
	xor a
	ld (bitflags+DARKFLAG),a
	ld a,(lastloc)
	ld (location),a
	; fall through

;
;	Action 64,76: Look
;
act64:
act76:
	; look
	jp look
;
;	Action 62: Move an object to a given location
;	
act62:
	call get_arg		; HL is objloc ptr, B is location
	push hl
	push bc
	call get_arg		; New location into A
	pop bc
	pop hl
	jp move_item

;
;	Action 63: Game Over
;
act63:
	; throw an exception out of the interpreter
	ld hl,playagain
	call strout_lower
	call yes_or_no
	or a
	jr nz, reset
	jp start_game
reset:
	rst 0

;
;	Action 67: Set bit flag 0
;
act67:
	ld a,255
act67b:
	ld (bitflags),a
	ret

;
;	Action 68: Clear bit flag 0
;
act68:
	xor a
	jr act67b

;
;	Action 69: Refill the lamp
;
act69:
	ld a,(lightfill)
	ld (lighttime),a
	xor a
	ld (bitflags+LIGHTOUT),a
	ld hl,objloc+LIGHT_SOURCE
	ld b,(hl)
	ld a,255
	jp move_item

;
;	Action 70: Clear the screen
;
act70:
	; clear screen (some versions only)
	jp wipe_lower

;
;	Action 72: Swap two objects over
;
act72:
	call get_arg
	push hl		; Save objloc ptr for object 1
	push bc		; Save current location for object 1
	call get_arg	; Get object 2
	ld a,b		; A is now the location of object 2
	pop bc		; Recover location of object 1
	ld (hl),a	; Second object to first
	pop hl		; Pointer to object 1
	ld (hl),b	; Swapped over
	ld a, (location)
	cp (hl)
	jr z, placerd
	cp b
	ret nz
placerd:
	jp redrawit
;
;	Action 73: Set the continuation flag
;
act73:
	ld a,1
	ld (continuation),a
	ret

;
;	Action 74: Move object to inventory regardless of weight
;
act74:
	call get_arg
	ld a,255
	jp move_item
;
;	Action 75: Place one object with another
;
act75:
	call get_arg
	push hl
	push bc
	call get_arg
	ld a,b			; loc of second object is our target
	pop bc
	pop hl
	jp move_item

;
;	Action 77: Decrement counter
;
act77:
	ld hl, (counter)
	; Decrement if >= 0
	bit 7,h
	ret nz
	dec hl
	ld (counter),hl
	ret
;
;	Action 78: Print counter value
;
act78:
	ld a,(counter)
	jp decout_lower

;
;	Action 79: Set counter
;
act79:
	call get_arg16
	ld (counter),de
;
;	Action 80: Swap player location with saved room (YOHO etc)
;
act80:
	ld a,(location)
	ld b,a
	ld a,(savedroom)
	ld (location),a
	ld a,b
	ld (savedroom),a
	jp redrawit
;
;	Action 81: Swap current counter with counter n
;
act81:
	call get_arg
	ld hl, counter_array
	add hl,de
	add hl,de
	ld de, (counter)
	ld c,(hl)
	ld (hl),e
	inc hl
	ld b,(hl)
	ld (hl),d
	ld (counter),bc
	ret
;
;	Action 82: Add to current counter
;
act82:
	call get_arg16
	ld hl,(counter)
	add hl,de
	ld (counter),hl
	ret
;
;	Action 83: Subtract from current counter. Negative values all turn
;	into -1.
;
act83:
	call get_arg16
	ld hl,(counter)
	or a
	sbc hl,de
	jr nc, storecounter
	ld hl,0
storecounter:
	ld (counter),hl
	ret
;
;	Action 84: Print the noun string and a newline 
;
act84:
	call act86
;
;	Action 86: Print a newline
;
act86:
	ld hl,newline
	jp strout_lower

;
;	Action 84: Print the noun string
;
act85:
	ld hl,nounbuf
	jp strout_lower
;
;	Action 87: Swap the current location and saveroom flag n
;	(Claymorgue). Interestingly this is broken on the genuine 6809
;	interpreter !
;
act87:
	call get_arg
	ld hl,roomsave
	add hl,de
	ld a,(location)
	ld b,(hl)
	cp b
	ld (hl),a
	ld a,b
	ld (location),a
	ret z
	jp redrawit

;
;	Action 88: Two second delay
;
act88:
	ld bc,0
snooze:
	djnz snooze
	dec c
	jr nz,snooze
	ret

;
;	Action 89: Various. SAGA uses it to draw pictures, Seas of Blood
;	uses it to start combat.
;
act89:
	; Specials for SAGA etc
	jp get_arg

;
;	Action 65: Display the score
;
;
;	FIXME: should give percentages
;
;	The score is computed by counting treasures in the treasure room
;	and seeing how many we have. If we have all of them we report so and
;	quit. It's any oddity of the Scott Adams system that you have to
;	type "score" to win !
;
act65:
	ld hl,stored_msg
	call strout_lower

	ld hl,objloc
	ld bc,#NUM_OBJ*256	; c -> 0
score2:
	ld a,(treasure)
	cp (hl)
	jr nz,notintreas
	push hl
	call getotext_x		; Object texts start * for treasure
	ld a,KEY_STAR
	cp (hl)
	jr nz, notintreas
	inc c
notintreas:
	pop hl
	inc hl
	djnz score2
	; C is now the count to print
	push bc
	ld a,c
	call decout_lower
	ld hl, stored_msg2
	call strout_lower
	pop bc
	ld a,(treasures)
	cp c
	ret nz
	jp act63

;
;	Action 66: Display the inventory
;
act66:
	ld hl,carrying
	call strout_lower
	ld hl,objloc
	ld bc,256*NUM_OBJ	; c -> 0
objl:
	ld a,255
	cp (hl)
	jr nz, notgot
	bit 0,c
	jr z, inv_1
	push hl
	ld hl,dashstr
	call strout_lower
	pop hl
inv_1:	ld c,1
	push hl
	call getotext_x
	call strout_lower
	pop hl
notgot:
	inc  hl
	djnz objl
	bit 0,c
	jr nz,invstuff
	ld hl,nothing
	call strout_lower
invstuff:
	ld hl,dotnewline
	jp strout_lower

;
;	Table execution engine. Each table is a series of lines in the form
;	[[A][R][E][CCC][AA]][conditions][actions]
;
;	A bit indicates an "auto" or continuation action
;	R bit indicates no random %age is present if an auto action
;	E bit indicates auto is every time (AUTO 100)
;	CCC is the number of conditions
;	AA is the number of actions
;
;	Followed by condition.b, value.b several times and then by action.b
;	several times to form the full line.
;
;

run_table:
	xor a
	ld (continuation),a	; will get set by a continuation action
next_action:
	ld (linestart),hl
	ld a,(hl)
	ld (condacts),a
	bit 7,a			; AUTO flag
	jr nz,not_cont
	bit 5,a			; AUTO 100
	jr nz, not_random
	bit 6,a			; AUTO 0
	jr z, is_random
	ld a,(continuation)
	or a			; Skip continuations we didn't match
	jr z,next_line
	jr doing_cont
not_random:
	xor a
	ld (continuation),a	; AUTO 100 ends continuations
doing_cont:
	inc hl			; Skip header byte and go
	jr do_line
is_random:
	inc hl
	ld a, (hl)
	call random
	jr c, next_line
	inc hl			; Skip header byte and random number
	jp do_line
;
;	If we are doing continuations and hit a non continuation line
;	then we have finished. Otherwise the verb/noun must match for us
;	to process the line but once we have one match we stop.
;
not_cont:
	inc hl			; Skip header byte
	ld a,(actmatch)
	or a
	jr nz,action_done	; hit a new block - done
	ld e, (hl)		; FIXME: allow R bit on verb/noun pairs

	inc hl
	ld d,(hl)
	inc hl			; Skip verb / noun pair
	ld a,(verb)
	cp e
	jr nz,next_line
	ld a, (noun)
	cp d
	jr z, match_ok
	xor a
	cp d
	jr nz, next_line
match_ok:
	;
	;	Verb matches and noun matches or is not given
	;
	ld a,1
	ld (linematch),a
do_line:
	call perform_line	; run the conditions and actions
;
;	X is the head byte of the current line. Use this to find
;	the next line.
;
next_line:
	ld hl,(linestart)	; current line start in HL
	ld a,(hl)
	bit 7,a			; 0x80 - vocab bytes omitted
	jr nz,squashed
	inc hl			; Move past header
	inc hl			; Move past verb
squashed1:
	ld d,0
	inc hl			; move on to the conditions and actions
	ld b,a
	and 3			; actions in the low 2 bits
	inc a			; 1-4 not 0-3
	ld e,a
	add hl,de
	ld a,b
	rrca			; 2 x conditions
	rrca
	and 0x0E		; 2 * conds
	ld e,a
	add hl,de
	ld a,255		; 255 is end of table
	cp (hl)			; round we go
	jr z, action_done
	jp next_action
squashed:
	bit 6,a
	jr nz, squashed1	; 0x40/0x20 = random %age suppressed
	bit 5,a
	jr nz,squashed1		; squashed1 skips the header
	inc hl			; skip the random %age as well
	jr squashed1

;
;	All completed. If this was an action then consider builtins, if not
;	we are done
;
action_done:
	ld a,(verb)
	cp 255
	ret z
builtins:
	ld a,(linematch)
	ret nz
	; FIXME - some games have specials for get/put all. Some games
	; have no builtins (early)
	ld a,(verb)
	cp VERB_GET		; 10
	jr nz,not_get
	;
	;	Automatic get handler
	;
	ld a,(noun)
	or a
	jr z, ummwhat
	ld a, (maxcar)
	ld b, a
	ld a, (carried)
	cp b
	jr nc, cancarry
	ld hl,toomuch
	call strout_lower
	jr all_done
cancarry:
	ld a,(location)
	call autonoun		; Find the object of this noun if any into B
	cp 255
	jr nz, knownobjg
bpower:
	ld hl,beyondpower
	call strout_lower
	jr all_done
knownobjg:
	ld a,255
domove:
	call move_item_b
	ld hl,okmsg
	call strout_lower
	jr all_done
not_get:
	;
	;	Automatic drop handler
	;
	cp VERB_DROP		; 18
	jr nz,not_drop
	ld a,(noun)
	jr z,ummwhat
	ld a,255
	call autonoun
	cp 255
	jr z,bpower
	ld a,(location)
	jr domove

all_done:
	ld a,1
	ld (actmatch),a		; auto logic "did" the action
not_drop:
	ret

ummwhat:
	ld hl,whatstr
	jp strout_lower

;
;	On entry B holds the location to scan. We check for
;	any auto entry that matches our word and is in the
;	correct location, then return that or 0xff if no match.
;
autonoun:
	ld hl, automap
	ld c,a			; location to match
	ld a,(wordbuf)
	cp SPACE
	jr z, noauto		; 0 - none
	cp 255
	jr z, noauto
autonounl:
	call wordeq
	jr z, foundnoun
nextnoun:
	ld de, WORDSIZE+1	; 5 bytes per entry
	add hl, de
	ld a, (hl)
	or a
	jr nz, autonounl
noauto:
	ld a,255		; Word present but not in vocabulary
	ret
foundnoun:
	push hl
	ld de,WORDSIZE		; d will be zero
	add hl,de
	ld e, (hl)
	ld hl,objloc
	add hl,de
	ld a,(hl)		; location
	cp c
	jr z,objmatch
	pop hl
	jr nextnoun
objmatch:
	pop hl
	ret

getloc_x:			; Get location ptr into HL - avoid shifts
	push de
	ld a,(location)
	ld l,a
	ld h,0
	add hl,hl
	push hl
	add hl,hl
	pop de
	add hl,de
	ld de,locdata		; as we might overflow
	add hl,de
	pop de
	ret

;
;	Look: Display the location details. This ends up in the upper
;	window, as the Scott Adams' system uses a two window output model.
;
look:
	call start_upper	; Clear out the old
	call islight		; See if it is dark
	jr z,cansee		; Nope
	ld hl,itsdark		; "It is dark"
	call strout_upper
	jp end_upper		; and done

cansee:
	call getloc_x		; Find the right location data
	push hl			; Save our pointer
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a			; Get the message ptr
	ld a,KEY_STAR		; Is it *
	cp (hl)
	jr z,notshort		; Nope.. just print it
	push hl			; Save it
	ld hl,youare		; Standard game prefix ("You are", "I am")
	call strout_upper
	pop hl
	dec hl
notshort:
	inc hl			; Skip *
	call strout_upper	; Print the location text
	ld hl,obexit		; Exits string
	call strout_upper
	pop hl			; Recover the location ptr
	inc hl			; Move on to exits
	inc hl
	ld bc,0			; Count exits, c = 0 (no exit seen)
exitl:
	xor a
	cp (hl)			; 0 = none
	jr z, notanexit
	push hl			; Save our pointer
	bit 0,c			; First exit ?
	jr z,firstexit
	ld hl,dashstr		; Print - or , 
	call strout_upper
firstexit:
	set 0,c			; No longer first exit
	ld hl #exitmsgptr	; Exit messages
	ld e,b
	ld d,0
	add hl,de
	add hl,de
	ld a,(hl)
	inc hl
	ld l,(hl)
	ld h,a
	call strout_upper	; Print it
	pop hl			; Get our exits pointer back
notanexit:
	inc hl			; Move on
	inc b			; Done ?
	ld a,6
	cp b
	jr nz, exitl
	bit 0,c			; No exits ?
	jr nz, wasstuff
	ld hl,nonestr		; "None"
	call strout_upper
wasstuff:
	ld hl,dotnewline	; "."
	call strout_upper
	ld bc, 256 * NUM_OBJ	; b = obj count, c = no objects seen
	ld hl, objloc
lookiteml:
	ld a,(location)
	cp (hl)			; Object here ?
	jr nz, objnothere
	push hl
	ld hl,dashstr
	bit 0,c
	jr nz,notfirsti
	ld hl,canalsosee
notfirsti:
	set 0,c			; Object seen
	call strout_upper
	pop hl			; Recover our object pointer
	push hl
	call getotext_hl	; Print the object name
	call strout_upper
	pop hl
objnothere:
	inc hl
	djnz lookitem1
	bit 0,c
	jr z, nothingtosee
	ld hl, dotnewline	; Finish up
	call strout_upper
nothingtosee:
	jp end_upper		; Draw the barrier line and donee

start_game:
	ld sp, stacktop
setup_obj:
	ld hl, objinit
	ld de, objloc
	ld bc, NUM_OBJ
	ldir
	ld hl, zeroblock	; Range to wipe
	ld de, zeroblock+1
	ld bc, zeroblock_end - zeroblock - 1
	ld (hl),0
	ldir
	ld a,(startloc)
	ld (location),a
	ld a,(startlamp)
	ld (lighttime),a
	ld a,(startcarried)
	ld (carried),ap
	ld (leftx), a
	ld (highx), a
	ld hl, justbuf
	ld (justify), hl
	call wipe_screen
	call redrawit
	jp main_loop	

nextchar:
	.dw $4200
lowtop:
	.dw $4020
nextupper:
	.dw $4000
leftx:
	.db 0
highx:
	.db 0
lineptr:
	.dw linebuf
linebuf:
	.ds 30		; buffer for input
wordbuf:
	.ds 4
nounbuf:
	.dw 0
redraw:			; Top display is dirty
	.db 0


wordflush:
;
;	Action 71: Save the game position
;
act71:
	ret

;
;	Action Table
;
actab:
	.dw act52
	.dw act53
	.dw act54
	.dw act55
	.dw act56
	.dw act57
	.dw act58
	.dw act59
	.dw act60
	.dw act61
	.dw act62
	.dw act63
	.dw act64
	.dw act65
	.dw act66
	.dw act67
	.dw act68
	.dw act69
	.dw act70
	.dw act71
	.dw act72
	.dw act73
	.dw act74
	.dw act75
	.dw act76
	.dw act77
	.dw act78
	.dw act79
	.dw act80
	.dw act81
	.dw act82
	.dw act83
	.dw act84
	.dw act85
	.dw act86
	.dw act87
	.dw act88
	.dw act89

;
;	Abbreviations: FIXME - might be nice to separate these for non
; English games ?
;
;	FIXME: need to be generated in charset of target!
;
a_nort: .ascii "NORTH"
a_sout: .ascii "SOUTH"
a_east: .ascii "EAST"
	.db 0
a_west: .ascii "WEST"
	.db 0
a_up:   .ascii "UP"
	.db 0
a_down: .ascii "DOWN"
	.db 0
a_inve: .ascii "INVEN"
;
;	Engine temporaries
;
verb:
	.db 0
noun:
	.db 0
tmp8:
	.db 0
tmp8_2:
	.db 0
tmp16:
	.dw 0
linestart:
	.dw 0
linematch:
	.db 0
actmatch:
	.db 0
condacts:
	.db 0		; condact header byte for this line
continuation:
	.db 0
argh:
	.db 0		; argh high byte for last
argp:
	.dw 0
args:
	.ds 10			; max 5 parameters
justbuf:
	.ds 32
justify:
	.dw 0

	.ds 256			; overkill
stacktop:
	.db 0

;
;	Between here and saveblock_end is saved
;
saveblock:

carried:
	.db 0
lighttime:
	.db 0
location:
	.db 0

	.db 0,0,0,0,0
objloc:
	.ds NUM_OBJ
objloc_end:

;
;	Between here and zeroblock_end is wiped each new game
;

zeroblock:

roomsave:
	.ds 12		; Seem to be sufficient (6 room saves)
savedroom:
	.db 0		; single flag for this type
bitflags:
	.ds NUM_BITS
counter:
	.dw 0
counter_array
	.ds 32		; 16 counters

zeroblock_end:
saveblock_end:

