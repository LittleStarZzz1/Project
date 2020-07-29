#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>

#include "Coroutine.h"

int tcp_init()
{
    //创建套接字
    int listen_id = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_id == -1)
    {
        perror("socket error~~\n");
        exit(-1);
    }
    
    int op = 1;
    setsockopt(listen_id, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op));

    //定义ipv4地址节后
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = bind(listen_id, (struct sockaddr*)&addr, sizeof(struct sockaddr));

    if (ret == -1)
    {
        perror("bind error~~~\n");
        exit(-1);
    }

    listen(listen_id, SOMAXCONN);//SOMAXCONN定义了每一个端口最大监听队列长度

    return listen_id;
}

void set_unblock(int fd)//设置非阻塞
{
   int flag = fcntl(fd, F_GETFL, 0);
   flag |= O_NONBLOCK;
   fcntl(fd, F_SETFL, flag);
}

void accept_connect(int listen_fd, schedule_t* s, int co_ids[], void* (*call_back)(schedule_t* s, void* args))
{
    while (1)
    {
        int accept_fd = accept(listen_fd, NULL, NULL);
        if (accept_fd > 0)
        {
            set_unblock(accept_fd);//设置非阻塞
            int args[] = {listen_fd, accept_fd};
            int id = coroutine_create(s, call_back, args);

            int i = 0;
            for (; i < COROUTINE_SIZE; ++i)
            {
                if (co_ids[i] == -1)
                {
                    co_ids[i] = id;
                    break;
                }
            }
            if (i == COROUTINE_SIZE)
            {
                printf("连接过多~~~\n");
            }
            coroutine_running(s, id);
        }
        else
        {
            int i = 0;
            for (; i < COROUTINE_SIZE; ++i)
            {
                if (co_ids[i] == -1)
                    continue;
                coroutine_resume(s, co_ids[i]);
            }
        }
    }
}

void* _handler(schedule_t* s, void* args)
{
    int* arr = (int*)args;
    int accept_fd = arr[1];

    char buf[1024] = {};

    while (1)
    {
        memset(buf, 0x00, sizeof(buf));

        int ret = read(accept_fd, buf, 1023);

        if (ret == -1)
        {
            coroutine_yield(s);//读不到放弃CPU
        }
        else if (ret == 0)
        {
            break;//数据读完了,直接break
        }
        else
        {
            printf("Recv data : %s\n", buf);
            if (strncasecmp(buf, "exit", 4) == 0)
                break;
            write(accept_fd, buf, ret);
        }
    }
}

int main()
{
    int listen_fd = tcp_init();
    set_unblock(listen_fd);

    schedule_t* s = schedule_create();//创建协程调度器

    int co_fds[COROUTINE_SIZE];
    int i;
    for (i = 0; i < COROUTINE_SIZE; ++i)
    {
        co_fds[i] = -1;
    }

    accept_connect(listen_fd, s, co_fds, _handler);

    return 0;
}


