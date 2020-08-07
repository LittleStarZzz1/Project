#ifndef _PRIVSOCK_H
#define _PRIVSOCK_H

#include "common.h"
#include "session.h"

//FTP服务进程向nobody进程请求的命令
#define PRIV_SOCK_GET_DATA_SOCK 1 // 获取数据连接套接字
#define PRIV_SOCK_PASV_ACTIVE   2 // 被动连接是否被激活
#define PRIV_SOCK_PASV_LISTEN   3 // 获取被动连接的监听套接字
#define PRIV_SOCK_PASV_ACCEPT   4 // 获取被动连接模式下接收连接的套接字

//nobody 进程对FTP服务进程的应答
#define PRIV_SOCK_RESULT_OK  1    // 请求ok
#define PRIV_SOCK_RESULT_BAD 2    // 请求出错

void priv_sock_init(session_t *sess);//初始化内部进程间通信通道
void priv_sock_close(session_t *sess);//关闭内部进程间通信通道
void priv_sock_set_parent_context(session_t *sess);//设置父进程上下文环境
void priv_sock_set_child_context(session_t *sess);//设置子进程上下文环境
void priv_sock_send_cmd(int fd, char cmd);//发送命令(子->父)
char priv_sock_get_cmd(int fd);//获取命令(父<-子)
void priv_sock_send_result(int fd, char res);//发送结果(父->子)
char priv_sock_get_result(int fd);//获取结果(子<-父)
void priv_sock_send_int(int fd, int the_int);//发送一个整型
int  priv_sock_get_int(int fd);//获取一个整型
void priv_sock_send_buf(int fd, const char *buf, unsigned int len);//发送一个字符串
void priv_sock_recv_buf(int fd, char *buf, unsigned int len);//接收字符串
void priv_sock_send_fd(int sock_fd, int fd);//发送描述符
int  priv_sock_recv_fd(int sock_fd);//接收描述符

#endif // _PRIVSOCK_H
