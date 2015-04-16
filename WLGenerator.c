#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>	//exit
#include <sys/time.h>	//gettimeofday
#include <stdint.h> 	//uint64_t

#define BUF_SIZE 125

static uint64_t utime_calculator(struct timeval *s, struct timeval *e){
 	long sec, usec;
    	
	printf("start sec:%ld, usec:%ld\n", s->tv_sec, s->tv_usec);
 	printf("end sec:%ld, usec:%ld\n", e->tv_sec, e->tv_usec);
	sec = e->tv_sec - s->tv_sec;
	usec = e->tv_usec - s->tv_usec;
	if(sec < 0){
	    printf("%s : unbelievable thing happended, time warp\n", __func__);
	    exit(1);
	}
	if(usec < 0){
	    sec--;
	    usec += 1000000;
	}
	return sec*1000000ULL + usec;
}

void main(int argc, char *argv[]){
    	int fd;
	int rtn;
	struct timeval stime, etime;
	uint64_t ttime;
	
	char buf1[BUF_SIZE] = "This is for test1";
	char buf2[BUF_SIZE] = {0,};

	if( (fd = open("/dev/sda", O_CREAT|O_RDWR, 0666)) == -1){
	    printf("%s : File open error : %d\n", __func__, fd);
	    exit(1);
	}

	pwrite(fd, buf1, BUF_SIZE, 0);
	if(rtn == -1){
	    printf("%s : write error : %d\n", __func__, fd);
	}
	gettimeofday(&stime, NULL);

	pread(fd, buf2, BUF_SIZE, 0);
	if(rtn == -1){
	    printf("%s : read error : %d\n", __func__, fd);
	}
	printf("This is the read result : %s\n", buf2);
	gettimeofday(&etime, NULL);
	
	ttime = utime_calculator(&stime, &etime);	    
	printf("%s : total elapse time : %lld\n", __func__, ttime);
}
