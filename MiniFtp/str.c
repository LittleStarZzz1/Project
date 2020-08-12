#include "str.h"

void str_trim_crlf(char* str)
{
    assert(str != NULL);

    char* str_end = str + (strlen(str) - 1);
    while (*str_end == '\n' || *str_end == '\r')
    {
        *str_end = '\0';
        str_end--;
    }
}

void str_split(const char* str, char* left, char* right, char c)
{
    assert(str != NULL);

    char* pos = strchr(str, c);
    if (pos == NULL)//没有找到间隔字符
    {
        strcpy(left, str);
    }
    else
    {
        strncpy(left, str, pos - str);
        strcpy(right, pos + 1);
    }
}

void str_upper(char* str)
{
    while (*str != '\0')
    {
        if (*str <= 'z' && *str >= 'a')
            *str -= 32;
        str++;
    }
}


