#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <setjmp.h>

#include <stdio.h>

static jmp_buf restart;

static char linebuf[81];
static char *nounbuf;
static char wordbuf[WORDSIZE + 1];

static uint8_t verb;
static uint8_t noun;
static const uint8_t *linestart;
static uint8_t linematch;
static uint8_t actmatch;
static uint8_t continuation;
static uint16_t *param;
static uint16_t param_buf[5];
static uint8_t carried;
static uint8_t lighttime;
static uint8_t location;
static uint8_t objloc[NUM_OBJ];
static uint8_t roomsave[6];
static uint8_t savedroom;
static uint32_t bitflags;
static int16_t counter;
static int16_t counter_array[16];
static uint8_t redraw;

#define VERB_GO		1
#define VERB_GET	10
#define VERB_DROP	18

#define LIGHTOUT	16
#define DARKFLAG	15
#define LIGHT_SOURCE	9

/* Hackish temporary I/O code */
static void strout_lower(const char *p)
{
  write(1, p, strlen(p));
}

static void strout_lower_spc(const char *p)
{
  write(1, p, strlen(p));
  write(1, " ", 1);
}

static void decout_lower(uint16_t v)
{
  char buf[9];
  snprintf(buf, 8, "%d", v);
  strout_lower(buf);
}

static void strout_upper(const char *p)
{
  write(1, p, strlen(p));
}

static void exit_game(uint8_t code)
{
  exit(code);
}

static void error(const char *p)
{
  write(2, p, strlen(p));
  exit_game(1);
}

static char readchar(void)
{
  char c;
  if (read(0, &c, 1) < 1)
    return -1;
  return c;
}

static uint8_t yes_or_no(void)
{
  char c;
  do {
    c = readchar();
    if (c == 'Y'  || c == 'y' || c == 'J' || c == 'j')
      return 1;
  } while(c != -1 && c != 'N' && c != 'n');
  return 0;
}

static void wait_cr(void)
{
  while(readchar() != '\n');
}

static void line_input(void)
{
  int l = read(0, linebuf, sizeof(linebuf));
  if (l < 0)
    error("read");
  linebuf[l] = 0;
  if (l && linebuf[l-1] == '\n')
    linebuf[l-1] = 0;
}

static uint8_t random_chance(uint8_t v)
{
  v = v + v + (v >> 1);	/* scale as 0-249 */
  if (((rand() >> 3) & 0xFF) <= v)
    return 1;
  return 0;
}

static char *skip_spaces(char *p)
{
  while(*p && isspace(*p))
    p++;
  return p;
}

static char *copyword(char *p)
{
  char *t = wordbuf;
  p = skip_spaces(p);
  memset(wordbuf, ' ', WORDSIZE+1);
  while (*p && !isspace(*p) && t < wordbuf + WORDSIZE)
    *t++ = *p++;
  while(*p && !isspace(*p))
    p++;
  return p;
}

static int wordeq(const char *a, const char *b, uint8_t l)
{
  while(l--)
    if ((*a++ & 0x7F) != toupper(*b++))
      return 0;
  return 1;
}

static uint8_t whichword(const uint8_t *p)
{
  uint8_t code = 0;
  uint8_t i = 0;

  if (*wordbuf == 0 || *wordbuf == ' ')
    return 0;		/* No word */
  i--;
  
  do {
    i++;
    if (!(*p & 0x80))
      code = i;
    if (wordeq(p, wordbuf, WORDSIZE))
      return code;
    p += WORDSIZE;
  } while(*p != 0);
  return 255;
}

static void scan_noun(uint8_t *x)
{
  x = skip_spaces(x);
  nounbuf = x;
  copyword(x);
  noun = whichword(nouns);
}

static void scan_input(void)
{
  uint8_t *x = copyword(linebuf);
  verb = whichword(verbs);
  scan_noun(x);
}

void abbrevs(void)
{
  uint8_t *x = skip_spaces(linebuf);
  uint8_t *p = NULL;
  if (x[1] != 0 && x[1] != ' ')
    return;
  switch(toupper(*x)) {
    case 'N': 
      p = "NORTH";
      break;
    case 'E':
      p = "EAST";
      break;
    case 'S':
      p = "SOUTH";
      break;
    case 'W':
      p = "WEST";
      break;
    case 'U':
      p = "UP";
      break;
    case 'D':
      p = "DOWN";
      break;
    case 'I':
      p = "INVEN";
      break;
  }
  if (p)
    strcpy(linebuf, p);
}
  
static const uint8_t *run_conditions(const uint8_t *p, uint8_t n)
{
  uint8_t i;
  
  for (i = 0; i < n; i++) {
    uint8_t opc = *p++;
    uint16_t par = *p++ | ((opc & 0xE0) >> 5);
    opc &= 0x1F;
    uint8_t op = objloc[par];

    switch(opc) {
      case 0:
        *param++ = par;
        break;
      case 1:
        if (op != 255)
          return NULL;
        break;
      case 2:
        if (op != location)
          return NULL;
        break;
      case 3:
        if (op != 255 && op != location)
          return NULL;
        break;
      case 4:
        if (location != par)
          return NULL;
        break;
      case 5:
        if (op == location)
          return NULL;
        break;
      case 6:
        if (op == 255)
          return NULL;
        break;
      case 7:
        if (location == par)
          return NULL;
        break;
      case 8:
        if (!(bitflags & (1 << par)))
          return NULL;
        break;
      case 9:
        if (bitflags & (1 << par))
          return NULL;
        break;
      case 10:
        if (!carried)
          return NULL;
        break;
      case 11:
        if (carried)
          return NULL;
        break;
      case 12:
        if (op == 255 || op == location)
          return NULL;
        break;
      case 13:
        if (op == 0)
          return NULL;
        break;
      case 14:
        if (op != 0)
          return NULL;
        break;
      case 15:
        if (counter > par)
          return NULL;
        break;
      case 16:
        if (counter < par)
          return NULL;
        break;
      case 17:
        if (op != objinit[par]) 
          return NULL;
        break;
      case 18:
        if (op == objinit[par])
          return NULL;
        break;
      case 19:
        if (counter != par)
          return NULL;
        break;
      default:
        error("BADCOND");
    }
  }
  return p;
}

uint8_t islight(void)
{
  uint8_t l = objloc[LIGHT_SOURCE];
  if (!(bitflags & (1 << DARKFLAG)))
    return 1;
  if (l == 255 || l == location)
    return 1;
  return 0;
}

static void action_look(void)
{
  const uint8_t *e;
  const char *p;
  uint8_t c;
  uint8_t f = 1;
  const uint8_t **op = objtext;

  strout_upper("\n\n\n\n");

  if (!islight()) {
    strout_upper(itsdark);
    return;
  }
  p = locdata[location].text;
  e = locdata[location].exit;
  if (*p == '*')
    p++;
  else
    strout_upper(youare);
  strout_upper(p);
  strout_upper(obexit);

  for (c = 0; c < 6; c++) {
    if (*e++) {
      if (f)
        f = 0;
      else
        strout_upper(dashstr);
      strout_upper(exitmsgptr[c]);
    }
  }
  if (f)
    strout_upper(nonestr);
  strout_upper(dotnewline);
  f = 1;
  e = objloc;
  while(e < objloc + NUM_OBJ) {
    if (*e++ == location) {
      if (f) {
        strout_upper(canalsosee);
        f = 0;
      } else
        strout_upper(dashstr);
      strout_upper(*op);
    }
    op++;
  }
  strout_upper("\n--------------------------------------------------\n");
  redraw = 0;
}

static void action_delay(void)
{
  sleep(2);
}

static void action_dead(void)
{
  strout_lower(dead);
  bitflags &= ~(1 << DARKFLAG);
  location = lastloc;
  action_look();
}

static void action_quit(void)
{
  strout_lower(playagain);
  if (yes_or_no())
    longjmp(restart, 0);
  exit_game(0);
}

static void action_score(void)
{
  uint8_t *p = objloc;
  const uint8_t **m = objtext;
  uint8_t t = 0, s = 0;

  while(p < objloc + NUM_OBJ) {
    if (*m[0] == '*') {
      t++;
      if (*p == treasure)
        s++;
    }
    m++;
    p++;
  }

  strout_lower(stored_msg);
  decout_lower(s);
  strout_lower(stored_msg2);
  decout_lower((s * (uint16_t)100) / t);
  strout_lower(dotnewline);
  if (s == t)
    action_quit();
}

static void action_inventory(void)
{
  uint8_t *p = objloc;
  const uint8_t **m = objtext;
  uint8_t f = 1;

  strout_lower(carrying);
  if (carried == 0)
    strout_lower(nothing);
  else {  
    while(p < objloc + NUM_OBJ) {
      if (*p == 255) {
        if (!f)
          strout_lower(dashstr);
        else
          f = 0;
        strout_lower(*m);
      }
      m++;
      p++;
    }
  }
  strout_lower(dotnewline);
}

static void moveitem(uint8_t i, uint8_t l)
{
  uint8_t *p = objloc + i;
  if (*p == location || l == location)
    redraw = 1;
  *p = l;
}

static void run_actions(const uint8_t *p, uint8_t n)
{
  uint8_t i;

  for (i = 0; i < n; i++) {
    uint8_t a = *p++;
    uint8_t tmp;
    uint16_t tmp16;

    if (a < 50) {
      strout_lower_spc(msgptr[a]);
      continue;
    }
    if (a > 102 ) {
      strout_lower_spc(msgptr[a - 50]);
      continue;
    }
    switch(a) {
      case 51:	/* nop - check */
        break;
      case 52:	/* Get */
        if (carried >= maxcar)
          strout_lower(toomuch);
        else
          moveitem(*param++, 255);
        break;
      case 53: /* Drop */
        moveitem(*param++, location);
        break;
      case 54: /* Go */
        location = *param++;
        redraw = 1;
        break;
      case 55: /* Destroy */
      case 59: /* ?? */
        moveitem(*param++, 0);
        break;
      case 56:	/* Set dark flag */
        bitflags |= (1 << DARKFLAG);
        break;
      case 57:	/* Clear dark flag */
        bitflags &= ~(1 << DARKFLAG);
        break;
      case 58:	/* Set bit */
        bitflags |= (1 << *param++);
        break;
      /* 59 see 55 */
      case 60:	/* Clear bit */
        bitflags &= ~(1 << *param++);
        break;
      case 61:	/* Dead */
        action_dead();
        break;
      case 64:	/* Look */
      case 76:	/* Also Look ?? */
        action_look();
        break;
      case 62:	/* Place obj, loc */
        tmp = *param++;
        moveitem(tmp, *param++);
        break;
      case 63:	/* Game over */
        action_quit();
      case 65:	/* Score */
        action_score();
        break;
      case 66:	/* Inventory */
        action_inventory();
      case 67:	/* Set bit 0 */
        bitflags |= (1 << 0);
        break;
      case 68:	/* Clear bit 0 */
        bitflags &= ~(1 << 0);
        break;
      case 69:	/* Refill lamp */
        lighttime = lightfill;
        bitflags &= ~(1 << LIGHTOUT);
        moveitem(LIGHT_SOURCE, 255);
        break;
      case 70:	/* Wipe lower */
        /* TODO */
        break;
      case 71:	/* Save */
        /* TODO */
      case 72:	/* Swap two objects */
        tmp = objloc[*param];
        moveitem(*param, objloc[param[1]]);
        moveitem(param[1], tmp);
        param += 2;
        break;
      case 73:
        continuation = 1;
        break;
      case 74:	/* Get without weight rule */
        moveitem(*param++, 255);
        break;
      case 75:	/* Put one item by another */
        moveitem(*param, objloc[param[1]]);
        param += 2;
        break;
      case 77:	/* Decrement counter */
        if (counter >= 0)
          counter--;
        break;
      case 78:	/* Display counter */
        decout_lower(counter);
        break;
      case 79:	/* Set counter */
        counter = *param++;
        break;
      case 80:	/* Swap player and saved room */
        tmp = savedroom;
        savedroom = location;
        location = tmp;
        redraw = 1;
        break;
      case 81:	/* Swap counter and counter n */
        tmp16 = counter;
        counter = counter_array[*param];
        counter_array[*param++] = tmp16;
        break;
      case 82:	/* Add to counter */
        counter += *param++;
        break;
      case 83:	/* Subtract from counter */
        counter -= *param++;
        if (counter < 0)
          counter = -1;
        break;
      case 84:	/* Print noun, newline */
        strout_lower(nounbuf);
        /* Fall through */
      case 86:	/* Print newline */
        strout_lower(newline);
        break;
      case 85:	/* Print noun */ 
        strout_lower(nounbuf);
        break;
      case 87: /* Swap player and saveroom array entry */
        tmp16 = *param++;
        tmp = roomsave[tmp16];
        roomsave[tmp16] = location;
        if (tmp != location) {
          location = tmp;
          redraw = 1;
        }
        break;
      case 88:
        action_delay();
        break;
      case 89:
        *param++;		/* SAGA etc specials */
        break;
      default:
        error("BADACT");
    }
  }
}

void next_line(void)
{
  uint8_t c = *linestart++;
  if (!(c & 0x80))
    linestart += 2;	/* Skip verb/noun */
  else if (!(c & 0x60))
    linestart++;	/* Skip random value */
  linestart += (c & 3) + 1;	/* Actions 1 - 4 */
  c >>= 1;
  c &= 0x0E;		/* 2 x conditions */
  linestart += c;
}

void run_line(const uint8_t *ptr, uint8_t c, uint8_t a)
{
  memset(param_buf, 0, sizeof(param_buf));
  param = param_buf;
  if (c)
    ptr = run_conditions(ptr, c);
  if (ptr) {
    actmatch = 1;
    param = param_buf;
    run_actions(ptr, a);
  }
  next_line();
}

void run_table(const uint8_t *tp)
{
  continuation = 0;
  linestart = tp;
  while(1) {
    uint8_t hdr;
    uint8_t c, a;
    tp = linestart;
    hdr = *tp++;
    c = (hdr >> 2) & 0x07;
    a = (hdr & 3) + 1;
    
/*    printf("H%02X c = %d a = %d\n", hdr, c, a); */
    if (hdr == 255)
      return;		/* End of table */
    if (hdr & 0x80) {
      if (hdr & 0x40) {	/* Auto 0 */
        if (continuation)
          run_line(tp, c, a);
        else
          next_line();
        continue;
      }
      if (!(hdr & 0x20)) {	/* Auto number */
        if (!random_chance(*tp++))
          next_line();
        continue;
      }
      run_line(tp, c, a);
    } else {
      if (actmatch)
        return;
/*      printf("VN %d %d\n", *tp, tp[1]); */
      linematch = 1;
      if (*tp++ == verb && (*tp == noun || *tp == 0))
        run_line(tp+1, c, a);
      else
        next_line();
    }
  }
}

uint8_t autonoun(uint8_t loc)
{
  const uint8_t *p = automap;
  if (*wordbuf == ' ' || *wordbuf == 0)
    return 255;
  while(*p) {
    if (strncasecmp(p, wordbuf, WORDSIZE) == 0 && objloc[p[WORDSIZE]] == loc)
      return p[WORDSIZE];
    p += WORDSIZE + 1;
  }
  return 255;
}
  
void run_command(void)
{
  uint8_t tmp;
  run_table(actions);
  if (actmatch)
    return;
  if (verb == VERB_GET) {		/* Get */
    if (noun == 0)
      strout_lower(whatstr);
    else if (carried >= maxcar)
      strout_lower(toomuch);
    else {
      tmp = autonoun(location);
      if (tmp == 255)
        strout_lower(beyondpower);
      else
        moveitem(tmp, 255);
    }
    actmatch = 1;
    return;
  }
  if (verb == VERB_DROP) {		/* Drop */
    if (noun == 0)
      strout_lower(whatstr);
    else {
      tmp = autonoun(255);
      if (tmp == 255)
        strout_lower(beyondpower);
      else
        moveitem(tmp, location);
    }
    actmatch = 1;
    return;
  }
}

void process_light(void)
{
  uint8_t l;
  if ((l = objloc[LIGHT_SOURCE]) == 0)
    return;
  if (lighttime == 255)
    return;
  if (!--lighttime) {
    bitflags &= ~(1 << LIGHTOUT);	/* Check clear ! */
    if (l == 255 || l == location) {
      strout_lower(lightout);
      redraw = 1;
      return;
    }
  }
  if (lighttime > 25)
    return;
  strout_lower(lightoutin);
  decout_lower(lighttime);
  strout_lower(lighttime == 1 ? turn : turns);
}

void main_loop(void)
{
  uint8_t first = 1;
  uint8_t *p;

  action_look();
  
  while (1) {
    if (!first)
      process_light();
    else
      first = 0;
    verb = 0;
    noun = 0;
    run_table(status);

    if (redraw)
      action_look();

    strout_lower(whattodo);
    do {
      do {
        strout_lower(prompt);
        line_input();
        abbrevs();
        p = skip_spaces(linebuf);
      }
      while(*p == 0);

      scan_noun(p);
      if (noun && noun <= 6) {
        verb = VERB_GO;
        break;
      }
      scan_input();
      if (verb == 255)
        strout_lower(dontknow);
    } while (verb == 255);
    
    if (verb == VERB_GO) {
      if (!noun) {
        strout_lower(givedirn);
        continue;
      }
      if (noun <= 6) {
        uint8_t light = islight();
        uint8_t dir;

        if (!light)
          strout_lower(darkdanger);
        dir = locdata[location].exit[noun - 1];
        if (!dir) {
          if (!light) {
            strout_lower(brokeneck);
            action_delay();
            action_dead();
            continue;
          }
          strout_lower(cantgo);
          continue;
        }
        location = dir;
        redraw = 1;
        continue;
      }
    }
    linematch = 0;
    actmatch = 0;
    run_command();
    if (actmatch)
      continue;
    if (linematch) {
      strout_lower(notyet);
      continue;
    }
    strout_lower(dontunderstand);
  }
}

void start_game(void)
{
  memcpy(objloc, objinit, sizeof(objloc));
  bitflags = 0;
  counter = 0;
  memset(counter_array, 0, sizeof(counter_array));
  savedroom = 0;
  memset(roomsave, 0, sizeof(roomsave));
  location = startloc;
  lighttime = startlamp;
  carried = startcarried;
}

int main(int argc, char *argv[])
{
  setjmp(restart);
  start_game();
  main_loop();
}
