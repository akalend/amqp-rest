#include <signal.h>
#include <err.h>
#include <event.h>
#include <evhttp.h>
#include "amqpcpp.h"

#include "amqp-rest.h"
#include "amqp_conf.c"
#include "ini_parse.c"
#include "tools.c"
#include "http_handler.cpp"



ctx  Ctx;
struct evhttp *httpd;

int main( int argc, const char** argv ){

	struct event_base *ev_base; 
	int res=0;
	int is_stop=0;

	if (argc > 1) {
		Ctx.confile = (char *) argv[1];
		if (strcmp(Ctx.confile,"stop")==0) {
			is_stop=1;
			Ctx.confile = "/usr/local/etc/amqp-rest.conf";
		printf( "option stop\n");			
		}
	} else
		Ctx.confile = "/usr/local/etc/amqp-rest.conf";

	ini_parse(&Ctx);
	
	if (is_stop) {
		pid_t pid = pid_read(&Ctx); 
		if (pid > 0) {
			kill(pid, SIGTERM);
		}
		free_ctx(&Ctx);			
		return 0;
	}
	
	if (pid_exist(&Ctx)) {
		printf( "the pid file exist or daemon already started\n");
		free_ctx(&Ctx);		
		return 1;
	}
	
	if (Ctx.daemon) {
		res=demonize(&Ctx);
		if (res==-1) {
			free_ctx(&Ctx);	
			return 1;
		}
	}
	
	log_open(&Ctx);
	res=pid_create(&Ctx);
	if (res) {
		free_ctx(&Ctx);	
		return 1;
	}
		
	setSignal();
		
	tolog(&Ctx, LOG_NOTICE, "server started" );

    ev_base = event_init();
	httpd = evhttp_new (ev_base);
	
    //httpd = evhttp_start(Ctx.bind, Ctx.port);
	if (evhttp_bind_socket (httpd, Ctx.bind, (u_short)Ctx.port )) { 
		tolog(&Ctx, LOG_ERROR, "Failed to bind %s:%d", Ctx.bind, Ctx.port);
		log_close(&Ctx);		
		free_ctx(&Ctx);
		exit (1); 
	} 	
	
	res=0;
	AMQP * amqp = NULL;
	try {
		amqp = new AMQP(Ctx.amqp_connect);
		Ctx.queue = amqp->createQueue();
		Ctx.exchange = amqp->createExchange();

	} catch (AMQPException e) {
		tolog(&Ctx, LOG_ERROR, "amqp error %s", e.getMessage().c_str());
		res=1;
	}

	if (!res) {
		evhttp_set_gencb(httpd, http_handler, NULL);

		event_base_dispatch(ev_base);
		evhttp_free(httpd);
	}

	delete amqp;
	pid_delete(&Ctx);
	tolog(&Ctx, LOG_NOTICE, "server finished ");
	log_close(&Ctx);
	free_ctx(&Ctx);	

	return 0;
}
