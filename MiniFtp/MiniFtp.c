#include "common.h"
#include "sysutil.h"
#include "session.h"


int main(int argc, char* argv)
{
    //权限提升, 以root用户打开Miniftp
    if (getuid() != 0)
    {
        printf("MiniFtp must be started as root.\n");
        exit(EXIT_FAILURE);
    }
    session_t sess = 
    {
        //控制连接
        -1, -1, "", "", "",
        
        //数据连接
        NULL, -1, -1,

        // ftp协议状态
        0,

        //父子进程通道
        -1, -1
    };
    
    //创建监听套接字
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

            //设置会话
            sess.ctrl_fd = sock_Conn;
            session_begin(&sess);

            exit(EXIT_SUCCESS);
        }
        else//父进程
        {
            close(sock_Conn);
        }
    }
    //关闭套接字
    close(listen_fd);

    return 0;
}





