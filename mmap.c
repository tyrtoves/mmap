#include<stdio.h>
#include<semaphore.h>
#include<unistd.h>
#include<sys/types.h>
#include<stdlib.h>
#include<time.h> 
#include<sys/stat.h>
#include<string.h>
#include<fcntl.h>
#include <sys/mman.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <limits.h>


#define SIZE 256
#define NUMBER 16 

struct shmstruct {
	sem_t mutex; 
	sem_t nempty;
	sem_t nstored;
	int nput; 
	long msgoff[NUMBER];
	char msgdata[SIZE * NUMBER]; 

};

int main(int argc, char** argv) {

	int fd, index, lastnoverflow, temp;
	long int offset;
   	struct shmstruct *ptr;
   	if (argc != 2) {
   		printf("Wrong number of arg\n");
   		exit(0);
   	}

	shm_unlink("first_shm"); 

    fd = shm_open("first_shm", O_RDWR | O_CREAT | O_EXCL, 0);

	ptr = mmap(NULL, sizeof(struct shmstruct), PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0);
	ftruncate(fd, sizeof(struct shmstruct));
	close(fd);
	
	int i;
	for (i = 0; i < NUMBER; ++i)
		ptr -> msgoff[i] = i * SIZE;
	
	sem_init(&ptr -> mutex, 1, 1);
	sem_init(&ptr -> nempty, 1, NUMBER);
	sem_init(&ptr -> nstored, 1, 0);
	
	srand (time(NULL));
  	struct timeval this_time, this_time_end, res;
  	
	int n = atoi(argv[1]);
	printf("%d\n", n);
	int count = 0;
	
	int pid=fork();
	if ( pid < 0 ) {
		perror("FORK");
		exit(0);
	}	
	else if ( pid == 0 ) {//записываем
		int j = 0;		
		char mesg[SIZE];
		long offset;
		
		pid_t pid = getpid();
		for (j = 0; j < n; ++j) {
		
		 	snprintf(mesg, SIZE, "pid %ld message %d", (long) pid, j);
	   		sem_wait(&ptr -> nempty);
			sem_wait(&ptr -> mutex);
			offset = ptr -> msgoff[ptr->nput];
			if (++(ptr -> nput) >= NUMBER)
				ptr -> nput = 0; 
			sem_post(&ptr->mutex);
			strcpy(&ptr->msgdata[offset], mesg);
			sem_post(&ptr->nstored);
		}
		
		exit(0);	
	}
	else {//считываем
		index = 0;
		gettimeofday(&this_time, 0);
		for (;count < n;++count) {
			sem_wait(&ptr -> nstored);
   			sem_wait(&ptr -> mutex);
       		offset = ptr -> msgoff[index];
       		printf("%s\n", &ptr -> msgdata[offset]);
			if (++index >= NUMBER)
				index = 0;
       		sem_post(&ptr->mutex);
       		sem_post(&ptr->nempty);
       		
		}
		gettimeofday(&this_time_end, 0);
		res.tv_sec= this_time_end.tv_sec -this_time.tv_sec;
		res.tv_usec=this_time_end.tv_usec-this_time.tv_usec;
		if(res.tv_usec < 0) {
			res.tv_sec--;
			res.tv_usec+=1000000;
		}
		printf("%u,%u sec \n", res.tv_sec, res.tv_usec); 
		exit(0);		
	}
	return 0;
}
