#include "session.h"
#include "ftp_proto.h"
#include "priv_parent.h"
#include "privsock.h"

void session_begin(session_t* sess)
{
    priv_sock_init(sess);
    pid_t pid = fork();
    if (pid == -1)
        ERR_EXIT("fork error~~~\n");
    if (pid == 0)//子进程
    {
        priv_sock_set_child_context(sess);
        //ftp 服务进程
        handler_child(sess);
    }
    else
    {
        //nobody 进程
        priv_sock_set_parent_context(sess);
        handler_parent(sess);
    }
    //priv_sock_close(sess);
}



