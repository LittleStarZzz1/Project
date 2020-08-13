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

//获取当前系统的秒数
long get_time_sec();
//获取当前系统的微秒数
long get_time_usec();

void nono_sleep(double sleep_time);


#endif //_SYSUTIL_H
