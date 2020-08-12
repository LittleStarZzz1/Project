#ifndef _SYSUTIL_H
#define _SYSUTIL_H

#include "common.h"

int tcp_server(const char* ip, uint16_t port);
int tcp_client(int port);

const char* statbuf_get_purview(struct stat* sbuf);

const char* statbuf_get_date(struct stat* sbuf);

void send_fd(int sock_fd, int fd);
int recv_fd(const int sock_fd);

void getLocalip(char* ip);



#endif //_SYSUTIL_H
