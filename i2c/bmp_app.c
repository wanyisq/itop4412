#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int fd;
    char *filename;
    int ret = 0;
    long temp;

    if(argc != 2){
        printf("error usage\r\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename, O_RDWR);
    if(fd < 0){
        printf("can not open %s\r\n", filename);
        return -1;
    }

    while (1)
    {
        /* code */
        ret = read(fd, &temp, sizeof(temp));
        if(ret == 0){
            printf("read temperature is %d\r\n", temp);
        }

        usleep(200000);
    }

    close(fd);
    return 0;
}