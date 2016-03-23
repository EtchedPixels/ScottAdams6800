/*
 *	MC-10 Scott Adams game generator, derived from ScottFree 1.14
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	3 of the License, or (at your option) any later version.
 *
 *
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

#include "Generator.h"

Header GameHeader;
Tail GameTail;
Item *Items;
Room *Rooms;
char **Verbs;
char **Nouns;
char **Messages;
Action *Actions;
int Options;		/* Option flags set */
int CPU;

static FILE *output;
static int fcb_n1;

void Fatal(char *x)
{
	fprintf(stderr,"%s.\n",x);
	exit(1);
}

void *MemAlloc(int size)
{
	void *t=(void *)malloc(size);
	if(t==NULL)
		Fatal("Out of memory");
	return(t);
}

int CountCarried()
{
	int ct=0;
	int n=0;
	while(ct<=GameHeader.NumItems)
	{
		if(Items[ct].Location == 255)
			n++;
		ct++;
	}
	return(n);
}

char *ReadString(FILE *f)
{
	char tmp[1024];
	char *t;
	int c,nc;
	int ct=0;
	do
	{
		c=fgetc(f);
	}
	while(c!=EOF && isspace(c));
	if(c!='"')
	{
		Fatal("Initial quote expected");
	}
	do
	{
		c=fgetc(f);
		if(c==EOF)
			Fatal("EOF in string");
		if(c=='"')
		{
			nc=fgetc(f);
			if(nc!='"')
			{
				ungetc(nc,f);
				break;
			}
		}
		if(c==0x60) 
			c='"'; /* pdd */
		if (ct == 1024)
			Fatal("Line too long");
		tmp[ct++]=c;
	}
	while(1);
	tmp[ct]=0;
	t=MemAlloc(ct+1);
	memcpy(t,tmp,ct+1);
	return(t);
}
	
void LoadDatabase(FILE *f, int loud)
{
	int ni,na,nw,nr,mc,pr,tr,wl,lt,mn,trm;
	int ct;
	short lo;
	Action *ap;
	Room *rp;
	Item *ip;
/* Load the header */
	
	if(fscanf(f,"%*d %d %d %d %d %d %d %d %d %d %d %d",
		&ni,&na,&nw,&nr,&mc,&pr,&tr,&wl,&lt,&mn,&trm)<10)
		Fatal("Invalid database(bad header)");
	GameHeader.NumItems=ni;
	Items=(Item *)MemAlloc(sizeof(Item)*(ni+1));
	GameHeader.NumActions=na;
	Actions=(Action *)MemAlloc(sizeof(Action)*(na+1));
	GameHeader.NumWords=nw;
	GameHeader.WordLength=wl;
	Verbs=(char **)MemAlloc(sizeof(char *)*(nw+1));
	Nouns=(char **)MemAlloc(sizeof(char *)*(nw+1));
	GameHeader.NumRooms=nr;
	Rooms=(Room *)MemAlloc(sizeof(Room)*(nr+1));
	GameHeader.MaxCarry=mc;
	GameHeader.PlayerRoom=pr;
	GameHeader.Treasures=tr;
	GameHeader.LightTime=lt;
	GameHeader.NumMessages=mn;
	Messages=(char **)MemAlloc(sizeof(char *)*(mn+1));
	GameHeader.TreasureRoom=trm;
	
/* Load the actions */

	ct=0;
	ap=Actions;
	if(loud)
		printf("Reading %d actions.\n",na);
	while(ct<na+1)
	{
		if(fscanf(f,"%hd %hd %hd %hd %hd %hd %hd %hd",
			&ap->Vocab,
			&ap->Condition[0],
			&ap->Condition[1],
			&ap->Condition[2],
			&ap->Condition[3],
			&ap->Condition[4],
			&ap->Action[0],
			&ap->Action[1])!=8)
		{
			printf("Bad action line (%d)\n",ct);
			exit(1);
		}
		ap++;
		ct++;
	}			
	ct=0;
	if(loud)
		printf("Reading %d word pairs.\n",nw);
	while(ct<nw+1)
	{
		Verbs[ct]=ReadString(f);
		Nouns[ct]=ReadString(f);
		ct++;
	}
	ct=0;
	rp=Rooms;
	if(loud)
		printf("Reading %d rooms.\n",nr);
	while(ct<nr+1)
	{
		fscanf(f,"%hd %hd %hd %hd %hd %hd",
			&rp->Exits[0],&rp->Exits[1],&rp->Exits[2],
			&rp->Exits[3],&rp->Exits[4],&rp->Exits[5]);
		rp->Text=ReadString(f);
		ct++;
		rp++;
	}
	ct=0;
	if(loud)
		printf("Reading %d messages.\n",mn);
	while(ct<mn+1)
	{
		Messages[ct]=ReadString(f);
		ct++;
	}
	ct=0;
	if(loud)
		printf("Reading %d items.\n",ni);
	ip=Items;
	while(ct<ni+1)
	{
		ip->Text=ReadString(f);
		ip->AutoGet=strchr(ip->Text,'/');
		/* Some games use // to mean no auto get/drop word! */
		if(ip->AutoGet && strcmp(ip->AutoGet,"//") && strcmp(ip->AutoGet,"/*"))
		{
			char *t;
			*ip->AutoGet++=0;
			t=strchr(ip->AutoGet,'/');
			if(t!=NULL)
				*t=0;
		}
		fscanf(f,"%hd",&lo);
		ip->Location=(unsigned char)lo;
		ip->InitialLoc=ip->Location;
		ip++;
		ct++;
	}
	ct=0;
	/* Comment Strings */
	ap = Actions;
	while(ct<na+1)
	{
		ap->Comment = ReadString(f);
		ap++;
		ct++;
	}
	fscanf(f,"%d",&ct);
	if(loud)
		printf("Version %d.%02d of Adventure ",
		ct/100,ct%100);
	fscanf(f,"%d",&ct);
	if(loud)
		printf("%d.\nLoad Complete.\n\n", ct);
}

static void newlines(void) {
	fprintf(output, "\n\n");
}

static void fcb(int v) {
	if (v < -128 || v > 255) {
		fprintf(stderr, "%d: ", v);
		Fatal("FCB out of range");
	}
	if (CPU == CPU_Z80)
		fprintf(output, "\t.db %d\n", v);
	else if (CPU == CPU_MC6800 || CPU == CPU_MC6801)
		fprintf(output, "\tfcb %d\n", v);
	else
		fprintf(output, "\t%d,\n", v);
}

static void fcb_cont(int v) {
	if (v < 0 || v > 255)
		Fatal("FCB out of range");
	if (fcb_n1) {
		if (CPU == CPU_MC6800 || CPU == CPU_MC6801)
			fprintf(output, "\tfcb %d", v);
		else if (CPU == CPU_Z80)
			fprintf(output, "\t.db %d", v);
		else
			fprintf(output, "\t%d, ", v);
	}
	else if (CPU != CPU_C)
		fprintf(output, ", %d", v);
	else
		fprintf(output, "%d, ", v);
	fcb_n1 = 0;
}

static void fcb_first(void) {
	fcb_n1 = 1;
}

static void fcb_last(void) {
	if (!fcb_n1)
		fprintf(output, "\n");
	fcb_n1 = 1;
}

static void label(const char *p) {
	if (CPU == CPU_C)
		fprintf(output, "const uint8_t %s[] = {\n", p);
	else
		fprintf(output, "%s:\n", p);
}

static void labelptr(const char *p) {
	if (CPU == CPU_C)
		fprintf(output, "const uint8_t *%s[] = {\n", p);
	else
		fprintf(output, "%s:\n", p);
}

static void label_end(const char *p) {
	if (CPU == CPU_C)
		fprintf(output, "};\n");
	else if (p)
		fprintf(output, "%s_end:\n", p);
}

static void label_fcb(const char *p, int v) {
	if (CPU == CPU_C)
		fprintf(output, "const uint8_t %s = %d;\n", p, v);
	else {
		label(p);
		fcb(v);
	}
}

static void pointer(const char *p) {
	if (CPU == CPU_MC6800 || CPU == CPU_MC6801)
		fprintf(output, "\tfdb %s\n", p);
	else if (CPU == CPU_Z80)
		fprintf(output, "\t.dw %s\n", p);
	else
		fprintf(output, "\t%s,\n", p);
}

static void equ(const char *a, int v) {
	if (CPU == CPU_C)
		fprintf(output, "#define %s %d\n", a, v);
	else
		fprintf(output, "%s\tequ %d\n", a, v);
}

static void string(const char *l, const char *p)
{
	char c;
	if (strchr(p, '\''))
		c = '"';
	else
		c  = '\'';

	if (CPU == CPU_C) {
		if (l)
			fprintf(output, "const uint8_t %s[] = {\n", l);
		while(*p)
			fprintf(output, "%d, ", *p++);
		fprintf(output, "0 };\n");
	} else if (CPU == CPU_Z80) {
		if (l)
			fprintf(output, "%s:", l);
		fprintf(output, "\t.asciz %c", c);
		while(*p) {
			if (*p != '\n')
				fprintf(output, "%c", *p);
			p++;
		}
		fprintf(output, "%c\n", c);
	} else {
		int n = 0;
		if (l)
			fprintf(output, "%s:", l);
		while(*p) {
			if (!(n % 16))
				fprintf(output, "\n\tfcb ");
			fprintf(output, "%d", (int)toupper(*p));
			if ((n++ % 16) != 15)
				fprintf(output, ",");
			p++;
		}
		fprintf(output, "\n\tfcb 0\n");
	}
}

static void outword(char *p) {
	int i;

	if (Options&UNCOMMENTED) {
		if (CPU == CPU_Z80)
			fprintf(output,"\t.db ");
		else if (CPU == CPU_MC6800 || CPU == CPU_MC6801)
			fprintf(output,"\tfcb ");
	} else {
		if (CPU == CPU_C)
			fprintf(output, "\t/* %s */\n\t", p);
		else if (CPU == CPU_Z80)
			fprintf(output, ";%s\n\t.db ", p);
		else
			fprintf(output, ";%s\n\tfcb ", p);
	}

	if (*p == '*')	/* Aliasing */
		*++p |= 0x80;

	for (i = 0; i < GameHeader.WordLength;i++) {
		if (i)
			fprintf(output, ", ");
		if (*p)
			fprintf(output, "%d", (unsigned char)*p++);
		else
			fprintf(output, "32");
	}
	if (CPU == CPU_C)
		fprintf(output, ",");
	fprintf(output, "\n");
}

static void copyin(const char *p) {
	FILE *i = fopen(p, "r");
	char buf[512];
	if (i == NULL) {
		perror(p);
		exit(1);
	}
	while(fgets(buf, 512, i)) {
		if (*buf == '0') {
			if (CPU == CPU_MC6800)
				fputs(buf + 1, output);
		}
		else if (*buf == '1') {
			if (!(CPU == CPU_MC6800))
				fputs(buf + 1, output);
		}
		else
			fputs(buf, output);
	}
	fclose(i);
}

int main(int argc, char *argv[])
{
	FILE *f;
	int i;
	int acts;
	char wname[32];
	
	while(argv[1])
	{
		if(*argv[1]!='-')
			break;
		switch(argv[1][1])
		{
			case 'y':
				Options|=YOUARE;
				break;
			case 'i':
				Options&=~YOUARE;
				break;
			case 'd':
				Options|=DEBUGGING;
				break;
			case 's':
				Options|=SCOTTLIGHT;
				break;
			case 'p':
				Options|=PREHISTORIC_LAMP;
				break;
			case 'u':
				Options|=UNCOMMENTED;
				break;
			case '0':
				CPU = CPU_MC6800;
				break;
			case '1':
				CPU = CPU_MC6801;
				break;
			case 'Z':
				CPU = CPU_Z80;
				break;
			case 'C':
				CPU = CPU_C;
				break;
			case 'h':
			default:
				fprintf(stderr,"%s: [-h] [-y] [-s] [-i] [-d] [-p] [-u] [-0|1|Z|C] <gamename> <asmname>.\n",
						argv[0]);
				exit(1);
		}
		if(argv[1][2]!=0)
		{
			fprintf(stderr,"%s: option -%c does not take a parameter.\n",
				argv[0],argv[1][1]);
			exit(1);
		}
		argv++;
		argc--;
	}			

	if(argc!=3)
	{
		fprintf(stderr,"%s <database> <asmfile>.\n",argv[0]);
		exit(1);
	}
	f=fopen(argv[1],"r");
	if(f==NULL)
	{
		perror(argv[1]);
		exit(1);
	}
	LoadDatabase(f,(Options&DEBUGGING)?1:0);
	fclose(f);
	
	output = fopen(argv[2], "w");
	if (output == NULL) {
		perror(argv[2]);
		exit(1);
	}

	equ("NUM_OBJ", GameHeader.NumItems);
	equ("WORDSIZE", GameHeader.WordLength);

	/* Main game file */
	if (CPU == CPU_MC6800 || CPU == CPU_MC6801)
		copyin("core.s");
	else if (CPU == CPU_Z80)
		copyin("core-z80.s");
	
	/* Word handler */
	switch (CPU) {
		case CPU_MC6800:
			sprintf(wname, "word%d-0.s", GameHeader.WordLength);
			break;
		case CPU_MC6801:
			sprintf(wname, "word%d.s", GameHeader.WordLength);
			break;
		case CPU_Z80:
			sprintf(wname, "word%d-z80.s", GameHeader.WordLength);
			break;
	}
	if (CPU != CPU_C)
		copyin(wname);

	/* Correct text messages */
	if (CPU == CPU_MC6800 || CPU == CPU_MC6801) {
		if (Options&YOUARE)
			copyin("youmsg.s");
		else
			copyin("imsg.s");

		copyin("bridge.s");
	} else if (CPU == CPU_Z80) {
		if (Options&YOUARE)
			copyin("youmsg-z80.s");
		else
			copyin("imsg-z80.s");

		copyin("bridge-z80.s");
	}
	if (CPU == CPU_C) {
		if (Options&YOUARE)
			copyin("youmsg-c.c");
		else
			copyin("imsg-c.c");

		copyin("bridge-c.c");
	}

	if (GameHeader.LightTime > 255) {
		fprintf(stderr, "Warning: 16bit light time ?\n");
		GameHeader.LightTime = 255;
	}
	label_fcb("startlamp", GameHeader.LightTime);
	label_fcb("lightfill", GameHeader.LightTime);
	label_fcb("startcarried", CountCarried());
	label_fcb("maxcar", GameHeader.MaxCarry);
	label_fcb("treasure", GameHeader.TreasureRoom);
	label_fcb("treasures", GameHeader.Treasures);
	label_fcb("lastloc", GameHeader.NumRooms);  
	label_fcb("startloc", GameHeader.PlayerRoom);

	for (i = 0; i <= GameHeader.NumRooms; i++) {
		sprintf(wname, "loctxt_%d", i);
		string(wname, Rooms[i].Text);
	}
	newlines();

	if (CPU == CPU_C) {
		fprintf(output, "const struct location locdata[] = {\n");
		for (i = 0; i <= GameHeader.NumRooms; i++) {
			fprintf(output, "\t\t{ loctxt_%d, ", i);
			fprintf(output, " { %d, %d, %d, %d, %d, %d } }, \n",
				Rooms[i].Exits[0],
				Rooms[i].Exits[1],
				Rooms[i].Exits[2],
				Rooms[i].Exits[3],
				Rooms[i].Exits[4],
				Rooms[i].Exits[5]);
		}
		fprintf(output, "};\n");
	} else {
		label("locdata");
		for (i = 0; i <= GameHeader.NumRooms; i++) {
			sprintf(wname, "loctxt_%d\n", i);
			pointer(wname);
			if (CPU == CPU_Z80)
				fprintf(output, "\t.db");
			else if (CPU == CPU_C)
				fprintf(output, "BUG");
			else
				fprintf(output, "\tfcb");
			fprintf(output, " %d, %d, %d, %d, %d, %d\n",
				Rooms[i].Exits[0],
				Rooms[i].Exits[1],
				Rooms[i].Exits[2],
				Rooms[i].Exits[3],
				Rooms[i].Exits[4],
				Rooms[i].Exits[5]);
		}
	}
	label("objinit");
	for (i = 0; i <= GameHeader.NumItems; i++)
		fcb(Items[i].InitialLoc);
	label_end("objinit");
	newlines();
	for (i = 0; i <= GameHeader.NumItems; i++) {
		sprintf(wname, "objtxt_%d", i);
		string(wname, Items[i].Text);
	}
	newlines();
	labelptr("objtext");
	for (i = 0; i <= GameHeader.NumItems; i++) {
		sprintf(wname, "objtxt_%d", i);
		pointer(wname);
	}
	label_end(NULL);
	for (i = 0; i <= GameHeader.NumMessages; i++) {
		sprintf(wname, "msgtxt_%d", i);
		string(wname, Messages[i]);
	}
	labelptr("msgptr");
	for (i = 0; i <= GameHeader.NumMessages; i++) {
		sprintf(wname, "msgtxt_%d", i);
		pointer(wname);
	}
	label_end(NULL);
	newlines();
	label("status");
	acts = 0;
	for (i = 0; i <= GameHeader.NumActions; i++) {
		int v, n;
		int cc, ac;
		int a;
		uint8_t hdr = 0;
		uint8_t acode[4];

		v = Actions[i].Vocab / 150;
		n = Actions[i].Vocab % 150;
		if (acts == 0 && v) {
			acts = 1;
			label_end(NULL);
			label("actions");
		}

		if (!(Options & UNCOMMENTED)) {
			if (CPU == CPU_C)
				fprintf(output, "/* ");
			else
				fprintf(output, "; ");
			if (v)
				fprintf(output, "%-5s %-5s",
					Verbs[v], Nouns[n]?Nouns[n]:"-");
			else
				fprintf(output, "AUTO  %-5d", n);
			fprintf(output,"\t%s", Actions[i].Comment);
			if (CPU == CPU_C)
				fprintf(output, "*/");
			fprintf(output, "\n");
		}

		if (v == 0) {
			hdr |= 0x80;
			if (n == 0)
				hdr |= 0x40;
			else if (n == 100)
				hdr |= 0x20;
		}
		cc = 0;
		for (a = 4; a >= 0; a--) {
			if (Actions[i].Condition[a]) {
				cc = a + 1;
				break;
			}
		}
		acode[0] = Actions[i].Action[0] / 150;
		acode[1] = Actions[i].Action[0] % 150;
		acode[2] = Actions[i].Action[1] / 150;
		acode[3] = Actions[i].Action[1] % 150;

		ac = 0;
		for (a = 3; a >= 0; a--) {
			if (acode[a]) {
				ac = a + 1;
				break;
			}
		}
		hdr |= cc << 2;
		hdr |= ac - 1;

		/* Drop out any dummy lines */
		if (ac || cc) {
			fcb_first();
			fcb_cont(hdr);
			if (v) {
				fcb_cont(v);
				fcb_cont(n);
			} else {
				if (n && n != 100) {
					if (CPU != CPU_C)
						n = n *256 / 100;
					fcb_cont(n);
				}
			}
			fcb_last();
			for (a = 0; a < cc; a++) {
				uint16_t t = Actions[i].Condition[a];
				uint8_t code = t % 20;
				t /= 20;
				fcb_cont(code | ((t >> 3) & 0xE0));
				fcb_cont(t & 0xFF);
			}
			fcb_last();
			for (a = 0; a < ac; a++)
				fcb_cont(acode[a]);
			fcb_last();
		}
	}
	fcb(255);
	label_end(NULL);
	newlines();
	label("verbs");
	for (i = 0; i <= GameHeader.NumWords; i++)
		outword(Verbs[i]);
	fcb(0);
	label_end(NULL);
	label("nouns");
	for (i = 0; i <= GameHeader.NumWords; i++)
		outword(Nouns[i]);
	fcb(0);
	label_end(NULL);
	label("automap");
	for (i = 0; i <= GameHeader.NumItems; i++) {
		if (Items[i].AutoGet) {
			outword(Items[i].AutoGet);
			fcb(i);
		}
	}
	fcb(0);
	label_end(NULL);

	if (CPU != CPU_C) {
		label("percentages");
		for (i = 0; i <= GameHeader.Treasures; i++)
			fcb((i * 100)/GameHeader.Treasures);
		label("zzzz");
	} else
		copyin("core-c.c");
	fclose(output);
	return 0;
}
	