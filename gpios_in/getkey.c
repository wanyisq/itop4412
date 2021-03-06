#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc,char **argv)
{
	int fd;
	
	if(argc!=2){
		printf("arvc is 2:\nargv1 is device_node\n");
	}
	
	if((fd = open(argv[1], O_RDWR|O_NOCTTY|O_NDELAY))<0){
		printf("open %s failed\n",argv[1]);   
		return -1;
	}
	else{
		printf("open %s ok\n",argv[1]); 
		printf("key value is %d\n",ioctl(fd,0,0));		
    }
	close(fd);
	return 0;
}
