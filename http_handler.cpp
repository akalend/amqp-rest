/*
 *  http_handler.cpp
 *  amqp-rest
 *
 *  Created by Alexandre Kalendarev on 01.05.10.
 *
 */

#ifndef __HTTPHANDLER__
#define __HTTPHANDLER__

#include <err.h>
#include <event.h>
#include <evhttp.h>
#include <memory>
#include "amqpcpp.h"
#include <cstdarg>

#include "tools.c"
void  tolog(ctx * Ctx, int type, const char * message , ...);

extern ctx Ctx;


void http_finalize(struct evhttp_request *req, struct evbuffer *buf ) {
	evhttp_add_header(req->output_headers, "Content-type", "text/json");
	evhttp_add_header(req->output_headers, "Server", "amqp-rest");
	
	evhttp_send_reply(req, HTTP_OK, "OK", buf);	
	evbuffer_free(buf);
}

void parse_uri(const char * uri, char * name, int name_len, char * parm, int parm_len) {
	char * p = (char *)uri;
	char * pn = name;
	int len=0;

	while(*p && *p != '?' && len < name_len) {
		if (*p=='/') {			
			pn=name;
			bzero(pn,name_len);
			p++;
		}	
		*(pn++) =*(p++);
		len++;	
	}
	
	if (!*p)
		return;
	
	len=0;
	p++;
	pn=parm;	
	while(*p && len < parm_len) {			
		*(pn++) =*(p++);
		len++;	
	}		
	
}
int str_escape(const char * in , char * out) {
	int len = 0;
	char * p = (char *) in;
	while (*p) {
		if (*p == '"') {
			*out++ = '\\';
			len++;
		}
		*out++ = *p++;
		len++;
	}
	
	*out = '\0';
	return len;
}

void sendCount(struct evhttp_request *req, char * name) {
        struct evbuffer *buf;
			
        buf = evbuffer_new();
		if (buf == NULL) {
			tolog( &Ctx, LOG_STDERR, "failed to create response buffer");
		}

		try {				
				Ctx.queue->Declare(name);
				char out[64];
				sprintf(out, "{\"result\": \"Ok\", \"count\": %d }\r\n", Ctx.queue->getCount() );
				evbuffer_add_printf(buf, out );					
				
			} catch (AMQPException e) {		
				tolog(&Ctx, LOG_ERROR, "error QUEUE.DECLARE: %s", e.getMessage().c_str());	
				tolog(&Ctx, LOG_CONTENT, " %s GET %s code:%d",*Ctx.ppa, evhttp_request_uri(req), e.getReplyCode());
				evbuffer_add_printf(buf, "{\"result\": \"error\"}\r\n" );					
			}

	http_finalize( req, buf); 

}


void sendGetAll(struct evhttp_request *req, char * name) {
	struct evbuffer *buf;
		
	buf = evbuffer_new();
	if (buf == NULL) {
		tolog( &Ctx, LOG_STDERR, "failed to create response buffer");
//           err(1, "failed to create response buffer");
	}

	try {
	
		Ctx.queue->setName(name);
		Ctx.queue->reopen();
		
		bool is_first=1;
		int count = 0;
		string tmp("");
		char * out_buf = (char *) malloc( 131072 );

		while(1) {
			Ctx.queue->Get(AMQP_NOACK);
			AMQPMessage * m = Ctx.queue->getMessage();
					
			if ( is_first && m->getMessageCount() == -1) {
				evbuffer_add_printf(buf, "{\"result\": \"empty\"}\r\n" );
				tolog(&Ctx, LOG_CONTENT, " %s GET %s empty 200 OK",*Ctx.ppa, evhttp_request_uri(req));
				break;		
			} 	

			if ( is_first ) {
				is_first = 0;
				count = m->getMessageCount()+1;
			}
		
			char * msg = m->getMessage();
			string tmp2;
			
			if ( m->getMessageCount() == -1) {
					break;		
			}
			
			if (!msg) {
				tolog(&Ctx, LOG_ERROR, "message is null");
				tmp2="\"";
			} else	{			
				str_escape(msg, out_buf );				
				tmp2 = string("\"")+ out_buf;  

				if ( m->getMessageCount() == 0) {
					tmp += tmp2 + "\"";
				} else	
					tmp += tmp2 + "\",";	
			}
		}
		
		free(out_buf);
		char str_count[16]; bzero(str_count,16);
		sprintf(str_count,"%d",count);
		if (!is_first){
			string res = string("{\"result\": \"OK\", \"count\" : ") +  str_count  +string(", messages [") + tmp +string("]}\r\n");		
			evbuffer_add_printf(buf, res.c_str() );
			tolog(&Ctx, LOG_CONTENT, " %s GET %s bytes %d 200 OK", *Ctx.ppa, evhttp_request_uri(req), res.size());
		}
						
	} catch (AMQPException e) {		
		tolog(&Ctx, LOG_ERROR, "error BASIC.GET: %s", e.getMessage().c_str());	
		tolog(&Ctx, LOG_CONTENT, " %s GET %s code:%d", *Ctx.ppa, evhttp_request_uri(req), e.getReplyCode());
		evbuffer_add_printf(buf, "{\"result\": \"error\"}\r\n" );					
	}

	http_finalize( req, buf); 
}

void sendGet(struct evhttp_request *req, char * name) {
	struct evbuffer *buf;
		
	buf = evbuffer_new();
	if (buf == NULL) {
		tolog( &Ctx, LOG_STDERR, "failed to create response buffer");
//           err(1, "failed to create response buffer");
	}


	try {
		Ctx.queue->reopen();
		Ctx.queue->setName(name);
		Ctx.queue->Get(AMQP_NOACK);
		AMQPMessage * m = Ctx.queue->getMessage();				
	
		if (m->getMessageCount() == -1) {
			evbuffer_add_printf(buf, "{\"result\": \"empty\"}\r\n" );

			tolog(&Ctx, LOG_CONTENT, " %s GET %s empty 200 OK", *Ctx.ppa,evhttp_request_uri(req));
		} else {
			
			char * msg = m->getMessage();
			
			char * out_buf = (char *) malloc( 131072 );
			char tmp[32]; bzero(tmp,32);
					
			if ( out_buf )  {
				str_escape(msg, out_buf ); 
				sprintf(tmp,"%d", m->getMessageCount());						
				string out="{\"result\": \"OK\", \"message\":\""+ string(out_buf) +"\", size : "+ tmp +"}\r\n" ;
				tolog(&Ctx, LOG_CONTENT, " %s GET %s byte:%d 200 OK",*Ctx.ppa, evhttp_request_uri(req), strlen(msg) );
				evbuffer_add_printf(buf, out.c_str());
				free(out_buf);
			} else {
				tolog(&Ctx, LOG_STDERR, "malloc buf");
				evbuffer_add_printf(buf, "{\"result\": \"error\"}\r\n" );
			}

		}
				
	} catch (AMQPException e) {		
		tolog(&Ctx, LOG_ERROR, "error BASIC.GET: %s", e.getMessage().c_str());	
		tolog(&Ctx, LOG_CONTENT, " %s GET %s code:%d",*Ctx.ppa, evhttp_request_uri(req), e.getReplyCode());
		evbuffer_add_printf(buf, "{\"result\": \"error\"}\r\n" );					
	}

	http_finalize( req, buf); 
}

void sendPublish(struct evhttp_request *req, char * name, char * parm) {
	struct evbuffer *buf;
	string out;
		
	buf = evbuffer_new();
	if (buf == NULL) {
		tolog( &Ctx, LOG_STDERR, "failed to create response buffer");
//           err(1, "failed to create response buffer");
	}

	try {
		const char * str_len = evhttp_find_header(req->input_headers, "Content-Length");
		int len = atoi(str_len);
		
		out.assign((const char *)EVBUFFER_DATA(req->input_buffer),len);

		Ctx.queue->reopen();
		Ctx.exchange->setName(name);
		Ctx.exchange->Publish( out, parm );

		evbuffer_add_printf(buf, "{\"result\": \"Ok\"}\r\n");
		tolog(&Ctx, LOG_CONTENT, " %s POST %s bytes: %d 200 OK", *Ctx.ppa, evhttp_request_uri(req), len);	

	
	} catch (AMQPException e) {		
		tolog(&Ctx, LOG_ERROR, "error BASIC.PUBLISH: %s", e.getMessage().c_str());	
		tolog(&Ctx, LOG_CONTENT, " %s POST %s code:%d", *Ctx.ppa, evhttp_request_uri(req), e.getReplyCode());
		evbuffer_add_printf(buf, "{\"result\": \"error\"}\r\n" );					
	}
	
	http_finalize( req, buf); 

}

void http_handler(struct evhttp_request *req, void *arg) {
		
		const char * uri = evhttp_request_uri(req);
		char name[256]; bzero(name,256);
		char parm[16];  bzero(parm,16);
		
		parse_uri(uri, name, 256, parm,16);

		u_short port=0;	
		bzero(Ctx.ip,16);
		char * pa=Ctx.ip;
		char ** ppa=&pa;
		evhttp_connection_get_peer(req->evcon, ppa, &port);
		Ctx.ppa=ppa;
	
		if (req->type == EVHTTP_REQ_POST) {
			sendPublish(req,name, parm);
//			evhttp_send_reply(req, HTTP_OK, "OK", buf);	
			return;
		}

		if (req->type == EVHTTP_REQ_GET) {
			if (strcmp(parm,"all")==0) 
				sendGetAll(req, name);		
			else if (strcmp(parm,"count")==0) 
				sendCount(req, name);
			else
				sendGet(req, name);
			return;
		}
}


#endif