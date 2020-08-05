#ifndef _SYSUTIL_H
#define _SYSUTIL_H

#include "common.h"

int tcp_server(const char* ip, uint16_t port);
int tcp_client();

const char* statbuf_get_purview(struct stat* sbuf);

const char* statbuf_get_date(struct stat* sbuf);

#endif //_SYSUTIL_H
