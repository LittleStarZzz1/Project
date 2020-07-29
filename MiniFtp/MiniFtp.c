#include "common.h"
#include "sysutil.h"
#include "session.h"


int main(int argc, char* argv)
{
   session_t sess = 
   {
       //控制连接
       -1, -1, "", "", ""
   };

    int listen_fd = tcp_server("192.168.188.131", 9000);
    
    int sock_Conn;
    struct sockaddr_in addr_Cli;
    socklen_t addrlen;
    while (1)
    {
        //不断循环获取客户端的连接
        sock_Conn = accept(listen_fd, (struct sockaddr*)&addr_Cli, &addrlen);

        if (sock_Conn < 0)
            ERR_EXIT("accept error~~~\n");

        //创建子进程
        pid_t pid = fork();

        if (pid == -1)
            ERR_EXIT("fork error~~~\n");

        if (pid == 0)//子进程
        {
            close(listen_fd);

            //会话
            sess.ctrl_fd = sock_Conn;
            session_begin(&sess);

            exit(EXIT_SUCCESS);
        }
        else//父进程
        {
            close(sock_Conn);
        }
    }
    close(listen_fd);

    return 0;
}





