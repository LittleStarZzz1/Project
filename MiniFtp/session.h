#ifndef _SESSION_H
#define _SESSION_H

#include "common.h"

typedef struct session
{
    uid_t uid;
    //控制连接
    int ctrl_fd;

    char cmd_line[MAX_COMMAND_SIZE];//命令行
    char cmd[MAX_COMMAND];//命令
    char arg[MAX_ARG];//参数

    // 数据连接
    struct sockaddr_in* port_addr;
    int data_fd;
    int pasv_listen_fd;

    // ftp 协议状态(ascii or binary)
    int is_ascii;
    char* rnfr_name;
    long long restart_pos;//记录断点续传重新开始的位置

    // 父子进程通道
    int parent_fd;
    int child_fd;
}session_t;

void session_begin(session_t* sess);

#endif //_SESSION_H
