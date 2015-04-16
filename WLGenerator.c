#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>	//exit
#include <sys/time.h>	//gettimeofday
#include <stdint.h> 	//uint64_t
#include <aio.h>	//posixaio
#include <errno.h>
#include <string.h> 	//memcpy

#define BUF_SIZE 125

#define DEBUG_MODE

#ifdef DEBUG_MODE
#define print_log(...) \
    	do{ printf(__VA_ARGS__); }while(0)
#else
#define print_log
#endif


typedef enum{
    DIR_READ=0,
    DIR_WRITE
}IO_DIR;

/*
typedef struct ioRequest_t{
	int 	
}ioRequest;
*/

static uint64_t utime_calculator(struct timeval *s, struct timeval *e){
 	long sec, usec;
    	
	print_log("start sec:%ld, usec:%ld\n", s->tv_sec, s->tv_usec);
 	print_log("end sec:%ld, usec:%ld\n", e->tv_sec, e->tv_usec);
	sec = e->tv_sec - s->tv_sec;
	usec = e->tv_usec - s->tv_usec;
	if(sec < 0){
	    print_log("%s : unbelievable thing happended, time warp\n", __func__);
	    exit(1);
	}
	if(usec < 0){
	    sec--;
	    usec += 1000000;
	}
	return sec*1000000ULL + usec;
}

static void io_completion_handler(sigval_t sigval){
	int rtn;
	struct aiocb *aiocbp;
	aiocbp = (struct aiocb *)sigval.sival_ptr;
	    
	if(aio_error(aiocbp) == 0 ){
	    rtn = aio_return(aiocbp);
	    print_log("%s : IO succeeded : %d\n", __func__, rtn);
	}
}

static void io_initialize(struct aiocb * aiocbp, int fd, char *buf, 
	unsigned int buflen, unsigned long long offset){
 
	aiocbp->aio_fildes = fd;
	aiocbp->aio_buf = buf;
	aiocbp->aio_nbytes = buflen;
	aiocbp->aio_offset = offset;

	aiocbp->aio_sigevent.sigev_notify = SIGEV_THREAD;
	aiocbp->aio_sigevent.sigev_notify_function = io_completion_handler;
	aiocbp->aio_sigevent.sigev_notify_attributes = NULL;
	aiocbp->aio_sigevent.sigev_value.sival_ptr = aiocbp;
}

static int io_enqueue(IO_DIR direction, struct aiocb *aiocbp){
    	int rtn;

    	if(direction == DIR_READ)
	    rtn = aio_read(aiocbp);
	else
	    rtn = aio_write(aiocbp);

	if(rtn){
	    print_log("%s : aio Read/Write error, Direction:%d, rtn:%d\n",
		    __func__, direction, rtn);
	    exit(1);
	}else{
	    print_log("%s : IO_DIR : %d\n", __func__, rtn);
	}
	return rtn;
}


void main(int argc, char *argv[]){
    	int fd;
	int rtn;
	struct timeval stime, etime;
	uint64_t ttime;

	struct aiocb *aiocbp;
	int status;

	char *buf;

	//if( (fd = open("/dev/sda", O_CREAT|O_RDWR, 0666)) == -1){
	if( (fd = open("./test.txt", O_CREAT|O_RDWR|O_DIRECT, 0666)) == -1){
	    print_log("%s : File open error : %d\n", __func__, fd);
	    exit(1);
	}
	/*int minSize = 512;
	int maxSize = 512*64;
	int try = 10;

	for (int size=minSize; size<=maxSize; size*=2){
	    for (int try=10; try>0; try--){
	    }
	}*/
	//io_initialize(aiocbp, fd, buf, size, offset);
	aiocbp = malloc(sizeof(struct aiocb));
	buf = malloc(BUF_SIZE+1);
	memcpy(buf, "absadfsdf", 5);

	io_initialize(aiocbp, fd, buf, BUF_SIZE, 0);
	io_enqueue(DIR_WRITE, aiocbp);
	gettimeofday(&stime, NULL);

	while(1)
	{
	    usleep(100);
	}

	/*gettimeofday(&etime, NULL);
	
	ttime = utime_calculator(&stime, &etime);	    
	print_log("%s : total elapse time : %lld\n", __func__, ttime);
	*/
	
}
