#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "poll.h"
#include "sys/select.h"
#include "sys/time.h"
#include "linux/ioctl.h"

int main(int argc, char *argv[])
{
    int fd;
    int ret = 0;
    char *filename;
    unsigned char data;
    struct pollfd fds;
    fd_set readfds;
    struct timeval timeout;

    if(argc != 2){
        printf("error usage\r\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename, O_RDWR | O_NONBLOCK);
    if(fd < 0){
        printf("can not open file %s\r\n", filename);
        return -1;
    }

    while(1){
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
        ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
        switch (ret)
        {
        case 0://超时
            printf("timeout\r\n");
            break;
        
        case -1: //错误
        break;

        default: //可以读取数据
            if(FD_ISSET(fd, &readfds)){
                ret = read(fd, &data, sizeof(data));
                if(ret < 0){
                    //读取错误
                }else{
                    if(data){
                        printf("key value is %d\r\n", data);
                    }
                }
            }
            break;
        }
    }

    close(fd);
    return ret;
}