/*
 *  ini_parse.cpp
 *  amqp-rest
 *
 *  Created by Alexandre Kalendarev on 30.04.10.
 *
 */


#ifndef __PARSEINI__
#define __PARSEINI__

#include "amqp-rest.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int ini_parse( ctx * Ctx) {
	
	Scanner in;
    int t;
    memset((char*) &in, 0, sizeof(in));
    in.fd = 0;
	unsigned int len;
	
	in.fd = open( Ctx->confile, O_RDONLY);
	if (in.fd <1) {
	//	printf("conf %s\n", Ctx->confile);
		perror("can't open config file");
		exit(1);
	}   	
		
	bzero(Ctx,sizeof(Ctx));
	Ctx->port = 80;
	Ctx->log_level = 1;
	
    while((t = scan(&in)) != 0){
	
		len = in.cur-in.ptok-1;

//		printf("%d\t'%.*s'   line=%d  param=%d\n", t, in.cur - in.tok, in.tok ,in.line+1, in.parm);
//		printf("%d %d '%.*s' \n\n", in.cur - in.tok ,in.cur-in.ptok,len, in.ptok);
  
		switch ( in.parm ) {
			case 1: {
			if (t==101)
				Ctx->daemon=1;
				break;
			} 	
			case 2: {
				if (t==303) {
				 Ctx->logfile = (char*)malloc(len+1);
				 if (Ctx->logfile ==NULL) {
					perror("malloc logfile");
					return 1;
				}
					
				 memcpy(Ctx->logfile,in.ptok,len);
				 *(Ctx->logfile+len)='\0';
				} else {
					printf("WARNING: log filename parse error\n");
					break;				
				}
				break;
			} 	
			case 3: {				
				if (t==200) {
				Ctx->port = atoi((const char *)in.ptok);
				}
				break;
			} 	
			case 4: {
			
				if (t==301) {
				 Ctx->amqp_connect = (char*)malloc(len+1);
				 if (Ctx->amqp_connect == NULL) {
					perror("WARNING: malloc amqp_connect");
					return 1;
				}

				 memcpy(Ctx->amqp_connect,in.ptok,len);
				 *(Ctx->amqp_connect+len)='\0';
				} else {
					printf("WARNING: amqp connect string parse error\n");
					break;				
				}
				break;
			} 	
			case 5: {			
				Ctx->log_level = t;
				if (Ctx->log_level<1) {
					printf("WARNING: log level parse error\n");
					Ctx->log_level=1;
				}	
				break;
			}
			case 6: {			
				if (t==100) {
				 Ctx->bind = (char*)malloc(len+1);
				 if (Ctx->bind == NULL) {
					perror("WARNING: malloc bind");
					return 1;
				}

				 memcpy(Ctx->bind,in.ptok,len);
				 *(Ctx->bind+len)='\0';
				} else {
					printf("WARNING: http IP parse error\n");
					break;				
				}
				break;
			}
			case 7: {
				if (t==303) {
				 Ctx->pidfile = (char*)malloc(len+1);
				 if (Ctx->pidfile ==NULL) {
					perror("malloc pidfile");
					return 1;
				}
					
				 memcpy(Ctx->pidfile,in.ptok,len);
				 *(Ctx->pidfile+len)='\0';
				} else {
					printf("WARNING: pid filename parse error\n");
					break;				
				}
				break;
			} 	
			
			default : {
				printf("WARNING: parse error\n");
				break;
			}
		}
		in.parm = 0;
    }
	
    close(in.fd);
	
	if (!Ctx->logfile) {
		Ctx->logfile=strdup("/var/log/amqp-rest.log");
	}

	if (!Ctx->pidfile) {
		Ctx->pidfile=strdup("/var/amqp-rest.pid");
	}
	
	if (!Ctx->bind) {
		printf("ERROR: http IP error\n");
		return 1;
	}
	
	//printf("daemon %d\n log level=%d '%s'\nport=%d %s\namqp '%s'\n", Ctx->daemon,Ctx->log_level, Ctx->logfile,Ctx->port,Ctx->bind,Ctx->amqp_connect);
	
	return 0;
}


void free_ctx(ctx * Ctx) {
	if (Ctx->logfile!=NULL)
		free(Ctx->logfile);

	if (Ctx->amqp_connect != NULL)
		free(Ctx->amqp_connect);
		
	if (Ctx->pidfile) {
		free(Ctx->pidfile);
	}

	if (Ctx->bind) {
		free(Ctx->bind);
	}
		
		
}
#endif