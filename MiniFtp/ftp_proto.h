#ifndef _FTP_PROTO_H
#define _FTP_PROTO_H

#include "common.h"
#include "session.h"

//回应函数
void ftp_reply(session_t* sess, int code, const char* text);

void handler_child(session_t* sess);



#endif //_FTP_PROT_H
