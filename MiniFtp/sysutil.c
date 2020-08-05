#include "sysutil.h"

//在该模块中完成对socket的初始化
//创建监听套接字
int tcp_server(const char* ip, uint16_t port)
{
    int listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listen_fd < 0)
        ERR_EXIT("socket error~~\n");

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    //设置套接字属性
    int op = 1;
    //int setsockopt(int socket, int level, int option_name,
    //               const void *option_value, socklen_t option_len);              
    //SOL_SOCKET 基本套接字
    //SO_REFUSEADDR 允许重用本地地址和端口
    int ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op));
    if (ret < 0)
        ERR_EXIT("setsockopt error~~~\n");

    //绑定服务端地址信息
    ret = bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0)
        ERR_EXIT("bind error~~~\n");

    //开始监听
    ret = listen(listen_fd, SOMAXCONN);
    if (ret < 0)
        ERR_EXIT("listen error~~~\n");

    //返回监听套接字的描述符
    return listen_fd;
}

int tcp_client()
{
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        ERR_EXIT("socker error~~\n");
    return fd;
}

const char* statbuf_get_purview(struct stat* sbuf)
{
    // - --- --- ---
    static char purview[] = "----------";
    mode_t mode = sbuf->st_mode;

    switch(mode & S_IFMT)
    {
        case S_IFSOCK:
            purview[0] = 's';
            break;
        case S_IFLNK:
            purview[0] = 'l';//链接文件
            break;
        case S_IFREG:
            purview[0] = '-';//普通文件
            break;
        case S_IFBLK:
            purview[0] = 'b';
            break;
        case S_IFDIR:
            purview[0] = 'd';//目录文件
            break;
        case S_IFCHR:
            purview[0] = 'c';//字符文件
            break;
        case S_IFIFO:
            purview[0] = 'p';//管道文件
            break;
    }
    if (mode & S_IRUSR)
        purview[1] = 'r';
    if (mode & S_IWUSR)
        purview[2] = 'w';
    if (mode & S_IXUSR)
        purview[3] = 'x';

    if (mode & S_IRGRP)
        purview[4] = 'r';
    if (mode & S_IWGRP)
        purview[5] = 'w';
    if (mode & S_IXGRP)
        purview[6] = 'x';

    if (mode & S_IROTH)
        purview[7] = 'r';
    if (mode & S_IWOTH)
        purview[8] = 'w';
    if (mode & S_IXOTH)
        purview[9] = 'x';

    return purview;
}

const char* statbuf_get_date(struct stat* sbuf)
{
    static char datebuf[64] = {0};
    time_t time_file = sbuf->st_mtime;//获取文件最后一次修改时间
    struct tm* ptm = localtime(&time_file);//将时间格式化,或者说格式化一个时间字符串

    //%b月份的简写 %e十进制表示每月的第几天
    // %H 24小时制的小时  %M 十进制表示的分钟数
    strftime(datebuf, 64, "%b %e %H:%M", ptm);
    return datebuf;
}






