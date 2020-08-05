#include "ftp_proto.h"
#include "ftp_codes.h"
#include "str.h"
#include "sysutil.h"

//ftp服务进程

// --------------
// |命令映射机制|
// --------------

typedef struct ftpcmd
{
    const char* cmd;//命令
    void (*cmd_handler)(session_t* sess);//命令对应的回调函数
}ftpcmd_t;

static void do_user(session_t* sess);
static void do_pass(session_t* sess);
static void do_syst(session_t* sess);
static void do_feat(session_t* sess);
static void do_pwd(session_t* sess);
static void do_type(session_t* sess);
static void do_port(session_t* sess);
static void do_list(session_t* sess);

//命令映射表
static ftpcmd_t ctrl_cmds[] =
{
    {"USER", do_user},
    {"PASS", do_pass},
    {"SYST", do_syst},
    {"FEAT", do_feat},
    {"PWD", do_pwd},
    {"TYPE", do_type},
    {"PORT", do_port},
    {"LIST", do_list}
};

//////////////////////////////////////////////////////////////////////


//封装回应函数
void ftp_reply(session_t* sess, int code, const char* text)
{
    char buf[MAX_BUFFER_SIZE] = {0};
    sprintf(buf, "%d %s\r\n", code, text);
    send(sess->ctrl_fd, buf, strlen(buf), 0);
}

void handler_child(session_t* sess)
{
    //send(sess->ctrl_fd, "220 (MiniFtp 1.0)\r\n",strlen("220 (MiniFtp 1.0))\r\n"), 0);
    ftp_reply(sess, FTP_GREET, "(test MiniFtp 1.0)");

    int ret;
    while (1)
    {
        //不停地等待客户端的命令并进行处理
        memset(sess->cmd_line, 0, MAX_COMMAND_SIZE);
        memset(sess->cmd, 0, MAX_COMMAND);
        memset(sess->arg, 0, MAX_ARG);

        ret = recv(sess->ctrl_fd, sess->cmd_line, MAX_COMMAND_SIZE, 0);
        if (ret == -1)
            ERR_EXIT("recv error~~\n");
        else if (ret == 0)
        {
            exit(EXIT_SUCCESS);
        }
        // printf("recv success~~\n");
        //接下来对命令进行解析
        str_trim_crlf(sess->cmd_line);
        //printf("cmd_line : %s\n", sess->cmd_line); 

        str_split(sess->cmd_line, sess->cmd, sess->arg, ' ');
        //printf("cmd : %s\n", sess->cmd);
        //printf("arg : %s\n", sess->arg);

        int table_size = sizeof(ctrl_cmds) / sizeof(ftpcmd_t);
        int i;

        for (i = 0; i < table_size; ++i)
        {
            //该命令存在命令映射表当中
            if (strcmp(sess->cmd, ctrl_cmds[i].cmd) == 0)
            {
                if (ctrl_cmds[i].cmd_handler != NULL)//回调函数不为空
                {
                    ctrl_cmds[i].cmd_handler(sess);
                }
                else//回调函数为空,表示该命令的回调函数未实现
                {
                    ftp_reply(sess, FTP_COMMANDNOTIMPL, "Unimplement command.");
                }
                break;
            }
        }
        if (i >= table_size)
        {
            //该命令不存在命令映射表当中
            ftp_reply(sess, FTP_BADCMD, "Unknown command.");
        }
    }
}

//对应命令的回调函数
static void do_user(session_t* sess)
{
    struct passwd* pwd = getpwnam(sess->arg);

    if (pwd != NULL)
    {
        sess->uid = pwd->pw_uid;
    }
    ftp_reply(sess, FTP_GIVEPWORD, "Please specify the password.");
}

static void do_pass(session_t* sess)
{
    //鉴权
    struct passwd* pwd = getpwuid(sess->uid);
    if (pwd == NULL)
    {
        ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
        return;
    }
    struct spwd* spd = getspnam(pwd->pw_name);
    if (spd == NULL)
    {
        ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
        return;
    }

    char* encry_pwd = crypt(sess->arg, spd->sp_pwdp);
    if (strcmp(encry_pwd, spd->sp_pwdp) != 0)
    {
        ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
        return;
    }

    setegid(pwd->pw_gid);
    seteuid(pwd->pw_uid);
    chdir(pwd->pw_dir);

    ftp_reply(sess, FTP_LOGINOK, "Login successful.");
}

static void do_syst(session_t* sess)
{
    ftp_reply(sess, FTP_SYSTOK, "UNIX Type: L8");
}

static void do_feat(session_t* sess)
{
    //服务器支持的命令列表
    send(sess->ctrl_fd, "211-Features:\r\n" ,strlen("211-Features:\r\n"), 0);
    send(sess->ctrl_fd, " EPRT\r\n", strlen(" EPRT\r\n"), 0);
    send(sess->ctrl_fd, " EPSV\r\n", strlen(" EPSV\r\n"), 0);
    send(sess->ctrl_fd, " MDTM\r\n", strlen(" MDTM\r\n"), 0);
    send(sess->ctrl_fd, " PASV\r\n", strlen(" PASV\r\n"), 0);
    send(sess->ctrl_fd, " REST STREAM\r\n", strlen(" REST STREAM\r\n"), 0);
    send(sess->ctrl_fd, " SIZE\r\n", strlen(" SIZE\r\n"), 0);
    send(sess->ctrl_fd, " TVFS\r\n", strlen(" TVFS\r\n"), 0);
    send(sess->ctrl_fd, " UTF8\r\n", strlen(" UTF8\r\n"), 0);
    send(sess->ctrl_fd, "211 End\r\n", strlen("211 End\r\n"), 0);
}

static void do_pwd(session_t* sess)
{
    char buffer[MAX_BUFFER_SIZE] = {0};
    getcwd(buffer, MAX_BUFFER_SIZE);//获取当前路径到buffer中, (/home/test)

    char msg[MAX_BUFFER_SIZE] = {0};// "/home/test"
    sprintf(msg, "\"%s\"", buffer);

    ftp_reply(sess, FTP_PWDOK, msg);
}

void do_type(session_t* sess)
{
    // TYPE A (ASCII) or TYPE I (BINARY)
    // ASCII传输模式
    if (strcmp(sess->arg, "A") == 0)
    {
        sess->is_ascii = 1;
        ftp_reply(sess, FTP_TYPEOK, "Switching to ASCII mode.");
    }
    
    // 二进制传输模式
    else if (strcmp(sess->arg, "I") == 0)
    {
        sess->is_ascii = 0;
        ftp_reply(sess, FTP_TYPEOK, "Switching to Binary mode.");
    }
    else
        //无效的传输模式
        ftp_reply(sess, FTP_BADCMD, "Unrecognised TYPE command.");
}

static void do_port(session_t* sess)
{
    //主动模式建立连接
    //PORT 192,168,188,131,1,7,34
    unsigned int v[6] = {0};
    sscanf(sess->arg, "%u,%u,%u,%u,%u,%u", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);

    sess->port_addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));

    //拿到客户端端口
    unsigned char* p = (unsigned char*)&sess->port_addr->sin_port;
    p[0] = v[4];
    p[1] = v[5];
    
    //拿到客户端IP地址
    p = (unsigned char*)&sess->port_addr->sin_addr;
    p[0] = v[0];
    p[1] = v[1];
    p[2] = v[2];
    p[3] = v[3];

    sess->port_addr->sin_family = AF_INET;
    ftp_reply(sess, FTP_PORTOK, "command successful. Consider using PASV.");
}

static void do_pasv(session_t* sess)
{
    // 227 Entering Passive Mode (192,168,188,131,135,82)
    char ip[16] = "192.168.188.131";//服务器IP

    sess->pasv_listen_fd = tcp_server(ip, 0);//端口号为0表示生成临时端口

    struct sockaddr_in address;
    socklen_t socklen = sizeof(struct sockaddr);
    if (getsockname(sess->pasv_listen_fd, (struct sockaddr*)&address, &socklen) < 0)
        ERR_EXIT("setsockname error~~~\n");

    uint16_t port = ntohs(address.sin_port);

    int v[4] = {0};
    sscanf(ip, "%u.%u.%u.%u", &v[0], &v[1], &v[2], &v[3]);
    char msg[MAX_BUFFER_SIZE] = {0};
    sprintf(msg, "Entering Passive Mode (%u,%u,%u,%u,%u,%u).",
            v[0], v[1], v[2], v[3], port >> 8, port & 0xff);

    ftp_reply(sess, FTP_PASVOK, msg);

}

////////////////////////////////////////////////////////////////////////////////

int port_active(session_t* sess)
{
    if (sess->port_addr)
        return 1;//保存了客户端的IP和端口,表明是主动模式,主动模式被激活
    return 0;
}

int pasv_active(session_t* sess)
{
    if (sess->pasv_listen_fd != -1)
        return 1;
    return 0;
}

int get_transfer_fd(session_t* sess)
{
   if (!port_active(sess) && !pasv_active(sess))
   {
       ftp_reply(sess, FTP_BADSENDCONN,"Use PORT or PASV first.");
       return 0;
   }

   int ret = 1;
   //port
   if (port_active(sess))
   {
       int fd = tcp_client();

       if (connect(fd, (struct sockaddr*)sess->port_addr, sizeof(struct sockaddr)) < 0)
       {
           ret = 0;
       }
       else
       {
           sess->data_fd = fd;
       }
   }

   //pasv
   if (pasv_active(sess))
   {
        int fd = accept(sess->pasv_listen_fd, NULL, NULL);
        if (fd < 0)
            ret = 0;
        else
        {
            close(sess->pasv_listen_fd);
            sess->pasv_listen_fd = -1;
            sess->data_fd = fd;
        }
   }

   if (sess->port_addr)
   {
       free(sess->port_addr);
       sess->port_addr = NULL;
   }

   return ret;
}

static void list_common(session_t* sess)
{
    DIR* dir = opendir(".");//打开当前目录
    if (dir == NULL)
        return;
    
    //drwxr-xr-x    3 1000     1000           30 Sep 09  2019 Desktop
    char buf[MAX_BUFFER_SIZE] = {0};

    struct stat sbuf;//用于保存文件属性
    struct dirent* dt;//保存读取到的目录信息

    while ((dt = readdir(dir)) != NULL)
    {
        if (stat(dt->d_name, &sbuf) < 0)
            continue;
        if (dt->d_name[0] == '.')
            continue; //过滤掉隐藏文件

        memset(buf, MAX_BUFFER_SIZE, 0);//清空缓冲区

        //先组合权限
        const char* purview = statbuf_get_purview(&sbuf);
        int offset = 0;
        offset += sprintf(buf, "%s", purview); //drwxr-xr-x

        offset += sprintf(buf + offset, "%3d %-8d %-8d %8u ", sbuf.st_nlink, 
                sbuf.st_uid, sbuf.st_gid, sbuf.st_size);

        //后组合时间
        const char* pdate = statbuf_get_date(&sbuf);
        offset += sprintf(buf + offset, "%s ", pdate);
        sprintf(buf + offset, "%s\r\n", dt->d_name);

        //发送数据
        send(sess->data_fd, buf, strlen(buf), 0);
    }
}

//调用do_list之间已经确定是port(主动)还是pasv(被动)了
static void do_list(session_t* sess)
{
    //1. 建立数据连接
    if (get_transfer_fd(sess) == 0)
        return;//建立数据连接失败

    //2. 回复150
    ftp_reply(sess, FTP_DATACONN ,"Here comes the directory listing.");

    //3. 显示列表
    list_common(sess);

    //4. 关闭连接
    close(sess->data_fd);
    sess->data_fd = -1;

    //5. 回复226
    ftp_reply(sess, FTP_TRANSFEROK, "Directory send OK.");
}











