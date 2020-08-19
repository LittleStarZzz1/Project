#include "ftp_proto.h"
#include "ftp_codes.h"
#include "str.h"
#include "sysutil.h"
#include "privsock.h"
#include "tunable.h"

session_t* p_sess;

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
static void do_pasv(session_t* sess);
static void do_list(session_t* sess);
static void do_cwd(session_t* sess);
static void do_mkd(session_t* sess);
static void do_rmd(session_t* sess);
static void do_rnfr(session_t* sess);
static void do_rnto(session_t* sess);
static void do_dele(session_t* sess);
static void do_size(session_t* sess);
static void do_stor(session_t* sess);
static void do_retr(session_t* sess);
static void do_rest(session_t* sess);

//命令映射表
static ftpcmd_t ctrl_cmds[] =
{
    {"USER", do_user},
    {"PASS", do_pass},
    {"SYST", do_syst},
    {"FEAT", do_feat},
    {"PWD",  do_pwd},
    {"TYPE", do_type},
    {"PORT", do_port},
    {"PASV", do_pasv},
    {"LIST", do_list},
    {"CWD",  do_cwd},
    {"MKD",  do_mkd},
    {"RMD",  do_rmd},
    {"RNFR", do_rnfr},
    {"RNTO", do_rnto},
    {"DELE", do_dele},
    {"SIZE", do_size},
    {"STOR", do_stor},
    {"RETR", do_retr},
    {"REST", do_rest}
};

//////////////////////////////////////////////////////////////////////

void handle_ctrl_timeout(int sig)
{
    shutdown(p_sess->ctrl_fd, SHUT_RD);
    ftp_reply(p_sess, FTP_IDLE_TIMEOUT, "Timeout.");
    shutdown(p_sess->ctrl_fd, SHUT_WR);
    exit(EXIT_SUCCESS);
   
    // close(p_sess->ctrl_fd);
}
//数据连接空闲断开闹钟
void start_cmdio_alarm()
{
    if(tunable_idle_session_timeout > 0)
    {
        signal(SIGALRM, handle_ctrl_timeout);
        alarm(tunable_idle_session_timeout); //启动闹钟
    }
}

void start_data_alarm();

void handle_data_timeout(int sig)
{
    //在每次传输或下载指定字节之后p_sess->data_process会被置为1
    if(!p_sess->data_process)
    {
        ftp_reply(p_sess, FTP_DATA_TIMEOUT, "Data timeout. Reconnect Sorry.");
        exit(EXIT_FAILURE);
    }
    //重新设置闹钟
    //当上传或下载结束之后p_sess->data_process就不会被置为1
    //也就是说上传下载结束之后
    //数据连接的空闲断开闹钟又开始重新计时
    p_sess->data_process = 0;
    start_data_alarm();
}
void start_data_alarm()
{
    if(tunable_data_connection_timeout > 0)
    {
        signal(SIGALRM, handle_data_timeout);
        alarm(tunable_data_connection_timeout);
   }
    else if(tunable_idle_session_timeout > 0)
        alarm(0);
}


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

        //开启控制连接空闲断开的闹钟
        start_cmdio_alarm();

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
    //char ip[16] = "192.168.188.131";//服务器IP
    char ip[16] = {0};
    getLocalip(ip);

    priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_LISTEN);

    /*sess->pasv_listen_fd = tcp_server(ip, 0);//端口号为0表示生成临时端口

    struct sockaddr_in address;
    socklen_t socklen = sizeof(struct sockaddr);
    if (getsockname(sess->pasv_listen_fd, (struct sockaddr*)&address, &socklen) < 0)
        ERR_EXIT("setsockname error~~~\n");
    */
    uint16_t port = (uint16_t)priv_sock_get_int(sess->child_fd);

    int v[4] = {0};
    sscanf(ip, "%u.%u.%u.%u", &v[0], &v[1], &v[2], &v[3]);
    char msg[MAX_BUFFER_SIZE] = {0};
    sprintf(msg, "Entering Passive Mode (%u,%u,%u,%u,%u,%u).",
            v[0], v[1], v[2], v[3], port >> 8, port & 0xff);

    ftp_reply(sess, FTP_PASVOK, msg);
}

////////////////////////////////////////////////////////////////////////////////
int pasv_active(session_t* sess);

//主动模式是否被激活
int port_active(session_t* sess)
{
    //sess->port_addr != NULL表示主动模式被激活
    if (sess->port_addr)
    {
        //被动模式也被激活
        if (pasv_active(sess))
        {
            //表示出错
            fprintf(stderr, "both port and pasv are active~~\n");
            exit(EXIT_FAILURE);
        }
        return 1;//走到这里表示只有主动模式被激活
    }
    return 0;
}

//被动模式是否被激活
int pasv_active(session_t* sess)
{
    priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_ACTIVE);
    //sess->pasv_listen_fd != -1表示被动模式被激活
    if (priv_sock_get_int(sess->child_fd))
    {
        //主动模式也被激活
        if (port_active(sess))
        {
            //出错了
            fprintf(stderr, "both pasv and port are active~~\n");
            exit(EXIT_FAILURE);
        }
        return 1;
    }
    return 0;
}

int get_port_fd(session_t* sess)
{
    int ret = 1;
    //ftp服务进程(即子进程)向nobody进程(即父进程)发起通信
    priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_GET_DATA_SOCK);
    uint16_t port = ntohs(sess->port_addr->sin_port);
    char* ip = inet_ntoa(sess->port_addr->sin_addr);

    priv_sock_send_int(sess->child_fd, (int)port);
    priv_sock_send_buf(sess->child_fd, ip, strlen(ip));

    char res = priv_sock_get_result(sess->child_fd);
    if (res == PRIV_SOCK_RESULT_BAD)
        ret = 0;
    else if (res == PRIV_SOCK_RESULT_OK)
        sess->data_fd = priv_sock_recv_fd(sess->child_fd);
    return ret;
}

int get_pasv_fd(session_t* sess)
{
    int ret = 1;
    priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_ACCEPT);
    char res = priv_sock_get_result(sess->child_fd);
    if(res == PRIV_SOCK_RESULT_BAD)
        ret = 0;
    else if(res == PRIV_SOCK_RESULT_OK)
        sess->data_fd = priv_sock_recv_fd(sess->child_fd);

    return ret;
}

int get_transfer_fd(session_t* sess)
{
    //主动模式和被动模式都没有被激活 
    if (!port_active(sess) && !pasv_active(sess))
    {
        ftp_reply(sess, FTP_BADSENDCONN,"Use PORT or PASV first.");
        return 0;
    }

    int ret = 1;
    //port,主动模式被激活
    if (port_active(sess))
    {

        if (!get_port_fd(sess))
            ret = 0;

        /*int fd = tcp_client();

        if (connect(fd, (struct sockaddr*)sess->port_addr,sizeof(struct sockaddr)) < 0)
        {
            ret = 0;
        }
        else
        {
            sess->data_fd = fd;
        }*/ 
    }

    //pasv, 表示被动模式被激活
    if (pasv_active(sess))
    {
        /*int fd = accept(sess->pasv_listen_fd, NULL, NULL);
        if (fd < 0)
            ret = 0;
        else
        {
            close(sess->pasv_listen_fd);
            sess->pasv_listen_fd = -1;
            sess->data_fd = fd;
        }*/
        if (!get_pasv_fd(sess))
            ret = 0;
    }

    if (sess->port_addr)
    {
        free(sess->port_addr);
        sess->port_addr = NULL;
    }

    //数据连接建立成功, 开启数据连接空闲断开的闹钟
    if (ret)
        start_data_alarm();
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

        offset += sprintf(buf + offset, "%3d %-8d %-8d %8lld ", sbuf.st_nlink, 
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
    if (get_transfer_fd(sess) == 0)//担当被动和主动的连接建立
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

static void do_cwd(session_t* sess)
{
    if (chdir(sess->arg) < 0)
    {
        ftp_reply(sess, FTP_NOPERM, "Failed to change directory.");
        return;
    }
    ftp_reply(sess, FTP_CWDOK, "Directory successfully changed.");
}

static void do_mkd(session_t* sess)
{
    if (mkdir(sess->arg, 0777) < 0)
    {
        ftp_reply(sess, FTP_NOPERM, "Create directory operation failed.");
        return;
    }

    char text[1024] = {0};
    sprintf(text, "\"%s\" create", sess->arg);
    ftp_reply(sess, FTP_MKDIROK, text);
}

static void do_rmd(session_t* sess)
{
    if (rmdir(sess->arg) < 0)
    {
        ftp_reply(sess, FTP_FILEFAIL, "Remove directory operation failed.");
        return;
    }

    //250 Remove directory operation successful.
    ftp_reply(sess, FTP_RMDIROK, "Remove directory operation successful.");
}

static void do_rnfr(session_t* sess)
{
   sess->rnfr_name = (char*)malloc(strlen(sess->arg) + 1);
   memset(sess->rnfr_name, 0, strlen(sess->arg) + 1);
   strcpy(sess->rnfr_name, sess->arg);
   ftp_reply(sess, FTP_RNFROK, "Ready for RNTO.");
}
static void do_rnto(session_t* sess)
{
    if (sess->rnfr_name == NULL)
    {
        //503 RNFR required first.
        ftp_reply(sess, FTP_NEEDRNFR, "RNFR required first.");
        return;
    }

    if (rename(sess->rnfr_name, sess->arg) < 0)
    {
        ftp_reply(sess, FTP_NOPERM, "Rename failed.");
        return;
    }

    free(sess->rnfr_name);
    sess->rnfr_name = NULL;

    ftp_reply(sess, FTP_RENAMEOK, "Rename successful.");
}

static void do_dele(session_t* sess)
{
    if (unlink(sess->arg) < 0)
    {
        //550 Delete operation failed.
        ftp_reply(sess, FTP_NOPERM, "Delete operation failed.");
        return;
    }
    //250 Delete operation successful.
    ftp_reply(sess, FTP_DELEOK, "Delete operation successful.");
}

static void do_size(session_t* sess)
{
    struct stat sbuf;
    if (stat(sess->arg, &sbuf) < 0)
    {
         ftp_reply(sess, FTP_FILEFAIL, "SIZE operation failed.");
         return;
    }
    if (!S_ISREG(sbuf.st_mode))//不是普通文件
    {
        ftp_reply(sess, FTP_FILEFAIL, "Could not get file size.");
        return;
    }

    char text[1024] = {0};
    sprintf(text, "%lld", (long long)sbuf.st_size);
    ftp_reply(sess, FTP_SIZEOK, text);
}

//限速
static void limit_rate(session_t* sess, int bytes_transfered, int isupload)
{
    long cur_sec = get_time_sec();
    long cur_usec = get_time_usec();

    double pass_time = (double)(cur_sec - sess->transfer_start_sec);
    pass_time += (double)((cur_usec - sess->transfer_start_usec) 
            / (double)1000000);//1 s = 10^6 usec

    //计算当前的传输速度, 当前传输的字节数 / 当前流逝过的时间
    unsigned int cur_rate = (unsigned int)((double)bytes_transfered / pass_time);

    double rate_ratio;
    if (isupload)
    {
        if(cur_rate <= sess->upload_max_rate)
        {
            //当前传输速度小于等于最大传输速度
            //不需要限速, 重新记录起始时间
            sess->transfer_start_sec = cur_sec;
            sess->transfer_start_usec = cur_usec;
            return;
        }

        rate_ratio = cur_rate / sess->upload_max_rate;
    }
    else
    {
        if(cur_rate <= sess->download_max_rate)
        {
            sess->transfer_start_sec = cur_sec;
            sess->transfer_start_usec = cur_usec;
            return;
        }

        rate_ratio = cur_rate / sess->download_max_rate;
    }
    
    //需要睡眠的时间
    double sleep_time = (rate_ratio - 1) * pass_time;
    nano_sleep(sleep_time);

    //重新记录起始传送时间
    sess->transfer_start_sec = get_time_sec();
    sess->transfer_start_usec = get_time_usec();
}

//上传
static void do_stor(session_t* sess)
{
    //建立数据连接
    if (get_transfer_fd(sess) == 0)
        return;

    int fd = open(sess->arg, O_CREAT | O_WRONLY, 0755);
    if (fd == -1)
    {
        ftp_reply(sess, FTP_FILEFAIL, "Failed to open file.");
        return;
    }
    ftp_reply(sess, FTP_DATACONN,"Ok to send data.");

    //断点续传的实现 
    //记录偏移量
    long long offset = sess->restart_pos;
    sess->restart_pos = 0;
    if (lseek(fd, offset, SEEK_SET) < 0)
    {
        ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file.");
        return;
    }

    char buf[MAX_BUFFER_SIZE] = {0};
    int ret;

    //登记时间
    sess->transfer_start_sec = get_time_sec(); 
    sess->transfer_start_usec = get_time_usec();

    while (1)
    {
        ret = recv(sess->data_fd, buf, MAX_BUFFER_SIZE, 0);
        if (ret == -1)
        {
            ftp_reply(sess, FTP_BADSENDFILE, "Failure reading from local file.");
            break;
        }

        if (ret == 0)
        {
            //226 Transfer complete.
            ftp_reply(sess, FTP_TRANSFEROK, "Transfer complete.");
            return;
        }

        //设置空闲断开状态
        sess->data_process = 1;

        //限速
        if(sess->upload_max_rate != 0)
            limit_rate(sess, ret, 1);
        if (write(fd, buf, ret) != ret)
        {
            ftp_reply(sess, FTP_BADSENDFILE, "Failure writting to network stream.");
            break;
        }
    }
    close(fd);
    close(sess->data_fd);
    sess->data_fd = -1;
    //重新启动控制连接断开
    start_cmdio_alarm();
}

//下载
static void do_retr(session_t* sess)
{
    //先建立数据连接
    if (get_transfer_fd(sess) == 0)
        return;

    int fd = open(sess->arg, O_RDONLY);
    if (fd == -1)
    {
        ftp_reply(sess, FTP_FILEFAIL, "Failed to open file.");;
        return;
    }
    
    //记录文件信息
    struct stat sbuf;
    fstat(fd, &sbuf);

    long long offset = sess->restart_pos;
    sess->restart_pos = 0;
    if (offset >= sbuf.st_size)
    {
        ftp_reply(sess, FTP_TRANSFEROK, "Transfer complete.");
    }

    else
    {
        char msg[MAX_BUFFER_SIZE] = {0};
        if (sess->is_ascii)
            sprintf(msg, "Opening ASCII mode data connection for %s (%lld bytes).", 
                    sess->arg, (long long)sbuf.st_size);
        else
            sprintf(msg, "Opening BINARY mode data connection for %s (%lld bytes).", 
                    sess->arg, (long long)sbuf.st_size);

        //150 Opening ASCII mode data connection for /home/bss/mytt/abc/test.cpp (70 bytes).
        ftp_reply(sess, FTP_DATACONN, msg);

        if (lseek(fd, offset, SEEK_SET) < 0)
        {
            ftp_reply(sess, FTP_UPLOADFAIL, "Could not create file.");
            return;
        }

        char buf[MAX_BUFFER_SIZE] = {0};
        long long read_total_bytes = (long long)sbuf.st_size - offset;
        int read_count = 0;
        int ret;

        //开始下载之前登记开始的时间
        sess->transfer_start_sec = get_time_sec();
        sess->transfer_start_usec = get_time_usec();

        while (1)
        {
            read_count = read_total_bytes>MAX_BUFFER_SIZE?MAX_BUFFER_SIZE:read_total_bytes;
            ret = read(fd, buf, read_count);

            if (ret == -1 || ret != read_count)
            {
                ftp_reply(sess, FTP_BADSENDFILE, "Failure reading from local file.");
                break;
            }

            if (ret == 0)
            {
                // 226 Transfer complete.
                ftp_reply(sess, FTP_TRANSFEROK, "Transfer complete.");
                break;
            }
            
            //设置空闲断开状态
            sess->data_process = 1;
            //限速
            if (sess->download_max_rate != 0)
                limit_rate(sess, read_count, 0);
            if (send(sess->data_fd, buf, ret, 0) != ret)
            {
                ftp_reply(sess,FTP_BADSENDFILE,"Failure writting to network stream.");
                break;
            }
            read_total_bytes -= read_count;
        }
    }
    close(fd);
    close(sess->data_fd);
    sess->data_fd = -1;

    //重新启动控制连接断开
    start_cmdio_alarm();
}

//针对上传和下载均适用,保存上次传输的位置
static void do_rest(session_t* sess)
{
    sess->restart_pos = atoll(sess->arg);
    //350 Resatrt position accepted (1612906496).
    char msg[MAX_BUFFER_SIZE] = {0};
    sprintf(msg, "Restart position accepted (%lld).", sess->restart_pos);

    ftp_reply(sess, FTP_RESTOK, msg);
}







