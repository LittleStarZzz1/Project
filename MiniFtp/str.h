#ifndef _STR_H
#define _STR_H

#include "common.h"

//命令字符串解析模块
void str_trim_crlf(char* str);

void str_split(const char* str, char* left, char* right, char c);

#endif //_STR_H
