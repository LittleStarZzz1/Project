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
}session_t;

void session_begin(session_t* sess);

#endif //_SESSION_H
