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
#include <stdbool.h>	//boolean
#include <time.h>	//time()
#include "./WLGenerator.h"


	struct aiocb *aiocbp[32];

static uint64_t utime_calculator(struct timeval *s, struct timeval *e){
 	long sec, usec;
    	
	//print_log("start sec:%ld, usec:%ld\n", s->tv_sec, s->tv_usec);
 	//print_log("end sec:%ld, usec:%ld\n", e->tv_sec, e->tv_usec);
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

//static void io_completion_handler(sigval_t sigval){
static void io_completion_handler(struct aiocb *aio){
	int rtn;
	struct aiocb *aiocbp;
	int reqID;
	int i=1;
	
	aiocbp = aio;
	//aiocbp = (struct aiocb *)sigval.sival_ptr;
	//reqID = (int)sigval.sival_int;
	while(1){ 
	    if(aio_error(aiocbp) == 0 ){
		rtn = aio_return(aiocbp);
		print_log("\n %s : IO succeeded : reqID=%d, rtn=%d\n", __func__, reqID, rtn);
		break;
	    }else{
		print_log("%s : IO failed : reqID=%d, rtn=%d", __func__, reqID, rtn);
	    }
	}

	print_log("%s : reqID=%d\n", __func__, reqID);
}

static void io_initialize(struct aiocb * aiocbp, int fd, char *buf, 
	unsigned int buflen, unsigned long long offset, int reqID){
 
	aiocbp->aio_fildes = fd;
	aiocbp->aio_buf = buf;
	aiocbp->aio_nbytes = buflen;
	aiocbp->aio_offset = offset;

	/*aiocbp->aio_sigevent.sigev_notify = SIGEV_THREAD;
	aiocbp->aio_sigevent.sigev_notify_function = io_completion_handler;
	aiocbp->aio_sigevent.sigev_notify_attributes = NULL;
	aiocbp->aio_sigevent.sigev_value.sival_ptr = aiocbp;
	aiocbp->aio_sigevent.sigev_value.sival_int = reqID;*/
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

static void fill_buffer(char *buf, int size){
    unsigned int perSize;
    int rNum;
    int i=0;
	
    while(size){
	rNum = rand();
	perSize = sizeof(rNum);
	if(perSize > size)
	    perSize = size;
	memcpy(buf, &rNum, perSize);
	size -= perSize;
	buf += perSize;
    }
}

static int mem_allocation(char **buf, int reqSize, bool align){
	int alignedReqSize;
	int i;
	
	if(align){
	    alignedReqSize = (reqSize+SECTOR_SIZE-1)/SECTOR_SIZE*SECTOR_SIZE;
	    print_log("%s : reqSize:%d, alignedReqSize:%d, align:%d\n",
		    __func__, reqSize, alignedReqSize, align);
	    if( posix_memalign((void **)buf, SECTOR_SIZE, alignedReqSize) != 0){
		print_log("%s : buffer allocation failed\n", __func__);
		exit(1);
	    }
	}else{
	    if(	(*buf = (char *)malloc(reqSize)) == NULL ){
		print_log("%s : buffer allocation failed\n", __func__);
		exit(1);
	    }
	}
	if(align)
	    return alignedReqSize;
	else
	    return reqSize;
}

static int get_rand_offset(int align){
    	int randNum;
	if(align){
	    randNum = (rand()%MAX_FILE_SIZE + 1)/SECTOR_SIZE*SECTOR_SIZE;
	}
	else
	    randNum = rand()%MAX_FILE_SIZE;
        
	print_log("%s : randNum : %d\n",__func__, randNum);
	return randNum;
}

void main(int argc, char *argv[]){
	int fd;
	int fd2;

	int rtn;
	struct timeval stime, etime;
	uint64_t ttime;

	//struct aiocb *aiocbp[32];
	int status;

	char resultBuf[100]={""};;

	int i;
	int loop;

	char *nBuf;
	int tmpReqSize;
	int reqSize;
	int offset;
	
	//init random seed
	srand(time(NULL));

	if( (fd = open("/dev/sda", O_CREAT|O_RDWR|O_DIRECT, 0666)) == -1){
	//if( (fd = open("./test.txt", O_CREAT|O_RDWR|O_DIRECT, 0666)) == -1){
	    print_log("%s : File open error : %d\n", __func__, fd);
	    exit(1);
	}
	if( (fd2 = open("./result.txt", O_CREAT|O_RDWR, 0666)) == -1){
	    print_log("%s : File open error : %d\n", __func__, fd);
	    exit(1);
	}

	//for(tmpReqSize=512; tmpReqSize<=1*1024*1024; tmpReqSize*=2){
	for(tmpReqSize=1*1024; tmpReqSize<256*1024; tmpReqSize+=1*1024){
	    
	    reqSize = mem_allocation(&nBuf, tmpReqSize, true);
	    
	    for(loop=0; loop<100; loop++){
		fill_buffer(nBuf, reqSize);

		//for(i=0; i<reqSize; i++)
		//print_log("%d", nBuf[i]);
		//print_log("\n");

		//offset = get_rand_offset(true);
		lseek(fd, 0, SEEK_SET); 
		//lseek(fd, offset, SEEK_SET); 

		gettimeofday(&stime, NULL);
		printf("WRITE : %d\n", write(fd, nBuf, reqSize));
		fsync(fd);
		gettimeofday(&etime, NULL);
		ttime = utime_calculator(&stime, &etime);	    
		print_log("%s : total elapse time : %lld\n", __func__, ttime);

		sprintf(resultBuf, "%10d, %10d, %20lld\n", reqSize, loop, ttime);
		lseek(fd, sizeof(resultBuf), SEEK_SET);
		//printf("choki: %s\n", resultBuf);
		write(fd2, resultBuf, sizeof(resultBuf));
		//printf("READ : %d\n", write(fd, nBuf, 512));
		//usleep(7000);
	    }
	    free(nBuf);
	}
	close(fd);
	close(fd2);

	/*
	for(i=0; i<30; i++){
	    aiocbp[i] = calloc(1, sizeof(struct aiocb));
	    buf[i] = calloc(1, BUF_SIZE+1);
	    memcpy(buf[i], "avasdfasdfas", 6);
	    //io_initialize(aiocbp[i], fd, buf[i], BUF_SIZE, i*6, i);
	    io_initialize(aiocbp[i], fd, buf[i], sizeof(buf), i*6, i);
	    gettimeofday(&stime, NULL);

	    io_enqueue(DIR_READ, aiocbp[i]);
	    io_completion_handler(aiocbp[i]);
 
 	    gettimeofday(&etime, NULL);
	    ttime = utime_calculator(&stime, &etime);	    
            print_log("%s : total elapse time : %lld\n", __func__, ttime);
	}*/



}
