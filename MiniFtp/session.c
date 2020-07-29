#include "session.h"
#include "ftp_proto.h"
#include "priv_parent.h"

void session_begin(session_t* sess)
{
    pid_t pid = fork();

    if (pid == -1)
        ERR_EXIT("fork error~~~\n");

    if (pid == 0)//子进程
    {
        //ftp 服务进程
        handler_child(sess);
    }

    else
    {
        //nobody 进程
        //把root进程更名为 nobody进程
        struct passwd* pw = getpwnam("nobody");
        if (pw == NULL)
            ERR_EXIT("getpwnam error~~\n");

        if (setegid(pw->pw_gid) < 0)
            ERR_EXIT("setgid error~~\n");

        if (seteuid(pw->pw_uid) < 0)
            ERR_EXIT("seteuid error~~\n");

        handler_parent(sess);
    }
}



