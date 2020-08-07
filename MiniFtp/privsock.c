#include "privsock.h"
#include "sysutil.h"

void priv_sock_init(session_t *sess)//初始化内部进程间通信通道
{
    int sockfds[2];
    int ret;
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sockfds);
    if (ret < 0)
        ERR_EXIT("socketpair error~~\n");

    sess->parent_fd = sockfds[0];
    sess->child_fd = sockfds[1];

}
void priv_sock_close(session_t *sess)//关闭内部进程间通信通道
{
    if (sess->child_fd != -1)
    {
        close(sess->child_fd);
        sess->child_fd = -1;
    }
    if (sess->parent_fd != -1)
    {
        close(sess->parent_fd);
        sess->parent_fd = -1;
    }
}
void priv_sock_set_parent_context(session_t *sess)//设置父进程上下文环境
{
    if (sess->child_fd != -1)
    {
        close(sess->child_fd);
        sess->child_fd = -1;
    }
}
void priv_sock_set_child_context(session_t *sess)//设置子进程上下文环境
{
    if (sess->parent_fd != -1)
    {
        close(sess->parent_fd);
        sess->parent_fd = -1;
    }
}
void priv_sock_send_cmd(int fd, char cmd)//发送命令(子->父)
{
    //在privsock.h中定义的命令只有四个
    int ret = send(fd, &cmd, sizeof(cmd), 0);
    if (ret != sizeof(cmd))
        ERR_EXIT("prive_sock_send_cmd error~~\n");
}
char priv_sock_get_cmd(int fd)//获取命令(父<-子)
{
    char cmd;
    int ret = recv(fd, &cmd, sizeof(cmd), 0);
    if (ret == 0)
    {
        printf("ftp process exit~~\n");
        exit(EXIT_FAILURE);
    }
    if (ret != sizeof(cmd))
        ERR_EXIT("priv_sock_get_cmd error~~\n");
    return cmd;
}
void priv_sock_send_result(int fd, char res)//发送结果(父->子)
{
    int ret = send(fd, &res, sizeof(res), 0);
    if (ret != sizeof(res))
        ERR_EXIT("priv_sock_send_result error~~\n");
}
char priv_sock_get_result(int fd)//获取结果(子<-父)
{
    char res;
    int ret = recv(fd, &res, sizeof(res), 0);
    if (ret == 0)
    {
        printf("ftp process exit\n");
        exit(EXIT_FAILURE);
    }
    if (ret != sizeof(res))
        ERR_EXIT("priv_sock_get_result error~~\n");
    return res;
}
void priv_sock_send_int(int fd, int the_int)//发送一个整型
{
    int ret = send(fd, &the_int, sizeof(the_int), 0);
    if (ret != sizeof(the_int))
        ERR_EXIT("priv_sock_send_int error~~\n");
}
int  priv_sock_get_int(int fd)//获取一个整型
{
    int the_int;
    int ret = recv(fd, &the_int, sizeof(the_int), 0);
    if (ret == 0)
    {
        printf("ftp process exit\n");
        exit(EXIT_FAILURE);
    }
    if (ret != sizeof(the_int))
        ERR_EXIT("priv_sock_get_int error~~\n");
    return the_int;
}
void priv_sock_send_buf(int fd, const char *buf, unsigned int len)//发送一个字符串
{
    priv_sock_send_int(fd, (int)len);
    int ret = send(fd, buf, len, 0);
    if (ret != (int)len)
        ERR_EXIT("priv_sock_send_buf error~~\n");
}
void priv_sock_recv_buf(int fd, char *buf, unsigned int len)//接收字符串
{
    unsigned int buf_len = (unsigned int)priv_sock_get_int(fd);
    if (buf_len > len)
        ERR_EXIT("priv_sock_rev_buf error~~\n");

    int ret = recv(fd, buf, len, 0);
    if (ret != (int)buf_len)
        ERR_EXIT("priv_sock_rev_buf error~~\n");
}
void priv_sock_send_fd(int sock_fd, int fd)//发送描述符
{
    send_fd(sock_fd, fd);
}
int  priv_sock_recv_fd(int sock_fd)//接收描述符
{
    return recv_fd(sock_fd);
}

