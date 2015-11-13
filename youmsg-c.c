#include <stdint.h>

struct location {
  const uint8_t *text;
  uint8_t exit[6];
};

const char toomuch[] = { "You are carrying too much. " };
const char dead[] = { "You are dead.\n" };
const char stored_msg[] = { "You have stored " };
const char stored_msg2[] = { "treasures. On a scale of 0 to 100, that rates " };
const char dotnewline[] = { ".\n" };
const char newline[] = { "\n" };
const char carrying[] = { "You are carrying:\n" };
const char dashstr[] = { " - " };
const char nothing[] = { "nothing" };
const char lightout[] = { "Your light has run out." };
const char lightoutin[] = { "Your light runs out in " };
const char turns[] = { "turns" };
const char turn[] = { "turn" };
const char whattodo[] = { "\nTell me what to do ? " };
const char prompt[] = { "\n> " };
const char dontknow[] = { "You use word(s) I don't know! " };
const char givedirn[] = { "Give me a direction too. " };
const char darkdanger[] = { "Dangerous to move in the dark! " };
const char brokeneck[] = { "You fell down and broke your neck. " };
const char cantgo[] = { "You can't go in that direction. " };
const char dontunderstand[] = { "I don't understand your command. " };
const char notyet[] = { "You can't do that yet. " };
const char beyondpower[] = { "It is beyond your power to do that. " };
const char okmsg[] = { "O.K. " };
const char whatstr[] = { "What ? " };
const char itsdark[] = { "You can't see. It is too dark!" };
const char youare[] = { "You are in a " };
const char nonestr[] = { "none" };
const char obexit[] = { "\nObvious exits: " };
const char canalsosee[] = { "You can also see: " };
const char playagain[] = { "Do you want to play again Y/N: " };
const char invcond[] = { "INVCOND" };
const char *exitmsgptr[] = { "North", "South", "East", "West", "Up", "Down" };

