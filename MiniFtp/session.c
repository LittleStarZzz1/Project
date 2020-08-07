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
        priv_sock_set_parent_context(sess);

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
    priv_sock_close(sess);
}



