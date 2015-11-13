#include <stdint.h>

struct location {
  const uint8_t *text;
  uint8_t exit[6];
};

const uint8_t toomuch[] = { "You are carrying too much. " };
const uint8_t dead[] = { "You are dead.\n" };
const uint8_t stored_msg[] = { "You have stored " };
const uint8_t stored_msg2[] = { "treasures. On a scale of 0 to 100, that rates " };
const uint8_t dotnewline[] = { ".\n" };
const uint8_t newline[] = { "\n" };
const uint8_t carrying[] = { "You are carrying:\n" };
const uint8_t dashstr[] = { " - " };
const uint8_t nothing[] = { "nothing" };
const uint8_t lightout[] = { "Your light has run out." };
const uint8_t lightoutin[] = { "Your light runs out in " };
const uint8_t turns[] = { "turns" };
const uint8_t turn[] = { "turn" };
const uint8_t whattodo[] = { "\nTell me what to do ? " };
const uint8_t prompt[] = { "\n> " };
const uint8_t dontknow[] = { "You use word(s) I don't know! " };
const uint8_t givedirn[] = { "Give me a direction too. " };
const uint8_t darkdanger[] = { "Dangerous to move in the dark! " };
const uint8_t brokeneck[] = { "You fell down and broke your neck. " };
const uint8_t cantgo[] = { "You can't go in that direction. " };
const uint8_t dontunderstand[] = { "I don't understand your command. " };
const uint8_t notyet[] = { "You can't do that yet. " };
const uint8_t beyondpower[] = { "It is beyond your power to do that. " };
const uint8_t okmsg[] = { "O.K. " };
const uint8_t whatstr[] = { "What ? " };
const uint8_t itsdark[] = { "You can't see. It is too dark!" };
const uint8_t youare[] = { "You are in a " };
const uint8_t nonestr[] = { "none" };
const uint8_t obexit[] = { "\nObvious exits: " };
const uint8_t canalsosee[] = { "You can also see: " };
const uint8_t playagain[] = { "Do you want to play again Y/N: " };
const uint8_t invcond[] = { "INVCOND" };
const uint8_t *exitmsgptr[] = {
  (uint8_t *)"North",
  (uint8_t *)"South",
  (uint8_t *)"East",
  (uint8_t *)"West",
  (uint8_t *)"Up",
  (uint8_t *)"Down"
};

