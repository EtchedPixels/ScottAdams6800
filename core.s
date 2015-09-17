;
;	Major items to do
;
;	Save and Load
;	Rearrange to maximise use of branches via bsr/bra
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
	jmp start_game

;
;	Wipe the screen
;
wipe_screen:
	ldx #$4000
wipeblock:

1	ldd #$2020
1wiper:
1	std ,x
1	inx
1	inx

0	ldab #$20
0wiper:
0	stab ,x
0	inx
	cpx #$4200
	bne wiper
	rts

wipe_lower:
	ldx lowtop
	bra wipeblock
;
;	Scroll the lower window
;
scroll_lower:
	ldx lowtop
lowscrl:

1	ldd 32,x
1	std ,x
1	inx
1	inx

0	ldaa 32,x
0	staa ,x
0	inx

	cpx #$4200-32		; last line for scroll work
	bne lowscrl
	ldaa #$20
lowclr:
	staa ,x
	inx
	cpx #$4200
	bne lowclr
	rts

;
;	Start using the upper area. Clear the old upper zone
;
;	Assumption: upper is never empty
;
start_upper:
	jsr word_flush_l
	ldx #$4000
1	ldd #$2020
0	ldaa #$20
wipeoldupper:
0	staa ,x
1	std ,x
1	inx
	inx
	cpx lowtop
	bne wipeoldupper
	; Move the upper position
	ldx #$4000
	stx nextupper
	rts
;
;	Finished using the upper area. Draw the divider and set up for
;	the low section
;
end_upper:
	jsr word_flush_u
0	ldaa nextupper
0	ldab nextupper+1
0	addb #$1f
0	adca #0

1	ldd nextupper
1	addd #$1F

	andb #$E0
0	staa lowtop
0	stab lowtop+1
1	std lowtop
	ldx lowtop
0	ldaa #$7C
0	ldab #$20
1	ldd #$7C20	; 7C is < for 32 bytes
divider:
	staa ,x
	inx
	eora #$2
	decb
	bne divider
	stx lowtop
	rts
;
;	Print to the upper screen area. We never handle characters wrapping
;	as the higher level justifier is responsible for that
;
chout_upper:
	cmpa #' '
	beq is_space_u
	cmpa #10
	beq upper_nl
	;
	;	Normal text into the justify buffer
	;
chout_queue:
	anda #63
	ldx justify
	staa ,x
	inx
	stx justify
nonl_u:
	rts
	;
	;	Newline
	;
upper_nl:
	bsr word_flush_u
upper_nl_raw
	ldab nextupper+1	; Next position
	andb #$1f		; X only
	ldaa #$20		; Length
	sba			; Bytes remaining
	ldab #$20
	ldx nextupper
upper_spc:			; Clear the rest of the line
	stab ,x
	inx
	deca
	bne upper_spc
	bra upper_st_out
	;
	;	Space is much like newline and we should probably
	;	merge them
	;
is_space_u:
	bsr word_flush_u	; Flush the word if any
	ldaa nextupper+1	; Left hand row ?
	anda #$1f
	beq no_output_u		; No trailing spaces printed
	ldaa #' '
	;
	;	Print an upper screen character directly
	;
chout_upper_raw:
	ldx nextupper
	staa ,x
	inx
upper_st_out:
	stx nextupper
no_output_u:
	rts

word_flush_u:
; FIXME 6800
	ldd justify
	subd #justbuf		; space required is now in b
	ldaa nextupper+1
	anda #$1f		; column
	aba
	cmpa #$1f		; wrapping ?
	blt no_wrap_u		; it fits
	jsr upper_nl_raw
no_wrap_u:
	ldx #justbuf
wordfl_u_lp:
	cpx justify
	beq wordflush_u_done
	ldaa ,xp
0	stx justu_x+1
1	pshx
	bsr chout_upper_raw
0justu_x:
0	ldx #0
1	pulx
	inx
	bra wordfl_u_lp
wordflush_u_done:
	ldx #justbuf
	stx justify
noscroll_l:
	rts
;
;	Print to the lower screen area
;
chout_lower:
	cmpa #' '
	beq is_space_l
	cmpa #10
	beq lower_nl
	bra chout_queue
	;
	;	Newline
	;
lower_nl:
	bsr word_flush_l
	ldaa nextchar+1		; Did we just wrap anyway ?
	anda #$1f
	beq noscroll_l
lower_nl_raw:
	jsr scroll_lower
	ldx #$4200-32
	bra lower_st_out	; Save the position
	;
	;	Space is much like newline and we should probably
	;	merge them
	;
is_space_l:
	bsr word_flush_l	; Flush the word if any
	ldaa nextchar+1		; Left hand row ?
	anda #$1f
	beq no_output_l		; No trailing spaces printed
	ldaa #' '
	;
	;	Print a lower screen character directly
	;
chout_lower_raw:
	ldx nextchar
	cpx #$4200
	bne not_scroll
	psha
	jsr scroll_lower
	ldx #$4200-32
	pula
not_scroll:
	staa ,x
	inx
lower_st_out:
	stx nextchar
no_output_l:
	rts

word_flush_l:
; FIXME 6800
	ldd justify
	subd #justbuf		; space required is now in b
	ldaa nextchar+1
	anda #$1f		; column
	aba
	cmpa #$20		; wrapping ?
	blt no_wrap_l		; it fits
	jsr lower_nl_raw
no_wrap_l:
	ldx #justbuf
wordfl_l_lp:
	cpx justify
	beq wordflush_l_done
	ldaa ,xp
0	stx just_x+1
1	pshx
	bsr chout_lower_raw
0just_x:
0	ldx #0
1	pulx
	inx
	bra wordfl_l_lp
wordflush_l_done:
	ldx #justbuf
	stx justify
	rts
;
;	Print a string of text to the lower window
;
strout_lower:
	pshb
strout_lower_l:
	ldaa ,x
	beq strout_done
0	stx stroutl_x
1	pshx
	jsr chout_lower
0	ldx stroutl_x
1	pulx
	inx
	bra strout_lower_l
strout_done:
	pulb
	rts

0stroutl_x:
0	fdb 0

;
;	Print a sring of text to the lower window followed by a space
;
strout_lower_spc:
	bsr strout_lower
0	stx stroutl_x
1	pshx
	ldaa #$20
	jsr chout_lower
0	ldx stroutl_x
1	pulx
	rts
;
;	Print a string of text to the upper window
;
strout_upper:
	pshb
strout_upper_l:
	ldaa ,x
	beq strout_done
0	stx stroutu_x
1	pshx
	jsr chout_upper
0	ldx stroutu_x
1	pulx
	inx
	bra strout_upper_l

0stroutu_x:
0	fdb 0


hexoutpair_lower:
	pshb
	jsr hexout_lower
	pula
;
;	Hexadecimal output to lower window (debug only)
;
hexout_lower:
	psha
	rora
	rora
	rora
	rora
	bsr hexdigit
	pula
hexdigit:
	anda #$0F
	cmpa #$0A
	blt hexout_digit
	adda #$07
hexout_digit:
	adda #'0'
	jmp chout_lower

;
;	Decimal output to lower window (0-99 is sufficient)
;	
decout_lower:
	clrb
decout_div:
	suba #$0A
	bcs decout_mod
	incb
	bra decout_div
decout_mod:
	adda #$0A		; correct overrun
	psha
	tba
	bsr hexdigit
	pula
	bra hexdigit

	
;
;	Keyboard
;
read_key:
	ldx $FFDC
	jsr ,x
	beq read_key
;
;	0D - newline, 08 - delete
; 
	cmpa #0x0D
	beq key_cr		; we don't want to stir in the newline
				; as it and the random query will be a fixed
				; number of clocks apart in many cases
	jmp randseed		; add to the randomness

;
;	Wait for newline
;
wait_cr:
	bsr read_key
	cmpa #$0D
	bne wait_cr
key_cr:
	rts

;
;	Yes or No. Returns B = 0 for Yes, 0xFF for no
;
yes_or_no:
	jsr word_flush_l
	clrb
	bsr read_key
	cmpa #'y'
	beq is_yes
	cmpa #'Y'
	beq is_yes
	cmpa #'J'
	beq is_yes	; German Gremlins!
	cmpa #'j'
	beq is_yes
	cmpa #'n'
	beq is_no
	cmpa #'N'
	bne yes_or_no
is_no:	decb
is_yes:	rts

;
;	Line editor
;
line_input:
	dec nextchar+1		; won't wrap the page so safe
	ldx nextchar
	ldaa #96
	staa ,x			; initial cursor
	ldx #linebuf
	stx lineptr
line_loop:
	bsr read_key
	ldx lineptr
	cmpa #$08
	beq delete_key
	cmpa #$0D
	beq enter_key
	cmpa #' '
	blt line_loop
	cpx #linebuf+29		; 32 minus "> " and cursor
	beq line_loop		; didn't fit
	staa ,x
	inx
	stx lineptr
	ldx nextchar
	anda #63
	staa ,x
	inx
	ldaa #96
line_st:
	staa ,x
	stx nextchar
	bra line_loop
delete_key:
	cpx #linebuf
	beq line_loop
	dex
	stx lineptr
	ldx nextchar
	ldaa #' '
	staa ,x
	dex
	ldaa #96
	bra line_st
enter_key:
	clr ,x			; Mark the end of the buffer
	ldx nextchar		; Clean up the cursor
	ldaa #' '
	staa ,x
	ldaa #10
	jmp chout_lower		; Move on a line and return


;
;	Machine specific but for 6801 we can do this
;
random:
	tab
	aba		;	a = 2 x chance
	lsrb		;	b = 0.5 x chance
	aba		;	gives us 2.5 x the 1 in 100 chance
			;	versus 0-255
	ldab randpool	;	pool scrambling
	eorb $10	;	stir in timer
	rolb
	adcb #0		;	rollover
	stab randpool	;	stirred
	clrb		;	B = 0 or 1
1	cmpa randpool	;	use the timer pool
	sbcb #0		;	0x00 or 0xFF for no/yes
	rts
randpool:
	fcb	0

randseed:
	pshb
	ldab randpool
	lslb
	adcb $10
	stab randpool
	pulb
	rts

;
;	Parser
;


;
;	Find a word in the verb table
;
whichverb:
	ldx #verbs
	bra whichword
;
;	Find a word in the noun table
;
whichnoun:
	ldx #nouns
;
;	Find a word. The approach used is weird but we copy it to keep the
;	numbering. Aliases are the same as the non alias word, but all words
;	are numbered. So if word 2 is an alias then word 2 returns 1 but
;	word 3 returns 3
;
whichword:
	clr tmp8
	clr tmp8_2
	ldaa wordbuf
	beq foundword		; 0 - none
	cmpa #32
	beq foundword
	dec tmp8		; -1	
whichwordl:
	inc tmp8
	ldab ,x
	bitb #$80		; alias - don't bump code
	bne alias
	ldaa tmp8
	staa tmp8_2
alias:
	jsr wordeq
	beq foundword
	ldab #WORDSIZE
1	abx
0	jsr abx
	tst ,x			; 0 - end of list
	bne whichwordl
	ldab #$ff		; Word present but not in vocabulary
	rts
foundword:
	ldab tmp8_2		; Load word into B
skipdone:
	rts

;
;	X points to the input. Skip any spaces
;
skip_spaces:
	ldaa #32
skipl:
	cmpa ,x
	bne skipdone
	inx
	bra skipl

;
;	Scan the input for a verb / noun pair. German language games are a
;	bit different so this won't work for them
;
scan_input:
	ldx #linebuf
	jsr copy_word		; Verb hopefully
0	stx scanin_x
1	pshx
	jsr whichverb
	stab verb
0	ldx scanin_x
1	pulx
scan_noun:			; Extra entry point used for direction hacks
	jsr copy_word
	jsr whichnoun
	stab noun
	rts

0scanin_x:
0	fdb	0


;
;	See if we have a relevant one word abbreviation
;
abbrevs:
	ldx #linebuf
	jsr skip_spaces
	ldaa 1,x
	beq abbrev_ok
	cmpa #' '
	beq abbrev_ok
	rts
abbrev_ok:
	ldaa ,x
	ldx #a_nort
	cmpa #'N'
	beq do_abbr
	ldx #a_sout
	cmpa #'S'
	beq do_abbr
	ldx #a_east
	cmpa #'E'
	beq do_abbr
	ldx #a_west
	cmpa #'W'
	beq do_abbr
	ldx #a_up
	cmpa #'U'
	beq do_abbr
	ldx #a_down
	cmpa #'D'
	beq do_abbr
	ldx #a_inve
	cmpa #'I'
	beq do_abbr
	rts
do_abbr:
	jmp copy_abbrev
;	ldx #linebuf
;	jmp strout_lower

;
;	Main game logic runs from here
;
main_loop:
	jsr look
	bra do_command_1

do_command:
;
;	Implement the builtin logic for the lightsource in these games
;
	ldab objloc+LIGHT_SOURCE
	beq do_command_1
	ldaa lighttime
	cmpa #255			; does  not expire
	beq do_command_1
	deca
	bne light_ok
	clr bitflags+LIGHTOUT		; light goes out
	cmpb #255
	beq seelight
	cmpb location
	bne unseenl
seelight:
	ldx #lightout
	jsr strout_lower
unseenl:
; Earliest engine only
;	clr objloc+LIGHT_SOURCE
	inc redraw
	bra do_command_1
light_ok:
	cmpa #25			; warnings start here
	bhi do_command_1		; FIXME: some games do a general
	ldx #lightoutin			; warning every five instead
	psha
	jsr strout_lower
	pula
	psha
	jsr decout_lower
	ldx #turns
	pula
	cmpa #1
	bne turns_it_is
	ldx #turn
turns_it_is:
	jsr strout_lower

; Fall through	
;
;	We start by running the status table. All lines in this table are
;	automatic actions or random %ages. We could possibly squash it a bit
;	more by not using the same format, but it's not clear the code
;	increase versus data decrease is a win
;
do_command_1:
	clr verb		; indicate status
	clr noun
	ldx #status
	jsr run_table		; run through the table
	tst redraw		; do a look if the status table moved
	beq no_redraw		; anything in or out of view, or moved us
	jsr look
	clr redraw
no_redraw:
do_command_2:
	ldx #whattodo		; prompt the user
	jsr strout_lower
do_command_l:
	ldx #prompt
	jsr strout_lower
	jsr wordflush		; avoid buffering problems
	jsr line_input		; read a command
	jsr abbrevs		; do any abbreviation fixups
	ldx #linebuf
	jsr skip_spaces
	tst ,x			; empty ?
	beq do_command_l	; round we go
0	stx docoml_x
1	pshx
	jsr scan_noun		; take the first input word and see
	ldaa noun		; if its a direction
	beq notdirn
	cmpa #6
	bhi notdirn
	ldaa #VERB_GO
	staa verb		; convert this into "go foo"
0	ldx docoml_x
1	pulx
	bra parsed_ok

0docoml_x:
0	fdb	0
;
;	Try a normal verb / noun parse
;
notdirn:
0	ldx docoml_x
1	pulx
	jsr scan_input
;	pshx
;	ldaa #'V'
;	jsr chout_lower
;	ldaa verb
;	jsr hexout_lower
;	ldaa #'N'
;	jsr chout_lower
;	ldaa noun
;	jsr hexout_lower
;	pulx
	ldaa verb
	beq do_command_l	; no verb given ?
	cmpa #$ff
	bne parsed_ok
	ldx #dontknow		; no verb, error and back to the user
	jsr strout_lower
	jmp do_command_2


parsed_ok:
;
;	We have a verb noun pair
;
;	Hardcoded stuff first (yes this is a bad idea but it's how the
;	engine works)
;
	ldaa verb
	cmpa #VERB_GO		; GO [direction]
	bne not_goto
	ldab noun
	tstb
	bne not_goto_null
	ldx #givedirn		; Error "go" on it's own
	jsr strout_lower
	bra do_command_2
not_goto_null:
	cmpb #6
	bhi not_goto		; not a compass direction
	pshb
	clr tmp8
	jsr islight		; check if it is dark
	beq not_dark_goto
	inc tmp8		; temporary dark flag
	ldx #darkdanger		; warn the user
	jsr strout_lower
not_dark_goto:
	jsr getloc_x		; look up direction table
	pulb
0	jsr abx
1	abx			; add direction (1-6)
	inx			; allow for the fact 2 bytes of text ptr
	tst ,x			; valid ?
	bne can_do_goto
	tst tmp8		; falling in the dark ?
	beq movefail		; light so ok
	ldx #brokeneck		; tell the user
	jsr strout_lower
	jsr act88		; do a delay
	jsr act61		; and die
	jmp do_command
movefail:
	ldx #cantgo		; tell the user they can't move
	jsr strout_lower
	jmp do_command_2
can_do_goto			; a goto that works
	ldab ,x			; b is new location
	stab location		; move
	inc redraw		; will need to redraw
	bra do_command_far
;
;	Then the tables (the goto first is a design flaw in the Scott Adams
;	system)
;
not_goto:
	clr linematch		; match state for error reporing.
	clr actmatch
	ldx #actions		; run the action table
	jsr run_table
	tst actmatch		; we did actions, so all was good
	bne do_command_far
	ldx #dontunderstand
	tst linematch		; got as far as conditions
	beq notnotyet		; a match exists but conds failed ?
	ldx #notyet		; give user a clue if so
notnotyet:
	jsr strout_lower		; display error
do_command_far:
	jmp do_command

;
;	Useful Helpers
;

;
;	Check if we are in the light (Z) or not (NZ)
;
islight:
	tst bitflags+DARKFLAG		; if it isn't dark then it's light
	beq lighted
	ldaa objloc+LIGHT_SOURCE	; get the lamp
	cmpa #255			; carried ?
	beq lighted
	cmpa location			; in the room ?
lighted:
	rts

;
;	Given X is the objloc pointer of an object return its text string.
;	This looks an odd interface but all our callers happen to have this
;	X value directly to hand and we are quite register limited.
;
getotext_x:
	psha
	pshb
	stx tmp16	; Need to go X to D
	ldab tmp16+1	; Only the low offset matters
	subb #objloc % 256
	ldx #objtext	; Two byte pointer per object
0	jsr abx
0	jsr abx
1	abx
1	abx
	ldx ,x		; Dereference
	pulb
	pula
	rts

set_argp:
0	ldaa #args/256
0	ldab #args%256
0	staa argp
0	stab argp+1

1	ldd #args
1	std argp

	rts
;
;	Game Conditions
;
perform_line:
;	jsr debug
	bsr set_argp
	clra
	clrb
1	std args
1	std args+2
1	std args+4
1	std args+6
1	std args+8
0	clr args
0	clr args+1
0	clr args+2
0	clr args+3
0	clr args+4
0	clr args+5
0	clr args+6
0	clr args+7
0	clr args+8
0	clr args+9
	ldab condacts
	rorb
	rorb
	andb #$07		; B is condition count
	beq noconds
condl:
;	jsr debug2
0	stx condl_x
1	pshx
	pshb
	bsr cond		; run the condition
	pulb
0	ldx condl_x
1	pulx
	tsta			; did it fail
	bne nextrow		; if so we the line is done
	inx			; move on a condition
	inx
	decb
	bne condl		; done yet
;
;	Now we can process the actions
;
noconds:
	inc actmatch
	bsr set_argp		; reset the arg pointer
	ldab condacts		; see how many actions
	andb #3
	incb			; 1-4 not 0-3
nextact:
;	jsr debug3
	ldaa ,x			; get the action code
	inx			; move on
0	stx condl_x
1	pshx
	pshb
	jsr act			; run the action
	pulb
0	ldx condl_x
1	pulx
	decb
	bne nextact		; until done
nextrow:
	rts

0condl_x:
0	fdb	0

;
;	?? Does this need to sign extend ??
;
arghigh:
	ldaa argh
	rola
	rola
	rola
	anda #$07		; High bits
	rts

;
;	Run conditions. We do this odd dec/bra sequence to save us
;	registers. It's probably a mistake and we should use a table
;
cond:
0	ldaa ,x
0	ldab 1,x
1	ldd ,x			; A = cond, B = value
	staa argh		; High arg bits saved
	anda #$1F		; Low bits only
	tsta
	bne cond1
;
;	Condition 0 is "parameter", and always true. It saves a parameter
;	for the actions to use
;
	ldx argp
	stab 1,x
	bsr arghigh
	staa ,x
	inx
	inx
	stx argp
;
;	True condition
;
condnp:
	clra
	rts
cond1:
;
;	Many conditions want the object referenced by the argument. We
;	set x up as a pointer to the argument
;
	ldx #objloc
0	jsr abx
1	abx
	deca
;
;	Condition 1: true if the objct is carried
;
	bne cond2
	ldaa ,x
	cmpa #255
;
;	General purpose "true if eq"
;
cbrat:
	beq condnp
condfp:
	ldaa #255
	rts
cond2:
	deca
	bne cond3
;
;	Condition 2: true if the object is in the same room
;
	ldaa ,x
cond2c:
	cmpa location
	bra cbrat
cond3:
	deca
	bne cond4
;
;	Condition 3: true if the object is in the same room or carried
;
	ldaa ,x
	cmpa #255
	beq condnp
	bra cond2c
cond4:
	deca
	bne cond5
;
;	Condition 4: true if the player is in a given place
;
	cmpb location
	bra cbrat
cond5:
	deca
	bne cond6
;
;	Condition 5: true if the object is not in the same room
;
	ldaa ,x
	cmpa location
;
;	General purpose true if not eq
;
cbraf:
	bne condnp
	bra condfp
cond6:
	deca
	bne cond7
;
;	Condition 6: true if the object is not carried
;
	ldaa ,x
	cmpa #255
	bra cbraf
cond7:
	deca
	bne cond8
;
;	Condition 7: true if the player is not in a given place
;
	cmpb location
	bra cbraf
cond8:
	deca
	bne cond9
;
;	Condition 8: True if bitflag n is set
;
	ldx #bitflags
0	jsr abx
1	abx
	tst ,x
	bra cbraf
cond9:
	deca
	bne cond10
;
;	Condition 9: True if bitflag n is clear
;
	ldx #bitflags
0	jsr abx
1	abx
	tst ,x
	bra cbrat
cond10:
	deca
	bne cond11
;
;	Condition 10: Carrying any objects
;
	tst carried
	bra cbraf
cond11:
	deca
	bne cond12
;
;	Condition 11: Not carrying any objects
;
	tst carried
	bra cbrat
cond12:
	deca
	bne cond13
;
;	Condition 12: Object not carried or in location
;
	ldaa ,x
	cmpa #255
	beq condfp
	cmpa location
	bra cbrat
cond13:
	deca
	bne cond14
;
;	Condition 13: Object not destroyed (room 0)
;
	ldaa ,x
	bra cbraf
cond14:
	deca
	bne cond15
;
;	Condition 14: Object is destroyed (room 0)
;
	ldaa ,x
	bra cbrat_f
cond15:
	deca
	bne cond16
;
;	Condition 15: Current counter is <= arg
;
	jsr arghigh
	cmpa counter
	bgt condnp_f
	cmpb counter+1
	bgt condnp_f
condfp_f:
	jmp condfp
condnp_f:
	jmp condnp
cond16:
	deca
	bne cond17
;
;	Condition 16: Current counter is >= arg
;
	jsr arghigh
	cmpa counter
	blt condnp_f
	cmpb counter+1
	blt condnp_f
	bra condfp_f
cond17:
	deca
	bne cond18
;
;	Condition 17: Object is in its original location
;
	ldaa ,x
	ldx #objinit
0	jsr abx
1	abx
	cmpa ,x
	bra cbrat_f
cond18:
	deca
	bne cond19
;
;	Condition 18: Object is not in its original location
;
	ldaa ,x
	ldx #objinit
0	jsr abx
1	abx
	cmpa ,x
	jmp cbraf
cond19:
	deca
	bne condbad
;
;	Condition 19: Counter is equal to value.
;
	cmpb counter+1
	bne cbrat_f
	jsr arghigh
	cmpa counter
cbrat_f:
	jmp cbrat
;
;	If we get this far it is not a valid condition
;
condbad:
	ldx #invcond
	jsr strout_lower
halted:
	bra halted


;
;	Game Actions
;

;
;	Print a message given by A-51
;
msg2:
	suba #50
;
;	Print message given by A
;
msg:
	tab
	ldx #msgptr
0	jsr abx
0	jsr abx
1	abx
1	abx
	ldx ,x
	jmp strout_lower_spc

;
;	Action 0 (shouldn't appear)
;
noop:
	rts

;
;	Process the actions. We handle these via a jump table
;
act:
	tsta
	beq noop
;
;	Codes < 52 are messages, codes >= 102 are the second lot of
;	messages. Historical bad planning ?
;
	cmpa #52
	bcs msg
	cmpa #102
	bcc msg2
;
;	Real action
;
	tab
	ldx #actab-104  ; First action is 52, 2 bytes each
0	jsr abx
0	jsr abx
1	abx
1	abx		; Find our entry
	ldx ,x
	jmp ,x		; Off we go

;
;	Collect a parameter argument (low 8bits) and put it into
;	A. Return with A = arg, X = objloc ptr to object A and B = location
;	of object A
;
get_arg:
	ldx argp
	inx
	ldab ,x		; Low byte
	inx
	stx argp
	ldx #objloc
0	jsr abx
1	abx
	tba		; Argument to A
	ldab ,x		; Location to B
	rts

;
;	Retrieve a 16bit parameter in D
;
get_arg16:
	ldx argp
0	ldaa ,x
0	ldab 1,x
1	ldd ,x
	inx
	inx
	stx argp
	rts

;
;	Action 52: Get an object providing it can be carried. If not display an
;	error message.
;
act52:
	ldaa carried
	cmpa maxcar			; Full up ?
	blt carok
	inc argp			; Eat the argument
	inc argp
	ldx #toomuch			; And error
	jmp strout_lower
carok:
	bsr get_arg		; A argument, X ptr to O(arg), B = O(arg)
	ldaa #255		; Move to carried
	bra move_item

;
;	General purpose object move. Tracks redraw and carried counter
;	status.
;
move_item_b:
	ldx #objloc
0	jsr abx
1	abx
	ldab ,x
	
;	X = object ptr, B = current loc, A = new loc
;	moves object and fixes carried/redraw
;
move_item:
	cmpa ,x			; not moving ?
	beq noop
	cmpb location		; was visible
	bne notrdrw
	inc redraw		; so should redraw
notrdrw:
	cmpb #255		; was carried ?
	bne notlost
	dec carried		; so drop count
notlost:
	staa ,x			; move it
	cmpa #255		; now carried ?
	bne chkloc2
	inc carried		; so raise count
chkloc2:
	cmpa location		; moved into view ?
	bne notrdrw2
	inc redraw
notrdrw2:
	rts

;
;	Action 53: Drop an object into the current location
;
act53:
	bsr get_arg
	ldaa location
	bra move_item

;
;	Action 54: Move to a location
;
act54:
	bsr get_arg
	staa location
redrawit:
	inc redraw
	rts

;
;	Action 55/9: Destroy an object
;
act55:
act59:
	bsr get_arg
	clra
	jmp move_item
;
;	Action 56: Set the dark flag
;
act56:
	ldaa #255
	staa bitflags+DARKFLAG
	rts
;
;	Action 57: Clear the dark flag
;
act57:
	clr bitflags+DARKFLAG
	rts
;
;	Action 58: Set bit flag
;
act58:
	jsr get_arg
	tab
	ldx #bitflags
0	jsr abx
1	abx
	ldaa #255
	staa ,x
	rts

;
;	Action 60: Clear bit flag
;
act60:
	jsr get_arg
	tab
	ldx #bitflags
0	jsr abx
1	abx
	clr ,x
	rts

;
;	Action 61: Die, move to end of game
;
act61:
	ldx #dead
	jsr strout_lower
	clr bitflags+DARKFLAG
	ldaa lastloc
	staa location
	; fall through

;
;	Action 64,76: Look
;
act64:
act76:
	; look
	jmp look

0action_x:
0	fdb 0
;
;	Action 62: Move an object to a given location
;	
act62:
	jsr get_arg		; X is objloc ptr, B is location
0	stx action_x
1	pshx
	pshb
	jsr get_arg		; New location into A
	pulb
0	ldx action_x
1	pulx
	jmp move_item

;
;	Action 63: Game Over
;
act63:
	; throw an exception out of the interpreter
	ldx #playagain
	jsr strout_lower
	jsr yes_or_no
	tstb
	bne reset
	jmp start_game
reset:
	ldx $FFFE
	jmp ,x

;
;	Action 67: Set bit flag 0
;
act67:
	ldab #255
	stab bitflags
	rts

;
;	Action 68: Clear bit flag 0
;
act68:
	clr bitflags
	rts

;
;	Action 69: Refill the lamp
;
act69:
	ldaa lightfill
	staa lighttime
	clr bitflags+LIGHTOUT
	ldx #objloc+LIGHT_SOURCE
	ldab ,x
	ldaa #255
	jmp move_item

;
;	Action 70: Clear the screen
;
act70:
	; clear screen (some versions only)
	jmp wipe_lower

;
;	Action 72: Swap two objects over
;
act72:
	jsr get_arg
0	stx action_x
1	pshx		; Save objloc ptr for object 1
	pshb		; Save current location for object 1
	jsr get_arg	; Get object 2
	tba		; A is now the location of object 2
	pulb		; Recover location of object 1
	stab ,x		; Second object to first
0	ldx action_x
1	pulx		; Pointer to object 1
	staa ,x		; Swapped over
	cmpa location
	beq placerd
	cmpb location
	bne noplacerd
placerd:
	inc redraw
noplacerd:
	rts
;
;	Action 73: Set the continuation flag
;
act73:
	inc continuation
	rts

;
;	Action 74: Move object to inventory regardless of weight
;
act74:
	jsr get_arg
	ldaa #255
	jmp move_item
;
;	Action 75: Place one object with another
;
act75:
	jsr get_arg
0	stx action_x
1	pshx
	pshb
	jsr get_arg
	tba		; loc of second object is our target
	pulb
0	ldx action_x
1	pulx
	jmp move_item

load_counter:
0	ldaa counter
0	ldab counter+1
1	ldd counter
	rts
	
;
;	Action 77: Decrement counter
;
act77:
	bsr load_counter
	; Decrement if >= 0
	bita #$80
	bne nodec
0	subb #1
0	sbca #0
1	subd #1
	bra store_counter
nodec:
	rts

;
;	Action 78: Print counter value
;
act78:
	bsr load_counter
	tba
	jmp decout_lower

;
;	Action 79: Set counter
;
act79:
	jsr get_arg16
	bra store_counter
;
;	Action 80: Swap player location with saved room (YOHO etc)
;
act80:
	ldaa location
	ldab savedroom
	staa savedroom
	stab location
	inc redraw
	rts
;
;	Action 81: Swap current counter with counter n
;
act81:
	jsr get_arg
	tab
	ldx #counter_array		; find the counter pointer
0	jsr abx
0	jsr abx
0	ldaa ,x
0	ldab 1,x
1	abx
1	abx
1	ldd ,x				; load the value of the counter
	psha				; save the counters
	pshb
	bsr load_counter		; load the current counter
0	staa ,x
0	stab 1,x
1	std ,x				; save into the swapped counter
	pulb				; recover the old value
	pula
	bra store_counter		; store that in the current counter

;
;	Action 82: Add to current counter
;
act82:
	jsr get_arg16
0	addb counter+1
0	adca counter
1	addd counter
store_counter:
0	staa counter
0	stab counter+1
1	std counter
	rts

;
;	Action 83: Subtract from current counter. Negative values all turn
;	into -1.
;
act83:
	jsr get_arg16
0	staa tmp16
0	stab tmp16+1
1	std tmp16
	bsr load_counter
0	subb tmp16+1
0	sbca tmp16
1	subd tmp16
	bcc notneg
	ldaa #$ff		; ldd -1
	tab
notneg:
	bra store_counter

;
;	Action 84: Print the noun string and a newline 
;
act84:
	bsr act86
;
;	Action 86: Print a newline
;
act86:
	ldx #newline
	jmp strout_lower

;
;	Action 84: Print the noun string
;
act85:
	ldx #nounbuf
	jmp strout_lower
;
;	Action 87: Swap the current location and saveroom flag n
;	(Claymorgue). Interestingly this is broken on the genuine 6809
;	interpreter !
;
act87:
	jsr get_arg
	tab
	ldx #roomsave
0	jsr abx
1	abx
	ldaa ,x
	ldab location
	stab ,x
	staa location
	cmpa ,x
	beq noop2
	inc redraw
noop2:
	rts

;
;	Action 88: Two second delay
;
act88:
	clra
	clrb
snooze:
	addb #1
	adca #0
	bcc snooze
	rts

;
;	Action 89: Various. SAGA uses it to draw pictures, Seas of Blood
;	uses it to start combat.
;
act89:
	; Specials for SAGA etc
	jmp get_arg

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
	ldx #stored_msg
	jsr strout_lower

	ldx #objloc
	ldaa #0
score2:
	ldab treasure
	cmpb ,x
	bne notintreas
0	stx action_x
1	pshx
	jsr getotext_x		; Object texts start * for treasure
	ldab #'*'
	cmpb ,x
	bne notintreas
	inca
notintreas:
0	ldx action_x
1	pulx
	inx
	cpx #objloc_end
	bne score2
	; A is now the count to print
	psha
	jsr decout_lower
	ldx #stored_msg2
	jsr strout_lower
	pula
	cmpa treasures
	bne not_act63		; quit
	jmp act63
not_act63:
	rts

;
;	Action 66: Display the inventory
;
act66:
	ldx #carrying
	jsr strout_lower
	ldx #objloc
	clra
	clr tmp8
objl:
	ldab #255
	cmpb ,x
	bne notgot
	tst tmp8		; first item found ?
	beq inv_1
0	stx action_x
1	pshx
	ldx #dashstr
	jsr strout_lower
0	ldx action_x
1	pulx
inv_1:	inc tmp8
0	stx action_x
1	pshx
	jsr getotext_x
	jsr strout_lower
0	ldx action_x
1	pulx
notgot:
	inx
	cpx #objloc_end
	bne objl
	tst tmp8
	bne invstuff
	ldx #nothing
	jsr strout_lower
invstuff:
	ldx #dotnewline
	jmp strout_lower

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
	clr continuation	; will get set by a continuation action
next_action:
	stx linestart
	ldaa ,x
	staa condacts
	bita #$80		; AUTO flag
	beq not_cont
	bita #$20		; AUTO 100
	bne not_random
	bita #$40		; AUTO 0
	beq is_random
	tst continuation	; Skip continuations we didn't match
	beq next_line
	bra doing_cont
not_random:
	clr continuation	; AUTO 100 ends continuations
doing_cont:
	inx			; Skip header byte and go
	bra do_line
is_random:
	ldaa 1,x
	jsr random
	tstb
	beq next_line
	inx			; Skip header byte and random number
	inx
	bra do_line
;
;	If we are doing continuations and hit a non continuation line
;	then we have finished. Otherwise the verb/noun must match for us
;	to process the line but once we have one match we stop.
;
not_cont:
	inx			; Skip header byte
	tst actmatch
	bne action_done		; hit a new block - done
0	ldaa ,x
0	ldab 1,x
1	ldd ,x			; FIXME: allow R bit on verb/noun pairs
	inx			; Skip verb / noun pair
	inx
0	stx action_x
1	pshx
	psha
	pshb
;	jsr hexout_lower
	pula
	psha
;	jsr hexout_lower
;	jsr read_key
	pulb
	pula
0	ldx action_x
1	pulx
	cmpa verb
	bne next_line
	cmpb noun
	beq match_ok
	tstb
	bne next_line
match_ok:
	;
	;	Verb matches and noun matches or is not given
	;
	inc linematch
do_line:
	jsr perform_line	; run the conditions and actions
;
;	X is the head byte of the current line. Use this to find
;	the next line.
;
next_line:
	ldx linestart		; current line start in X
	ldaa ,x
	bita #$80		; 0x80 - vocab bytes omitted
	bne squashed
	inx			; Move past header
	inx			; Move past verb
squashed1:
	inx			; move on to the conditions and actions
	tab
	anda #$03		; actions in the low 2 bits
	inca			; 1-4 not 0-3
	lsrb			; 2 x conditions
	andb #$0E		; 2 * conds
0	jsr abx
1	abx			; Move over conditions
	tab
0	jsr abx
1	abx			; Move over actions
	ldaa ,x			; round we go
	cmpa #255		; 255 is end of table
	beq action_done
	jmp next_action
squashed:
	bita #$60		; 0x40/0x20 = random %age suppressed
	bne squashed1		; squashed1 skips the header
	inx			; skip the random %age as well
	bra squashed1

;
;	All completed. If this was an action then consider builtins, if not
;	we are done
;
action_done:
	ldaa verb
	cmpa #255
	bne builtins
all_finished:
	rts

builtins:
	tst linematch
	bne all_finished
	; FIXME - some games have specials for get/put all. Some games
	; have no builtins (early)
	ldaa verb
	cmpa #VERB_GET		; 10
	bne not_get
	;
	;	Automatic get handler
	;
	ldab noun
	beq ummwhat
	ldab carried
	cmpb maxcar
	blt cancarry
	ldx #toomuch
	jsr strout_lower
	bra all_done
cancarry:
	ldab location
	bsr autonoun		; Find the object of this noun if any
	cmpb #255
	bne knownobjg
bpower:
	ldx #beyondpower
	jsr strout_lower
	bra all_done
knownobjg:
	ldaa #255
domove:
	jsr move_item_b
	ldx #okmsg
	jsr strout_lower
	bra all_done
not_get:
	;
	;	Automatic drop handler
	;
	cmpa #VERB_DROP		; 18
	bne not_drop
	ldab noun
	beq ummwhat
	ldab #255
	bsr autonoun
	cmpb #255
	beq bpower
	ldaa location
	bra domove

all_done:
	inc actmatch		; auto logic "did" the action
not_drop:
	rts

ummwhat:
	ldx #whatstr
	jsr strout_lower

;
;	On entry B holds the location to scan. We check for
;	any auto entry that matches our word and is in the
;	correct location, then return that or 0xff if no match.
;
autonoun:
	ldx #automap
	stab tmp8		; location to match
	ldaa wordbuf
	beq noauto		; 0 - none
	cmpa #32
	beq noauto
autonounl:
	jsr wordeq
	beq foundnoun
nextnoun:
	ldab #WORDSIZE+1	; 5 bytes per entry
0	jsr abx
1	abx
	tst ,x			; 0 - end of list
	bne autonounl
noauto:
	ldab #$ff		; Word present but not in vocabulary
	rts
foundnoun:
0	stx action_x
1	pshx
	ldab WORDSIZE,x		; object id
	ldx #objloc
0	jsr abx
1	abx
	ldaa ,x			; location
	cmpa tmp8
	beq objmatch
0	ldx action_x
1	pulx
	bra nextnoun
objmatch:
0	ldx action_x
1	pulx
	rts

getloc_x:			; Get location ptr into X - avoid shifts
	ldab location
	ldx #locdata		; as we might overflow
0	bsr abx
0	bsr abx
0	bsr abx
0	bsr abx
0	bsr abx
0	bsr abx
0	bsr abx
0	bsr abx
1	abx
1	abx
1	abx
1	abx
1	abx
1	abx
1	abx
1	abx
	rts

0abx:	stx tmp_abx
0	psha
0	pshb
0	clra
0	addb tmp_abx+1
0	adca tmp_abx
0	stab tmp_abx+1
0	staa tmp_abx
0	pulb
0	pula
0	ldx tmp_abx
0	rts
0tmp_abx:
0	fdb 0

;
;	Look: Display the location details. This ends up in the upper
;	window, as the Scott Adams' system uses a two window output model.
;
look:
	jsr start_upper		; Clear out the old
	jsr islight		; See if it is dark
	beq cansee		; Nope
	ldx #itsdark		; "It is dark"
	jsr strout_upper
	jmp end_upper		; and done

cansee:
	bsr getloc_x		; Find the right location data
0	stx action_x
1	pshx			; Save our pointer
	ldx ,x			; Get the message ptr
	ldaa #'*'		; Is it *
	cmpa ,x
	beq notshort		; Nope.. just print it
0	stx look_x
1	pshx			; Save it
	ldx #youare		; Standard game prefix ("You are", "I am")
	jsr strout_upper
0	ldx look_x
1	pulx
	dex
notshort:
	inx			; Skip *
	jsr strout_upper	; Print the location text
	clr tmp8		; No exit seen yet
	ldx #obexit		; Exits string
	jsr strout_upper
0	ldx action_x
1	pulx			; Recover the location ptr
	inx			; Move on to exits
	inx
	clrb			; Count exits
exitl:
	tst ,x			; 0 = none
	beq notanexit
0	stx action_x
1	pshx			; Save our pointer
	tst tmp8		; First exit ?
	beq firstexit
	ldx #dashstr		; Print - or , 
	jsr strout_upper
firstexit:
	inc tmp8		; No longer first exit
	ldx #exitmsgptr		; Exit messages
0	jsr abx
0	jsr abx
1	abx			; Find the right one
1	abx
	ldx ,x
	jsr strout_upper	; Print it
0	ldx action_x
1	pulx			; Get our exits pointer back
notanexit:
	inx			; Move on
	incb			; Done ?
	cmpb #6
	blt exitl
	tst tmp8		; No exits ?
	bne wasstuff
	ldx #nonestr		; "None"
	jsr strout_upper
wasstuff:
	ldx #dotnewline		; "."
	jsr strout_upper
	clr tmp8		; No objects seen
	ldx #objloc
	ldab location
lookiteml:
	cmpb ,x			; Object here ?
	bne objnothere
0	stx action_x
1	pshx			; Print either - or also see message
	ldx #dashstr
	tst tmp8
	bne notfirsti
	ldx #canalsosee
notfirsti:
	inc tmp8		; Object seen
	jsr strout_upper
0	ldx action_x
1	pulx			; Recover our object pointer
1	pshx
	jsr getotext_x		; Print the object name
	jsr strout_upper
0	ldx action_x
1	pulx
objnothere:
	inx
	cpx #objloc_end		; Done ?
	blt lookiteml		; Keep going
	tst tmp8
	beq nothingtosee
	ldx #dotnewline		; Finish up
	jsr strout_upper
nothingtosee:
	jmp end_upper		; Draw the barrier line and donee

0look_x:
0	fdb 0

start_game:
	sei
setup_obj:
	ldx #objinit_end
	lds #objloc_end
setup_loop:
	dex
	ldaa ,x
	psha
	cpx #objinit
	bne setup_loop
	ldx #zeroblock		; Range to wipe
;
;	Clear flags and counters
;
clearl:
	clr ,x
	inx
	cpx #zeroblock_end
	bne clearl
	ldaa startloc
	staa location
	ldaa startlamp
	staa lighttime
	ldaa startcarried
	staa carried
	ldx #$4200
	stx nextchar
	ldx #justbuf
	stx justify
	lds #stacktop
	cli
	jsr wipe_screen
	inc redraw
	jmp main_loop	

nextchar:
	fdb $4200
lowtop:
	fdb $4020
nextupper:
	fdb $4000
lineptr:
	fdb linebuf
linebuf:
	zmb 30		; buffer for input
wordbuf:
	zmb 4
nounbuf:
	fdb 0
redraw:			; Top display is dirty
	fcb 0


wordflush:
;
;	Action 71: Save the game position
;
act71:
	rts

;
;	Action Table
;
actab:
	fdb act52
	fdb act53
	fdb act54
	fdb act55
	fdb act56
	fdb act57
	fdb act58
	fdb act59
	fdb act60
	fdb act61
	fdb act62
	fdb act63
	fdb act64
	fdb act65
	fdb act66
	fdb act67
	fdb act68
	fdb act69
	fdb act70
	fdb act71
	fdb act72
	fdb act73
	fdb act74
	fdb act75
	fdb act76
	fdb act77
	fdb act78
	fdb act79
	fdb act80
	fdb act81
	fdb act82
	fdb act83
	fdb act84
	fdb act85
	fdb act86
	fdb act87
	fdb act88
	fdb act89

;
;	Abbreviations: FIXME - might be nice to separate these for non
; English games ?
;
a_nort: fcc "NORTH"
a_sout: fcc "SOUTH"
a_east: fcc "EAST"
	fcb 0
a_west: fcc "WEST"
	fcb 0
a_up:   fcc "UP"
	fcb 0
a_down: fcc "DOWN"
	fcb 0
a_inve: fcc "INVEN"
;
;	Engine temporaries
;
verb:
	fcb 0
noun:
	fcb 0
tmp8:
	fcb 0
tmp8_2:
	fcb 0
tmp16:
	fdb 0
linestart:
	fdb 0
linematch:
	fcb 0
actmatch:
	fcb 0
condacts:
	fcb 0		; condact header byte for this line
continuation:
	fcb 0
argh:
	fcb 0		; argh high byte for last
argp:
	fdb 0
args:
	zmb 10			; max 5 parameters
justbuf:
	zmb 32
justify:
	fdb 0

	zmb 256			; overkill
stacktop:
	fcb 0

;
;	Between here and saveblock_end is saved
;
saveblock:

carried:
	fcb 0
lighttime:
	fcb 0
location:
	fcb 0

	fcb 0,0,0,0,0
objloc:
	zmb NUM_OBJ
objloc_end:

;
;	Between here and zeroblock_end is wiped each new game
;

zeroblock:

roomsave:
	zmb 12		; Seem to be sufficient (6 room saves)
savedroom:
	fcb 0		; single flag for this type
bitflags:
	zmb NUM_BITS
counter:
	fdb 0
counter_array
	zmb 32		; 16 counters

zeroblock_end:
saveblock_end:


debug:
	psha
	pshb
0	stx debug_x
1	pshx
	ldaa #'*'
	jsr chout_lower
0	ldx debug_x
1	pulx
	pulb
	pula
	rts

debug2:
	psha
	pshb
0	stx debug_x
1	pshx
	ldaa #'L'
	jsr chout_lower
0	ldx debug_x
1	pulx
	pulb
	pula
	rts

debug3:
	psha
	pshb
0	stx debug_x
1	pshx
	ldaa #'D'
	jsr chout_lower
0	ldx debug_x
1	pulx
	pulb
	pula
	rts

0debug_x:
0	fdb 0

0abx:	stx tmp_abx
0	psha
0	pshb
0	clra
0	addb tmp_abx+1
0	adca tmp_abx
0	stab tmp_abx+1
0	staa tmp_abx
0	pulb
0	pula
0	ldx tmp_abx
0	rts
0tmp_abx:
0	fdb 0
