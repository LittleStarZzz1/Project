#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = inet_addr("192.168.188.131");

    socklen_t len = sizeof(struct sockaddr_in);

    int ret = connect(fd, (struct sockaddr*)&addr, len);

    if (ret == -1)
    {
        perror("connect error~~~n");
        exit(-1);
    }

    char buf[1024] = {};

    while (fgets(buf, 1024, stdin) != NULL)
    {
        write(fd, buf, strlen(buf));
        memset(buf, 0x00, sizeof(buf));

        int ret = read(fd, buf, 1024);
        if (ret <= 0)
            break;
        printf("=> %s\n", buf);
    }

    return 0;
}
