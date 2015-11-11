#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#define VERB_GO		1
#define VERB_GET	10
#define VERB_DROP	18

static void strout_lower(const char *p)
{
  write(1, p, strlen(p));
}

static void strout_lower_spc(const char *p)
{
  write(1, p, strlen(p));
  write(1, " ", 1);
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

static uint8_t random(uint8_t v)
{
  v = v + v + (v >> 1);	/* scale as 0-249 */
  if (((rand() >> 3) & 0xFF) <= v)
    return 1;
  return 0;
}

static uint8_t whichword(uint8_t *p)
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
  copyword(x);
  noun = whichword(nouns);
}

static void scan_input(void)
{
  uint8_t *x = copyword(linebuf);
  verb = whichword(verbs);
  scan_noun(x);
}

void abbrev(void)
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
  
static uint8_t *conditions(uint8_t *p, uint8_t n)
{
  uint8_t i;
  
  for (i = 0; i < n; i++) {
    uint8_t opc = *p++;
    uint16_t par = *p++ || ((opc & 0xE0) >> 5);
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

static void action_delay(void)
{
  sleep(2);
}

static void action_dead(void)
{
  strout_lower(dead);
  bitflags &= ~(1 << DARKFLAG);
  location = lastloc;
  look();
}

static void action_quit(void)
{
  strout_lower(playagain);
  if (yes_or_no())
    longjmp(restart);
  exit_game(0);
}

static void action_score(void)
{
  uint8_t *p = objloc;
  uint8_t **m = objtext;
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
  uint8_t **m = objtext;
  uint8_t f = 1;

  strout_lower(carrying);
  if (carried == 0) {
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


  strout_lower(stored_msg);
  decout_lower(s);
  strout_lower(stored_msg2);
  decout_lower((s * (uint16_t)100) / t);
  strout_lower(dotnewline);
  if (s == t)
    action_quit();
}
  
static void actions(uint8_t *p)
{
  uint8_t i;

  for (i = 0; i < 4; i++) {
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
        if (carried >= maxcarr)
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
        look();
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
        move_item(LIGHT_SOURCE, 255);
        break;
      case 70:	/* Wipe lower */
        /* TODO */
        break;
      case 71:	/* Save */
        /* TODO */
      case 72:	/* Swap two objects */
        tmp = objloc[*param];
        moveitem(*param, objloc[param[1]);
        moveitem(param[1], tmp);
        param += 2;
        break;
      case 73:
        continuation = 1;
        break;
      case 74:	/* Get without weight rule */
        move_item(*param++, 255);
        break;
      case 75:	/* Put one item by another */
        move_item(*param, objloc[param[1]]);
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
        tmp = savedroom
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
  c >> = 1;
  c &= 0x0E;		/* 2 x conditions */
  linestart += c;
}

void run_line(uint8_t *ptr, uint8_t c, uint8_t a)
{
  memset(param_buf, 0, sizeof(param_buf));
  param = param_buf;
  if (c)
    ptr = conditions(ptr, c);
  if (ptr) {
    actmatch = 1;
    param = param_buf;
    actions(ptr, a);
  }
  next_line();
}

void run_table(uint8_t *tp)
{
  continuation = 0;
  while(1) {
    uint8_t hdr;
    uint8_t c, a;
    linestart = tp;
    hdr = *tp++;
    c = (hdr >> 2) & 0x07;
    a = (hdr & 3) + 1;
    
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
  uint8_t *p = automap;
  if (*wordbuf == ' ' || *wordbuf == 0)
    return 255;
  while(*p) {
    if (memcmp(p, wordbuf, WORDSIZE) == 0 && objloc[p[WORDSIZE]] == loc)
      return p[WORDSIZE];
    p += WORDSIZE + 1;
  }
  return 255;
}
  
void run_command(void)
{
  run_table(actions);
  if (actmatch)
    return;
  if (verb == VERB_GET) {		/* Get */
    if (noun == 255)
      strout_lower(whatstr);
    else if (carried >= cancarry)
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
    if (noun == 255)
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
  strout_lower(lightime == 1 ? turn : turns);
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

void main_loop(void)
{
  uint8_t first = 1;
  uint8_t *p;

  look();
  
  while (1) {
    if (!first)
      process_light();
    else
      first = 0;
    verb = 0;
    noun = 0;
    run_table(status);
    if (redraw) {
      look();    
      redraw = 0;
    }
    strout_lower(whattodo);
    do {
      strout_lower(prompt);
      line_input();
      abbrevs();
      p = stpblk(linebuf);
      if (*p == 0)
        continue;
      scan_noun();
      if (noun && noun <= 6) {
        verb = VERB_GO;
        break;
      }
      scan_input();
      if (verb == 255) {
        strout_lower(dontknow);
        continue;
      }
    } while (verb == 0);
    
    if (verb == VERB_GO) {
      if (!noun) {
        strout_lower(givedirn);
        continue;
      }
      if (noun <= 6) {
        uint8_t light = islight();
        uint8_t dir;

        if (!light)
          strout_lower(dardanger);
        dir = locdata[location].exit[noun - 1];
        if (!dir) {
          if (!light) {
            strout_lower(brokeneck);
            action_delay();
            action_die();
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
