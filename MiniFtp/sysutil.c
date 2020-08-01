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





