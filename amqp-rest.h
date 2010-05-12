/*
 *  amqp-rest.h
 *  amqp-rest
 *
 *  Created by Alexandre Kalendarev on 30.04.10.
 *
 */

#ifndef __AMQPREST__
#define __AMQPREST__

typedef struct {
    int			fd;
    unsigned char		*bot, *tok, *ptr, *cur, *pos, *lim, *top, *eof, *ptok,parm;
    unsigned int		line;
} Scanner;


enum log_type_e {
	LOG_STDERR,
	LOG_ERROR,
	LOG_CONTENT,
	LOG_ALERT,
	LOG_WARNING,
    LOG_NOTICE,
	LOG_DEBUG
};

typedef struct  {
	int   log_fd;
	int   pidfd;
	char * confile;
	char * amqp_connect;
	char * bind;
	char * logfile; 	
	char * pidfile; 	
	AMQPQueue * queue;
	AMQPExchange * exchange;
	short daemon;
    short	port;
	char ip[16];
	char ** ppa;
	short log_level;

} ctx;

int scan(Scanner *s);

#endif