#include <stdio.h>	/* Not really used but needed for perror */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <setjmp.h>
#include <termios.h>

#ifdef __linux__
#include <stdio.h>
#endif

static jmp_buf restart;

struct savearea {
  uint16_t magic;
  uint8_t carried;
  uint8_t lighttime;
  uint8_t location;
  uint8_t objloc[NUM_OBJ];
  uint8_t roomsave[6];
  uint8_t savedroom;
  uint32_t bitflags;
  int16_t counter;
  int16_t counter_array[16];
};

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
static uint8_t rows, cols;
static uint8_t redraw;

static struct savearea game;

static void error(const char *p);

#define VERB_GO		1
#define VERB_GET	10
#define VERB_DROP	18

#define LIGHTOUT	16
#define DARKFLAG	15
#define LIGHT_SOURCE	9

/* Define this because 1 << n might be 16bit */
#define ONEBIT		((uint32_t)1)

#define REDRAW		1
#define REDRAW_MAYBE	2

#ifdef CONFIG_IO_CURSES

#include <curses.h>

#define REDRAW_MASK	(REDRAW|REDRAW_MAYBE)

static char wbuf[81];
static int wbp = 0;
static int xpos = 0, ypos = 0;
static int bottom;
static WINDOW *topwin, *botwin, *curwin;

static void flush_word(void)
{
  wbuf[wbp] = 0;
  waddstr(curwin, wbuf);
  xpos += wbp;
  wbp = 0;
}

static void new_line(void)
{
  xpos = 0;
  if (curwin == topwin)
    ypos++;
  else {
    scroll(curwin);
    ypos = bottom;
  }
  wmove(curwin, ypos, xpos);
}

static void char_out(char c)
{
  if (c == '\n') {
    flush_word();
    new_line();
    return;
  }
  if (c != ' ') {
    if (wbp < 80)
      wbuf[wbp++] = c;
    return;
  }
  if (xpos + wbp >= cols)
    new_line();
  flush_word();
  waddch(curwin, ' ');
  xpos++;
}

static void strout_lower(const uint8_t *p)
{
  while(*p)
    char_out(*p++);
}

static void strout_lower_spc(const uint8_t *p)
{
  strout_lower(p);
  char_out(' ');
}

static void decout_lower(uint16_t v)
{
#ifdef __linux__
  char buf[9];
  snprintf(buf, 8, "%d", v);	/* FIXME: avoid expensive snprintf */
  strout_lower((uint8_t *)buf);
#else
  strout_lower((uint8_t *)_itoa(v));
#endif
}

static void strout_upper(const uint8_t *p)
{
  strout_lower(p);
}

static char readchar(void)
{
  wrefresh(botwin);
  return wgetch(botwin);
}

static void line_input(int m)
{
  int c;
  char *p = linebuf;

  do {
    wmove(botwin, ypos, xpos);
    wrefresh(botwin);
    c = wgetch(botwin);
    if (c == 8 || c == 127) {
      if (p > linebuf) {
        xpos--;
        mvwaddch(botwin, ypos, xpos, ' ');
        p--;
      }
      continue;
    }
    if (c > 31 && c < 127) {
      if (p < linebuf + 80 && xpos < cols - 1) {
        *p++ = c;
        mvwaddch(botwin, ypos, xpos, c);
        xpos++;
      }
      continue;
    }
  }
  while (c != 13 && c != 10);
  *p = 0;
  new_line();
}

static int saved_x;

static void begin_upper(void)
{
  saved_x = xpos;
  curwin = topwin;
  werase(topwin);
  ypos = 0;
  xpos = 0;
}

static void end_upper(void)
{
  flush_word();
  curwin = botwin;
  xpos = saved_x;
  ypos = bottom;
  wrefresh(topwin);
}

static void display_init(void)
{
  int trow;

  initscr();
  noecho();
  cbreak();
  nonl();

  getmaxyx(stdscr, rows, cols);

  if (rows < 16)
    error("display too small");

  trow = 10;
  if (rows / 2 < 10)
    trow = rows / 2;
  bottom = rows - trow;

  topwin = newwin(trow, cols, 0, 0);
  botwin = newwin(bottom--, cols, trow, 0);
  if (!topwin || !botwin)
    error("curses");
  scrollok(botwin, TRUE);
  curwin = botwin;
  new_line();
}

static void display_exit(void)
{
  endwin();
}

#elif defined(CONFIG_IO_CUSS)

/* ---- */

#include <termcap.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>

/* A mini look alike to David Given's libcuss. If useful will probably
   become a library. For now pasted around to experiment */

uint_fast8_t screenx, screeny, screen_height, screen_width;

static char *t_go, *t_clreol, *t_clreos;
static uint8_t conbuf[64];
static uint8_t *conp = conbuf;

extern void con_puts(const char *s);

/* Queue a character to the output buffer */
static int conq(int c)
{
	if (conp == conbuf + sizeof(conbuf)) {
		write(1, conbuf, sizeof(conbuf));
		conp = conbuf;
	}
	*conp++ = (uint8_t) c;
	return 0;
}

/* Make sure the output buffer is written */
void con_flush(void)
{
	write(1, conbuf, conp - conbuf);
	conp = conbuf;
}

static const char hex[] = "0123456789ABCDEF";

/* Put a character to the screen. We handle unprintables and tabs */
void con_putc(uint8_t c)
{
	if (c == '\t') {
		uint8_t n = 8 - (screenx & 7);
		while (n--)
			con_putc(' ');
		return;
	}
	if (c > 127) {
		con_puts("\\x");
		con_putc(hex[c >> 4]);
		con_putc(hex[c & 0x0F]);
		return;
	} else if (c == 127) {
		con_puts("^?");
		return;
	}
	if (c < 32) {
		con_putc('^');
		c += '@';
	}
	conq(c);
	screenx++;
adjust:
	if (screenx == screen_width) {
		screenx = 0;
		screeny++;
	}
}

/* Write a termcap string out */
static void con_twrite(char *p, int n)
{
#if !defined(__linux__)
	tputs(p, n, conq);
#else
	while (*p)
		conq(*p++);
#endif
}

/* Write a string of symbols including quoting */
void con_puts(const char *s)
{
	uint8_t c;
	while (c = (uint8_t) *s++)
		con_putc(c);
}

/* Add a newline */
void con_newline(void)
{
	if (screeny >= screen_height)
		return;
	conq('\n');
	screenx = 0;
	screeny++;
}

/* We need to optimize this but firstly we need to fix our
   tracking logic as we use con_goto internally but don't track
   that verus the true user values */
void con_force_goto(uint_fast8_t y, uint_fast8_t x)
{
	con_twrite(tgoto(t_go, x, y), 2);
	screenx = x;
	screeny = y;
}

void con_goto(uint_fast8_t y, uint_fast8_t x)
{
#if 0
	if (screenx == x && screeny == y)
		return;
	if (screeny == y && x == 0) {
		conq('\r');
		screenx = 0;
		return;
	}
	if (screeny == y - 1 && x == 0) {
		con_newline();
		return;
	}
#endif	
	con_force_goto(y, x);
}

/* Clear to end of line */
void con_clear_to_eol(void)
{
	if (screenx == screen_width - 1)
		return;
	if (t_clreol)
		con_twrite(t_clreol, 1);
	else {
		uint_fast8_t i;
		/* Write spaces. This tends to put the cursor where
		   we want it next time too. Might be worth optimizing ? */
		for (i = screenx; i < screen_width; i++)
			con_putc(' ');
	}
}

/* Clear to the bottom of the display */

void con_clear_to_bottom(void)
{
	/* Most terminals have a clear to end of screen */
	if (t_clreos)
		con_twrite(t_clreos, screen_height);
	/* If not then clear each line, which may in turn emit
	   a lot of spaces in desperation */
	else {
		uint_fast8_t i;
		for (i = 0; i < screen_height; i++) {
			con_goto(i, 0);
			con_clear_to_eol();
		}
	}
	con_force_goto(0, 0);
}

void con_clear(void)
{
	con_goto(0, 0);
	con_clear_to_bottom();
}

int con_scroll(int n)
{
	if (n == 0)
		return 0;
	/* For now we don't do backscrolls: FIXME */
	if (n < 0)
		return 1;
	/* Scrolling down we can do */
	con_force_goto(screen_height - 1, 0);
	while (n--)
		conq('\n');
	con_force_goto(screeny, screenx);
}

/* TODO: cursor key handling */
int con_getch(void)
{
	uint8_t c;
	con_flush();
	if (read(0, &c, 1) != 1)
		return -1;
	return c;
}

int con_size(uint8_t c)
{
	if (c == '\t')
		return 8 - (screenx & 7);
	/* We will leave unicode out 8) */
	if (c > 127)
		return 4;
	if (c < 32 || c == 127)
		return 2;
	return 1;
}

static int do_read(int fd, void *p, int len)
{
	int l;
	if ((l = read(fd, p, len)) != len) {
		if (l < 0)
			perror("read");
		else
			write(2, "short read from tchelp.\n", 25);
		return -1;
	}
	return 0;
}

static char *tnext(char *p)
{
	return p + strlen(p) + 1;
}

static int tty_init(void)
{
	int fd[2];
	pid_t pid;
	int ival[3];
	int n;
	int status;

	if (pipe(fd) < 0) {
		perror("pipe");
		return -1;
	}

	pid = fork();
	if (pid == -1) {
		perror("fork");
		return -1;
	}

	if (pid == 0) {
		close(fd[0]);
		dup2(fd[1], 1);
		execl("/usr/lib/tchelp", "tchelp", "li#co#cm$ce$cd$cl$", NULL);
		perror("tchelp");
		_exit(1);
	}
	close(fd[1]);
	waitpid(pid, &status, 0);

	do_read(fd[0], ival, sizeof(int));
	if (ival[0] == 0)
		return -1;
	do_read(fd[0], ival + 1, 2 * sizeof(int));

	ival[0] -= 2 * sizeof(int);
	t_go = sbrk((ival[0] + 3) & ~3);

	if (t_go == (void *) -1) {
		perror("sbrk");
		return -1;
	}

	if (do_read(fd[0], t_go, ival[0]))
		return -1;

	close(fd[0]);
	t_clreol = tnext(t_go);
	t_clreos = tnext(t_clreol);
	if (*t_clreos == 0)	/* No clr eos - try for clr/home */
		t_clreos++;	/* cl cap if present */
	if (*t_go == 0) {
		write(2, "Insufficient terminal features.\n", 32);
		return -1;
	}
	/* TODO - screen sizes */
	screen_height = ival[1];
	screen_width = ival[2];
	/* need to try WINSZ and VT ioctls */
	return 0;
}

static struct termios con_termios, old_termios;

void con_exit(void)
{
	tcsetattr(0, TCSANOW, &old_termios);
}

int con_init(void)
{
	int n;
	static struct winsize w;
	if (tty_init())
		return -1;
	if (tcgetattr(0, &con_termios) == -1)
		return -1;
	memcpy(&old_termios, &con_termios, sizeof(struct termios));
	atexit(con_exit);
	con_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
	con_termios.c_iflag &= ~(IXON);
	con_termios.c_cc[VMIN] = 1;
	con_termios.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &con_termios) == -1)
		return -1;
#ifdef VTSIZE
	n = ioctl(0, VTSIZE, 0);
	if (n != -1) {
		screen_width = n & 0xFF;
		screen_height = (n >> 8) & 0xFF;
	}
#endif
	if (ioctl(0, TIOCGWINSZ, &w) == 0) {
		if (w.ws_col)
			screen_width = w.ws_col;
		if (w.ws_row)
			screen_height = w.ws_row;
	}
	return 0;
}


/* ---- */

/* Glue to the library */

#define REDRAW_MASK	0

static char wbuf[81];
static int wbp = 0;
static int upper;

static void display_exit(void)
{
  con_newline();
  con_flush();
}

static void display_init(void)
{
  if (con_init())
    exit(1);
  con_clear();
  con_goto(screen_height - 1, 0);
}

static void flush_word(void)
{
  if (screenx)
    con_putc(' ');
  wbuf[wbp] = 0;
  con_puts(wbuf);
  wbp = 0;
}

static void move_on(void)
{
    /* Move on a line in the correct manner */
    if (upper) {
      con_clear_to_eol();
      con_newline();
    } else {
      con_scroll(1);
      con_goto(screen_height - 1, 0);
    }
}

static void char_out(char c)
{
  if (c != ' ' && c != '\n') {
    if (wbp < 80)
      wbuf[wbp++] = c;
    return;
  }
  /* Does the word not fit ? */
  if (screenx + wbp + 1 >= screen_width)
    move_on();
  /* Write out the word */
  flush_word();
  if (c == '\n')
    move_on();
}

static void strout_lower(const uint8_t *p)
{
  while(*p)
    char_out(*p++);
}

static void strout_lower_spc(const uint8_t *p)
{
  strout_lower(p);
  char_out(' ');
}

static void decout_lower(uint16_t v)
{
#ifdef __linux__
  char buf[9];
  snprintf(buf, 8, "%d", v);	/* FIXME: avoid expensive snprintf */
  strout_lower((uint8_t *)buf);
#else
  strout_lower((uint8_t *)_itoa(v));
#endif
}

static void strout_upper(const uint8_t *p)
{
  strout_lower(p);
}

static void action_look(void);

static void line_input(int m)
{
  int c;
  char *p = linebuf;

  if (m == 0)
    action_look();

  do {
    c = con_getch();
    if (c == 8 || c == 127) {
      if (p > linebuf) {
        con_goto(screen_height - 1, screenx - 1);
        con_putc(' ');
        con_goto(screen_height - 1, screenx - 1);
        p--;
      }
      continue;
    }
    if (c > 31 && c < 127) {
      if (p < linebuf + 80 && screenx < screen_width - 1) {
        *p++ = c;
        con_putc(c);
      }
      continue;
    }
  }
  while (c != 13 && c != 10);
  *p = 0;
  con_scroll(1);
  con_goto(screen_height - 1, 0);
}

static char readchar(void)
{
  line_input(1);
  return *linebuf;
}


static uint8_t ly, lx;

static void begin_upper(void)
{
  ly = screeny;
  lx = screenx;
  flush_word();
  con_goto(0,0);
  upper = 1;
}

char xbuf[] = "<@><@><@><@><@><@><@><@><@><@><@><@><@><@><@><@><@><@><@><@><@><@><@><@><@><@><@>";

static void end_upper(void)
{
  flush_word();
  con_clear_to_eol();
  con_newline();
  upper = 0;
  xbuf[screen_width] = 0;
  con_puts(xbuf);  
  con_goto(ly, lx);
}

#else

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>

#define REDRAW_MASK	REDRAW

static char wbuf[80];
static int wbp = 0;
static int xpos = 0;

static void display_init(void)
{
  char *c;
#ifdef TIOCGWINSZ
  struct winsize w;
  if (ioctl(0, TIOCGWINSZ, &w) != -1) {
    rows = w.ws_row;
    cols = w.ws_col;
    return;
  }
#elif VTSIZE
  int16_t v = ioctl(0, VTSIZE, 0);
  if (v != -1) {
    rows =  v >> 8;
    cols = v;
    return;
  }
#endif
  c = getenv("COLS");
  rows = 25;
  cols = c ? atoi(c): 80;
  if (cols == 0)
    cols = 80;
}

static void display_exit(void)
{
}

static void flush_word(void)
{
  write(1, wbuf, wbp);
  xpos += wbp;
  wbp = 0;
}

static void char_out(char c)
{
  if (c == '\n') {
    flush_word();
    write(1, "\n", 1);
    xpos = 0;
    return;
  }
  if (c != ' ') {
    if (wbp < 80)
      wbuf[wbp++] = c;
    return;
  }
  if (xpos + wbp >= cols) {
    xpos = 0;
    write(1,"\n", 1);
  }
  flush_word();
  write(1," ", 1);
  xpos++;
}

static void strout_lower(const uint8_t *p)
{
  while(*p)
    char_out(*p++);
}

static void strout_lower_spc(const uint8_t *p)
{
  strout_lower(p);
  char_out(' ');
}

static void decout_lower(uint16_t v)
{
#ifdef __linux__
  char buf[9];
  snprintf(buf, 8, "%d", v);	/* FIXME: avoid expensive snprintf */
  strout_lower((uint8_t *)buf);
#else
  strout_lower((uint8_t *)_itoa(v));
#endif
}

static void strout_upper(const uint8_t *p)
{
  strout_lower(p);
}


static void line_input(int m)
{
  int l = read(0, linebuf, sizeof(linebuf));
  if (l < 0)
    error("read");
  linebuf[l] = 0;
  if (l && linebuf[l-1] == '\n')
    linebuf[l-1] = 0;
}

static char readchar(void)
{
  line_input(0);
  return *linebuf;
}

static void begin_upper(void)
{
  strout_upper("\n\n\n\n");
}

static void end_upper(void)
{
  uint8_t l = cols;
  char_out('\n');
  while(l--)
    char_out('-');
  char_out('\n');
}



#endif

/******************** Common code ******************/

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

static void exit_game(uint8_t code)
{
  display_exit();
  exit(code);
}

static void error(const char *p)
{
  display_exit();
  write(2, p, strlen(p));
  exit(1);
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

static int wordeq(const uint8_t *a, const char *b, uint8_t l)
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

static void scan_noun(char *x)
{
  x = skip_spaces(x);
  nounbuf = x;
  copyword(x);
  noun = whichword(nouns);
}

static void scan_input(void)
{
  char *x = copyword(linebuf);
  verb = whichword(verbs);
  scan_noun(x);
}

void abbrevs(void)
{
  char *x = skip_spaces(linebuf);
  const char *p = NULL;
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
    uint8_t op = game.objloc[par];
    opc &= 0x1F;

    switch(opc) {
      case 0:
        *param++ = par;
        break;
      case 1:
        if (op != 255)
          return NULL;
        break;
      case 2:
        if (op != game.location)
          return NULL;
        break;
      case 3:
        if (op != 255 && op != game.location)
          return NULL;
        break;
      case 4:
        if (game.location != par)
          return NULL;
        break;
      case 5:
        if (op == game.location)
          return NULL;
        break;
      case 6:
        if (op == 255)
          return NULL;
        break;
      case 7:
        if (game.location == par)
          return NULL;
        break;
      case 8:
        if (!(game.bitflags & (ONEBIT << par)))
          return NULL;
        break;
      case 9:
        if (game.bitflags & (ONEBIT << par))
          return NULL;
        break;
      case 10:
        if (!game.carried)
          return NULL;
        break;
      case 11:
        if (game.carried)
          return NULL;
        break;
      case 12:
        if (op == 255 || op == game.location)
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
        if (game.counter > par)
          return NULL;
        break;
      case 16:
        if (game.counter < par)
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
        if (game.counter != par)
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
  uint8_t l = game.objloc[LIGHT_SOURCE];
  if (!(game.bitflags & (ONEBIT << DARKFLAG)))
    return 1;
  if (l == 255 || l == game.location)
    return 1;
  return 0;
}

static void action_look(void)
{
  const uint8_t *e;
  const uint8_t *p;
  uint8_t c;
  uint8_t f = 1;
  const uint8_t **op = objtext;

  redraw = 0;

  begin_upper();

  if (!islight()) {
    strout_upper(itsdark);
    end_upper();
    return;
  }
  p = locdata[game.location].text;
  e = locdata[game.location].exit;
  if (*p == '*')
    p++;
  else
    strout_upper(youare);
  strout_upper(p);
  strout_upper(newline);
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
  e = game.objloc;
  while(e < game.objloc + NUM_OBJ) {
    if (*e++ == game.location) {
      if (f) {
        strout_upper(canalsosee);
        f = 0;
      } else
        strout_upper(dashstr);
      strout_upper(*op);
    }
    op++;
  }
  end_upper();
}

static void action_delay(void)
{
  sleep(2);
}

static void action_dead(void)
{
  strout_lower(dead);
  game.bitflags &= ~(ONEBIT << DARKFLAG);
  game.location = lastloc;
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
  uint8_t *p = game.objloc;
  const uint8_t **m = objtext;
  uint8_t t = 0, s = 0;

  while(p < game.objloc + NUM_OBJ) {
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
  uint8_t *p = game.objloc;
  const uint8_t **m = objtext;
  uint8_t f = 1;

  strout_lower(carrying);
  if (game.carried == 0)
    strout_lower(nothing);
  else {  
    while(p < game.objloc + NUM_OBJ) {
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

static char *filename(void)
{
  strout_lower("File name ? ");
  line_input(1);
  return skip_spaces(linebuf);
}

static void action_save(void)
{
  int fd;
  char *p = filename();
  if (*p == 0)
    return;
  game.magic = GAME_MAGIC;
  fd = open(p, O_WRONLY|O_CREAT|O_TRUNC, 0600);
  if (fd == -1 || write(fd, &game, sizeof(game)) != sizeof(game) || close(fd) == -1)
    strout_lower("Save failed.\n");
  close(fd);	/* Double closing is safe for non error path */
}

static int action_restore(void)
{
  while(1) {
    char *p = filename();
    int fd;

    if (*p == 0)
      return 0;

    fd = open(p, O_RDONLY, 0600);

    if (fd != -1 && read(fd, &game, sizeof(game)) == sizeof(game) && close(fd) != -1 &&
        game.magic == GAME_MAGIC)
      return 1;

    strout_lower("Load failed.\n");
    close(fd);
  }
}
  
static void moveitem(uint8_t i, uint8_t l)
{
  uint8_t *p = game.objloc + i;
  if (*p == game.location)
    redraw |= REDRAW_MAYBE;
  if (l == game.location)
    redraw |= REDRAW;
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
        if (game.carried >= maxcar)
          strout_lower(toomuch);
        else
          moveitem(*param++, 255);
        break;
      case 53: /* Drop */
        moveitem(*param++, game.location);
        break;
      case 54: /* Go */
        game.location = *param++;
        redraw |= REDRAW;
        break;
      case 55: /* Destroy */
      case 59: /* ?? */
        moveitem(*param++, 0);
        break;
      case 56:	/* Set dark flag */
        game.bitflags |= (ONEBIT << DARKFLAG);
        break;
      case 57:	/* Clear dark flag */
        game.bitflags &= ~(ONEBIT << DARKFLAG);
        break;
      case 58:	/* Set bit */
        game.bitflags |= (ONEBIT << *param++);
        break;
      /* 59 see 55 */
      case 60:	/* Clear bit */
        game.bitflags &= ~(ONEBIT << *param++);
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
        game.bitflags |= (ONEBIT << 0);
        break;
      case 68:	/* Clear bit 0 */
        game.bitflags &= ~(ONEBIT << 0);
        break;
      case 69:	/* Refill lamp */
        game.lighttime = lightfill;
        game.bitflags &= ~(ONEBIT << LIGHTOUT);
        moveitem(LIGHT_SOURCE, 255);
        break;
      case 70:	/* Wipe lower */
        /* TODO */
        break;
      case 71:	/* Save */
        action_save();
        break;
      case 72:	/* Swap two objects */
        tmp = game.objloc[*param];
        moveitem(*param, game.objloc[param[1]]);
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
        moveitem(*param, game.objloc[param[1]]);
        param += 2;
        break;
      case 77:	/* Decrement counter */
        if (game.counter >= 0)
          game.counter--;
        break;
      case 78:	/* Display counter */
        decout_lower(game.counter);
        break;
      case 79:	/* Set counter */
        game.counter = *param++;
        break;
      case 80:	/* Swap player and saved room */
        tmp = game.savedroom;
        game.savedroom = game.location;
        game.location = tmp;
        redraw |= REDRAW;
        break;
      case 81:	/* Swap counter and counter n */
        tmp16 = game.counter;
        game.counter = game.counter_array[*param];
        game.counter_array[*param++] = tmp16;
        break;
      case 82:	/* Add to counter */
        game.counter += *param++;
        break;
      case 83:	/* Subtract from counter */
        game.counter -= *param++;
        if (game.counter < 0)
          game.counter = -1;
        break;
      case 84:	/* Print noun, newline */
        strout_lower((uint8_t *)nounbuf);
        /* Fall through */
      case 86:	/* Print newline */
        strout_lower(newline);
        break;
      case 85:	/* Print noun */ 
        strout_lower((uint8_t *)nounbuf);
        break;
      case 87: /* Swap player and saveroom array entry */
        tmp16 = *param++;
        tmp = game.roomsave[tmp16];
        game.roomsave[tmp16] = game.location;
        if (tmp != game.location) {
          game.location = tmp;
          redraw |= REDRAW;
        }
        break;
      case 88:
        action_delay();
        break;
      case 89:
        param++;		/* SAGA etc specials */
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
      continuation = 0;
      if (!(hdr & 0x20)) {	/* Auto number */
        if (random_chance(*tp++))
          run_line(tp, c, a);
        else
          next_line();
        continue;
      }
      run_line(tp, c, a);
    } else {
      if (actmatch)
        return;
/*      printf("VN %d %d\n", *tp, tp[1]); */
      linematch = 1;
      continuation = 0;
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
    if (strncasecmp((const char *)p, wordbuf, WORDSIZE) == 0 && game.objloc[p[WORDSIZE]] == loc)
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
    else if (game.carried >= maxcar)
      strout_lower(toomuch);
    else {
      tmp = autonoun(game.location);
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
        moveitem(tmp, game.location);
    }
    actmatch = 1;
    return;
  }
}

void process_light(void)
{
  uint8_t l;
  if ((l = game.objloc[LIGHT_SOURCE]) == 0)
    return;
  if (game.lighttime == 255)
    return;
  if (!--game.lighttime) {
    game.bitflags &= ~(ONEBIT << LIGHTOUT);	/* Check clear ! */
    if (l == 255 || l == game.location) {
      strout_lower(lightout);
      redraw |= REDRAW_MAYBE;
      return;
    }
  }
  if (game.lighttime > 25)
    return;
  strout_lower(lightoutin);
  decout_lower(game.lighttime);
  strout_lower(game.lighttime == 1 ? turn : turns);
}

void main_loop(void)
{
  uint8_t first = 1;
  char *p;

  action_look();
  
  while (1) {
    if (!first)
      process_light();
    else
      first = 0;
    verb = 0;
    noun = 0;

    run_table(status);

    if (redraw & REDRAW_MASK)
      action_look();
    strout_lower(whattodo);

    do {
      do {
        strout_lower(prompt);
        line_input(0);
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
        dir = locdata[game.location].exit[noun - 1];
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
        game.location = dir;
        redraw |= REDRAW;
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
  memcpy(game.objloc, objinit, sizeof(game.objloc));
  game.bitflags = 0;
  game.counter = 0;
  memset(game.counter_array, 0, sizeof(game.counter_array));
  game.savedroom = 0;
  memset(game.roomsave, 0, sizeof(game.roomsave));
  game.location = startloc;
  game.lighttime = startlamp;
  game.carried = startcarried;
}

int main(int argc, char *argv[])
{
  display_init();
  setjmp(restart);
  strout_lower("Restore a saved game ? ");
  if (!yes_or_no() || !action_restore())
    start_game();
  main_loop();
}
