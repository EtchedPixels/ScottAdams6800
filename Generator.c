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
	if (v < 0 || v > 255)
		Fatal("FCB out of range");
	fprintf(output, "\tfcb %d\n", v);
}

static void fcb_cont(int v) {
	if (v < 0 || v > 255)
		Fatal("FCB out of range");
	if (fcb_n1)
		fprintf(output, "\tfcb %d", v);
	else
		fprintf(output, ", %d", v);
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
	fprintf(output, "%s:\n", p);
}

static void label_fcb(const char *p, int v) {
	label(p);
	fcb(v);
}

static void equ(const char *a, int v) {
	fprintf(output, "%s\tequ %d\n", a, v);
}

static void string(const char *p)
{
	char c;
	if (strchr(p, '\''))
		c = '"';
	else
		c  = '\'';

	fprintf(output, "\tfcc %c", c);
	while(*p) {
		if (*p != '\n')
			fprintf(output, "%c", toupper(*p));
		p++;
	}
	fprintf(output, "%c\n\tfcb 0\n", c);
}

static void outword(char *p) {
	int i;

	fprintf(output, ";%s\n\tfcb ", p);

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
			if (Options & MC6800)
				fputs(buf + 1, output);
		}
		else if (*buf == '1') {
			if (!(Options & MC6800))
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
			case '0':
				Options|=MC6800;
				break;
			case 'h':
			default:
				fprintf(stderr,"%s: [-h] [-y] [-s] [-i] [-d] [-p] [-0] <gamename> <asmname>.\n",
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

	/* Main game file */	
	copyin("core.s");
	
	/* Word handler */
	sprintf(wname, "word%d%s.s",
		GameHeader.WordLength,
		(Options&MC6800)?"-0":"");
	copyin(wname);

	/* Correct text messages */
	if (Options&YOUARE)
		copyin("youmsg.s");
	else
		copyin("imsg.s");

	copyin("bridge.s");

	label_fcb("wordsize", GameHeader.WordLength);
	label_fcb("startlamp", GameHeader.LightTime);
	label_fcb("lightfill", GameHeader.LightTime);
	label_fcb("startcarried", CountCarried());
	label_fcb("maxcar", GameHeader.MaxCarry);
	label_fcb("treasure", GameHeader.TreasureRoom);
	label_fcb("treasures", GameHeader.Treasures);
	label_fcb("lastloc", GameHeader.NumRooms);  
	label_fcb("startloc", GameHeader.PlayerRoom);

	label("locdata");
	for (i = 0; i <= GameHeader.NumRooms; i++) {
		fprintf(output, "\tfdb loctxt_%d\n", i);
		fprintf(output, "\tfcb %d, %d, %d, %d, %d, %d\n",
			Rooms[i].Exits[0],
			Rooms[i].Exits[1],
			Rooms[i].Exits[2],
			Rooms[i].Exits[3],
			Rooms[i].Exits[4],
			Rooms[i].Exits[5]);
	}
	for (i = 0; i <= GameHeader.NumRooms; i++) {
		fprintf(output, "loctxt_%d:\n", i);
		string(Rooms[i].Text);
	}
	newlines();
	label("objinit");
	for (i = 0; i <= GameHeader.NumItems; i++)
		fcb(Items[i].InitialLoc);
	label("objinit_end");
	newlines();
	label("objtext");
	for (i = 0; i <= GameHeader.NumItems; i++)
		fprintf(output, "\tfdb objtxt_%d\n", i);
	for (i = 0; i <= GameHeader.NumItems; i++) {
		fprintf(output, "objtxt_%d:\n", i);
		string(Items[i].Text);
	}
	newlines();
	label("msgptr");
	for (i = 0; i <= GameHeader.NumMessages; i++)
		fprintf(output, "\tfdb msgtxt_%d\n", i);
	for (i = 0; i <= GameHeader.NumMessages; i++) {
		fprintf(output, "msgtxt_%d:\n", i);
		string(Messages[i]);
	}
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
			label("actions");
		}

		if (v)
			fprintf(output, "; %-5s %-5s",
				Verbs[v], Nouns[n]?Nouns[n]:"-");
		else
			fprintf(output, "; AUTO  %-5d", n);
		fprintf(output,"\t%s\n", Actions[i].Comment);

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

		ac = 1;
		for (a = 3; a >= 0; a--) {
			if (acode[a]) {
				ac = a + 1;
				break;
			}
		}
		hdr |= cc << 2;
		hdr |= ac - 1;
		fcb_first();
		fcb_cont(hdr);
		if (v) {
			fcb_cont(v);
			fcb_cont(n);
		} else {
			if (n && n != 100)
				fcb_cont(n);
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
	fcb(255);
	newlines();
	label("verbs");
	for (i = 0; i <= GameHeader.NumWords; i++)
		outword(Verbs[i]);
	fcb(0);
	label("nouns");
	for (i = 0; i <= GameHeader.NumWords; i++)
		outword(Nouns[i]);
	fcb(0);
	label("automap");
	for (i = 0; i <= GameHeader.NumItems; i++) {
		if (Items[i].AutoGet) {
			outword(Items[i].AutoGet);
			fcb(i);
		}
	}
	fcb(0);
	fclose(output);
	return 0;
}
	