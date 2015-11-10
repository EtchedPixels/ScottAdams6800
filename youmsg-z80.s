;
;	System Messages
;
toomuch:
	.ascii "YOU ARE CARRYING TOO MUCH. "
	.db 0
dead:
	.ascii "YOU ARE DEAD."
	.db 10,0
stored_msg:
	.ascii "YOU HAVE STORED "
	.db 0
stored_msg2:
	.ascii "TREASURES. ON A SCALE OF 0 TO 100, THAT RATES "
dotnewline:
	.ascii "."
newline:
	.db 10,0
carrying:
	.ascii "YOU ARE CARRYING:"
	.db 10,0
dashstr:
	.ascii " - "
	.db 0
nothing:
	.ascii "NOTHING"
	.db 0
lightout:
	.ascii "YOUR LIGHT HAS RUN OUT. "
	.db 0
lightoutin:
	.ascii "YOUR LIGHT RUNS OUT IN "
	.db 0
turns:
	.ascii "TURNS"
	.db 0
turn:
	.ascii "TURN"
	.db 0
whattodo:
	.db 10
	.ascii "TELL ME WHAT TO DO ? "
	.db 10
	.db 0
prompt:
	.ascii ">  "
	.db 0
dontknow:
	.ascii "YOU USE WORD(S) I DON'T KNOW! "
	.db 0
givedirn:
	.ascii "GIVE ME A DIRECTION TOO. "
	.db 0
darkdanger:
	.ascii "DANGEROUS TO MOVE IN THE DARK! "
	.db 0
brokeneck:
	.ascii "YOU FELL DOWN AND BROKE YOUR NECK. "
	.db 0
cantgo:
	.ascii "YOU CAN'T GO IN THAT DIRECTION. "
	.db 0
dontunderstand:
	.ascii  "I DON'T UNDERSTAND YOUR COMMAND. "
	.db 0
notyet:
	.ascii "YOU CAN'T DO THAT YET. "
	.db 0
beyondpower:
	.ascii "IT IS BEYOND YOUR POWER TO DO THAT. "
	.db 0
okmsg:
	.ascii "O.K. "
	.db 0
whatstr:
	.ascii "WHAT ? "
	.db 0
itsdark:
	.ascii "YOU CAN'T SEE. IT IS TOO DARK!"
	.db 10
	.db 0
youare:
	.ascii "YOU ARE IN A "
	.db 0
nonestr:
	.ascii "NONE"
	.db 0
obexit:
	.db 10
	.ascii "OBVIOUS EXITS: "
	.db 0
canalsosee:
	.ascii "YOU CAN ALSO SEE: "
	.db 0
playagain:
	.ascii "DO YOU WANT TO PLAY AGAIN Y/N"
	.db 0
invcond:
	.ascii "INVCOND"
	.db 0
exit_n:
	.ascii "NORTH"
	.db 0
exit_e:
	.ascii "EAST"
	.db 0
exit_s:
	.ascii "SOUTH"
	.db 0
exit_w:
	.ascii "WEST"
	.db 0
exit_u:
	.ascii "UP"
	.db 0
exit_d:
	.ascii "DOWN"
	.db 0

exitmsgptr:
	.dw exit_n
	.dw exit_s
	.dw exit_e
	.dw exit_w
	.dw exit_u
	.dw exit_d

