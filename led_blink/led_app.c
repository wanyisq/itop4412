#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "linux/ioctl.h"

int main(int argc, char *argv[])
{
    int fd;
    int ret = 0;
    char *filename;
    unsigned char data;

    if(argc != 3){
        printf("error usage\r\n");
        return -1;
    }

    filename = argv[1];
    data = atoi(argv[2]);
    fd = open(filename, O_RDWR);
    if(fd < 0){
        printf("can not open file %s\r\n", filename);
        return -1;
    }

    while(1){
        ret = write(fd, &data, sizeof(data));
        if(ret < 0){
            return ret;
        }else{
            if(data){
                printf("led value %X\r\n", data);
            }
            return ret;
        }
    }

    close(fd);
    return ret;
}