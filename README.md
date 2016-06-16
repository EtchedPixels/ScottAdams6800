# ScottAdamsMC6800

This started out as a 680x engine for playing Scott Adams and related games
on the Tandy MC-10 and eventually hopefully FLEX and other 6800 systems. It
now knows how to generate compact linked together game and data for both
6800/1 on the MC10, and for low memory C targets such as FUZIX. The C target
will probably port easily to PIC and other microcontrollers. Very little
writable memory is needed.


# Current Status

6800/6801: Complete as far as I am aware

C: Plays the tested games. Choice of ncurses or plain text display. Needs a
termcap based version adding to keep size down on tiny boxes.

Z80: Most of the code generation and engine put together for a ZX81 target
but nothing yet assembled and tested.

# Things To Do

Complete the Z80 support

Add text compression to get the biggest few games to fit the MC-10 and ZX81
