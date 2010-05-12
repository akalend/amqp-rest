#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>

#define	EOI	0

//typedef unsigned int uint;
typedef unsigned char uchar;

#define	BSIZE	8192

#define	YYCTYPE		uchar
#define	YYCURSOR	cursor
#define	YYLIMIT		s->lim
#define	YYMARKER	s->ptr
#define	YYFILL(n)	{cursor = fill(s, cursor);}

#define	RET(i)	{s->cur = cursor; return i;}

#include "amqp-rest.h"

unsigned char *fill(Scanner *s, uchar *cursor){
	if(!s->eof) {
		unsigned int cnt = s->tok - s->bot;
		if(cnt){
			memcpy(s->bot, s->tok, s->lim - s->tok);
			s->tok = s->bot;
			s->ptr -= cnt;
			cursor -= cnt;
			s->pos -= cnt;
			s->lim -= cnt;
		}
		if((s->top - s->lim) < BSIZE){
			unsigned char *buf = (uchar*) malloc(((s->lim - s->bot) + BSIZE)*sizeof(uchar));
			memcpy(buf, s->tok, s->lim - s->tok);
			s->tok = buf;
			s->ptr = &buf[s->ptr - s->bot];
			cursor = &buf[cursor - s->bot];
			s->pos = &buf[s->pos - s->bot];
			s->lim = &buf[s->lim - s->bot];
			s->top = &s->lim[BSIZE];
			free(s->bot);
			s->bot = buf;
		}
		if((cnt = read(s->fd, (char*) s->lim, BSIZE)) != BSIZE){
			s->eof = &s->lim[cnt]; *(s->eof)++ = '\n';
		}
		s->lim += cnt;
	}
	return cursor;
}

int scan(Scanner *s){
	unsigned char *cursor = s->cur;
	int ret=0;
std:
	s->tok = cursor;
/*!re2c
any	= [\000-\377];
O	= [0-7];
D	= [0-9];
L	= [a-zA-Z_];
H	= [a-fA-F0-9];
E	= [Ee] [+-]? D+;
FS	= [fFlL];
IS	= [uUlL]*;
ESC	= [\\] ([abfnrtv?'"\\] | "x" H+ | O+);
EXPR	= [0-9a-zA-Z_];
PATH	= [0-9a-zA-Z_] | "." | "/" |  "-";
CONNSTR	= [0-9a-zA-Z_] | (".") | ("/") | (":") | ("@");
IP	= [0-7] | (".") | (":")? ;
NL	= "\r"? "\n" ;
TAB	= "\t";
*/

/*!re2c
	
	"/*"			{ goto comment; }
	"#"			{ goto unixcomment; }

	"daemon"		{
						s->parm = 1;
						goto _bool;
					}
	
	"logfile"		{
						s->parm = 2;
						goto param;
					}

	"log_level"		{
						s->parm = 5;
						goto lparam;
					}
	"port"		{
						s->parm = 3;
						goto dparam;
					}

	"http"		{
						s->parm = 6;
						goto ip_param;
					}
	"amqp"		{
						s->parm = 4;
						goto param;
					}

	"pidfile"		{
						s->parm = 7;
						goto param;
					}


	
	[\r\v\f]+		{ RET(8912); goto std; }

	";" 			{  goto std; }      
	" " 			{  goto std; }
 	"\t"	 		{ goto std;  }   

	NL
	    {
		if(cursor == s->eof) RET(EOI);
		s->pos = cursor; s->line++;
		goto std;
	    }

	  any
	    {
		printf("unexpected character: %c line %d \n", *s->tok, s->line);
		goto std;
	    }
*/
//s->tok = cursor; s->parm =3; goto param; 
//(PATH)*";"		{ RET(2001); }
//
	


param:
 s->ptok=cursor;
/*!re2c 

	(PATH)*  ";"	{ RET(303); } 	
	(CONNSTR)* ";"	{ RET(301 ); }
	(EXPR)*  ";"	{ RET(302); } 	

	any		{  goto param; }

*/

_bool: 
 s->ptok=cursor;
/*!re2c 
	"on"   { RET( 101); } 
	"off"  { RET( 102); } 
	"1"	{ RET(101); } 
	"0"  { RET(102); } 

	 ";"	{ goto std; } 	

	any		{  goto _bool; }

*/

lparam: 
 s->ptok=cursor;
/*!re2c 
	"error"    { RET(1); } 
	"content"  { RET(2); } 
	"alert"    { RET(3); } 
	"warning"  { RET(4); } 
	"notice"   { RET(5); } 
	"debug"    { RET(6); } 

	 ";"	{ goto std; } 	

	any		{  goto lparam; }

*/

ip_param:
 s->ptok=cursor;
 ret =  100;
/*!re2c 
	(IP)* ";"	{ RET(ret); }
	any		{  goto ip_param; }
*/

dparam:
 s->ptok=cursor;
 ret =  200;
/*!re2c 
	(D)* ";"	{ RET(ret); }
	any		{  goto dparam; }
*/

unixcomment:	
/*!re2c
	NL			{  s->line++; goto std; }
	any			{ goto unixcomment; }
*/

comment:
/*!re2c
	"*/"			{ goto std; }

	"\n"
	    {
		if(cursor == s->eof) RET(EOI);
		s->tok = s->pos = cursor; s->line++;
		goto comment;
	    }
        any	{ goto comment; }
*/
}



