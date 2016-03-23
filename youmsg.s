;
;	System Messages
;
toomuch:
	fcc "YOU ARE CARRYING TOO MUCH. "
	fcb 0
dead:
	fcc "YOU ARE DEAD."
	fcb 10,0
stored_msg:
	fcc "YOU HAVE STORED "
	fcb 0
stored_msg2:
	fcc " TREASURES. ON A SCALE OF 0 TO 100, THAT RATES "
	fcb 0
dotnewline:
	fcc "."
newline:
	fcb 10,0
carrying:
	fcc "YOU ARE CARRYING:"
	fcb 10,0
dashstr:
	fcc " - "
	fcb 0
nothing:
	fcc "NOTHING"
	fcb 0
lightout:
	fcc "YOUR LIGHT HAS RUN OUT. "
	fcb 0
lightoutin:
	fcc "YOUR LIGHT RUNS OUT IN "
	fcb 0
turns:
	fcc "TURNS"
	fcb 0
turn:
	fcc "TURN"
	fcb 0
whattodo:
	fcb 10
	fcc "TELL ME WHAT TO DO ? "
	fcb 10
	fcb 0
prompt:
	fcc ">  "
	fcb 0
dontknow:
	fcc "YOU USE WORD(S) I DON'T KNOW! "
	fcb 0
givedirn:
	fcc "GIVE ME A DIRECTION TOO. "
	fcb 0
darkdanger:
	fcc "DANGEROUS TO MOVE IN THE DARK! "
	fcb 0
brokeneck:
	fcc "YOU FELL DOWN AND BROKE YOUR NECK. "
	fcb 0
cantgo:
	fcc "YOU CAN'T GO IN THAT DIRECTION. "
	fcb 0
dontunderstand:
	fcc  "I DON'T UNDERSTAND YOUR COMMAND. "
	fcb 0
notyet:
	fcc "YOU CAN'T DO THAT YET. "
	fcb 0
beyondpower:
	fcc "IT IS BEYOND YOUR POWER TO DO THAT. "
	fcb 0
okmsg:
	fcc "O.K. "
	fcb 0
whatstr:
	fcc "WHAT ? "
	fcb 0
itsdark:
	fcc "YOU CAN'T SEE. IT IS TOO DARK!"
	fcb 10
	fcb 0
youare:
	fcc "YOU ARE IN A "
	fcb 0
nonestr:
	fcc "NONE"
	fcb 0
obexit:
	fcb 10
	fcc "OBVIOUS EXITS: "
	fcb 0
canalsosee:
	fcc "YOU CAN ALSO SEE: "
	fcb 0
playagain:
	fcc "DO YOU WANT TO PLAY AGAIN Y/N"
	fcb 0
invcond:
	fcc "INVCOND"
	fcb 0
exit_n:
	fcc "NORTH"
	fcb 0
exit_e:
	fcc "EAST"
	fcb 0
exit_s:
	fcc "SOUTH"
	fcb 0
exit_w:
	fcc "WEST"
	fcb 0
exit_u:
	fcc "UP"
	fcb 0
exit_d:
	fcc "DOWN"
	fcb 0

exitmsgptr:
	fdb exit_n
	fdb exit_s
	fdb exit_e
	fdb exit_w
	fdb exit_u
	fdb exit_d

