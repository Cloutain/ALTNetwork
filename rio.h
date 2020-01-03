#ifndef _RIO_H
#define _RIO_H

#include "netpro.h"

#define RIO_BUFSIZE 2048

typedef struct {
    int rio_fd;
    int rio_cnt;
    char *rio_bufptr;
    char rio_buf[RIO_BUFSIZE];
} rio_t;


/*!
 @function   rio_readinitb
 @abstract   初始化一个rio_t类型的变量，绑定fd
 */
void rio_readinitb(rio_t *rp, int fd);

/*!
 @function   rio_buf_readn
 @abstract   读取rp关联的文件描述符中的n个字节到存储器位置usrbuf，被系统调用中断自动重启
 @discussion ...
 @param      rp:        rio_t对象
 @param      usrbuf:    指向存储数据的区域
 @param      n:         读取的字节数
 @result     >0 :实际读取的字节数
 =0 :读取到文件末尾(EOF)或者遇到中断
 =-1:读取错误
 */
ssize_t rio_buf_readn(rio_t *rp, void *usrbuf, size_t n);


/*!
 @function   rio_write
 @abstract   往描述符fd写入存储器位置usrbuf开始的n个字节，被系统调用中断自动重启
 @discussion ...
 @param      fd:文件描述符
 @param      usrbuf:指向存储数据的区域
 @param      n:写入的字节数
 @result     >0 :实际读取的字节数
             =0 :读取到文件末尾(EOF)或者遇到中断
             =-1:读取错误
 */
ssize_t rio_write(int fd, void *usrbuf, size_t n);

#endif
