#include<stdio.h>
#include<assert.h>
#include<string.h>
#include<pthread.h>
#include<malloc.h>
#include <time.h>
#include <sys/time.h>

#define MAX_COUNT 10
#define MAX_CHAR_COUNT 100
#define MAX_THREAD 512
struct block {
	int sIndex;
	int eIndex;
} *fBlocks;

int **count;
char psList[MAX_COUNT][MAX_CHAR_COUNT];
int numThread;
int numProc;
char logFileName[50];

pthread_mutex_t levelWaitMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  levelWaitCond = PTHREAD_COND_INITIALIZER;
int numThreadsInLevel=0;
//int finalCount[MAX_COUNT];

int *thrArray;
//int *offSet;
//pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

//following function have been copied from my last sem IP project
int timeval_subtract(struct timeval *result, struct timeval *t1, struct timeval *t2)
{
    long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
    result->tv_sec = diff / 1000000;
    result->tv_usec = diff % 1000000;

    return (diff<0);
}

int init( int psCount,int threadCount ) {
	int i=0;
	
	thrArray = (int *)malloc(sizeof(int) * threadCount);
	fBlocks=(struct block*)malloc( sizeof(struct block) * threadCount );

        count= (int **)calloc( psCount , sizeof(int *) );
	for(i=0;i<psCount;i++)
                count[i]=(int *) calloc(threadCount , sizeof(int) );
	
	for(i=0;i<threadCount;i++) { 
		thrArray[i]=i;
		(fBlocks+i)->sIndex=-1;
		(fBlocks+i)->eIndex=-1;
	}
		
	return 0;
}

void * getStat(void *arg) {
	int threadId = *(int *)arg;

//	printf("Thread Id : %d\n",threadId);
	int start = (fBlocks+threadId)->sIndex;
	int end = (fBlocks+threadId)->eIndex;
	if( start == -1 || end == -1)
		return;
//Read each ps from log file
//for each ps ,do findMatch and increment the count of index returned by it.
	FILE *fp ;
	char procName[50];
	fp = fopen(logFileName,"r");
	//printf("Block start : %d end : %d \n",start,end);
	int pIndex;
	fseek(fp,start,SEEK_SET);
	while( ftell(fp) != end ) {
//		printf("while\n");
		fscanf(fp,"%*s %*s %*s %[^\[]%*[^\n]\n",procName);
		pIndex = findMatch(procName);
		if(pIndex!=-1) {
//			pthread_mutex_lock(&mutex1);
			count[pIndex][threadId]++;
			//printf("Count----> : %d\n",count[pIndex][threadId]);
//			pthread_mutex_unlock(&mutex1);
//
		}
	}

	fclose(fp);
	return 0;
}

void printDetails() {
	int i = 0,j,tmp=0,numLogLines=0;
/*
	printf("========================================================\n");
	printf("Statistics\n");
	printf("========================================================\n");
	printf("Process Name\tCount\n");
*/	
	for(i=0;i<numProc;i++) {
		for(j=0;j<numThread;j++) {
			tmp+=count[i][j];
		}
		//printf("%-12s\t%4d\n",psList[i],tmp);

                printf("pName: %s count : %d\n",psList[i],tmp );
		numLogLines+=tmp;
		tmp=0;
	}

	printf("Total Number of loglines: %d\n",numLogLines);

}

void printRedDetails() {
        int i = 0,j,tmp=0,numLogLines=0;
/*
        printf("========================================================\n");
        printf("Statistics\n");
        printf("========================================================\n");
        printf("Process Name\tCount\n");
*/
        for(i=0;i<numProc;i++) {
                //for(j=0;j<numThread;j++) {
                //        tmp+=count[i][j];
                //}
                //printf("%-12s\t%4d\n",psList[i],tmp);

                printf("pName: %s count : %d\n",psList[i],count[i][0] );
                numLogLines+=count[i][0];
                tmp=0;
        }

        printf("Total Number of loglines: %d\n",numLogLines);

}


int findMatch(char *s) {

	int found=0;

	int i;
	for(i=0;i<numProc;i++) {
		if(strcmp(s,psList[i])==0)
			return i;
	}

	return -1;
} 

int getProcList(char *fileName) {
	FILE *fp;
	fp = fopen(fileName,"r");
	assert(fp);
	int i=0;
	while(fscanf(fp,"%[^\n]\n",psList[i++])==1);

/*	numProc=i-1;
	for(i=0;i<numProc;i++) {
		printf("%s\n",psList[i]);
	}
*/
	fclose(fp);
	return i-1;
}

int findBlockIndices(char *fileName, int threadCount ) {
//Fill in sIndex and eIndex of fBlocks[0..threadCount-1]

	FILE *fp;
	int size,blockSize,i,start,end,lineNum=0;
	char newLine;
	fp = fopen(fileName,"r");
	assert(fp);
	
	fseek(fp,0,SEEK_END);
	size = ftell(fp);
	assert(size!=0);
//	printf("Size of file is : %d\n",size);

	fseek(fp,0,SEEK_SET);

/*
	while(fscanf(fp,"%*[^\n]\n")==0)
		lineNum++;

	fseek(fp,0,SEEK_SET);
	printf("Total number of lines : %d \n",lineNum);	
//	blockSize = lineNum/threadCount;
	start=0;
	int j=0;
	int reminder=0;
	int tCount = threadCount;
	for(i=0;i<threadCount;i++) {

		reminder = lineNum%tCount;
		if( ( (float) reminder/tCount ) >= 0.5) {
			blockSize = lineNum/tCount + 1;	
		} else {
			blockSize = lineNum/tCount;
		}
		tCount--;
		lineNum = lineNum - blockSize;

		printf("Number of lines allocated for thread : %d is %d \n",i , blockSize);
		for(j=0;j<blockSize && fscanf(fp,"%*[^\n]\n")==0; j++);
		end=ftell(fp);
                (fBlocks+i)->sIndex = start;
                (fBlocks+i)->eIndex = end;
                start=end+1;
			
	}
*/

	start=0;
	end=0;
	int tc=threadCount;
	int remChars = size;
	for(i=0;i<threadCount && end < size;i++) {
		blockSize = remChars/tc;
		//printf("i : %d . blockSize : %d remChars : %d remThread : %d \n",i,blockSize,remChars,tc);
		if( (start+blockSize-1 ) <= size ) {
			if (fseek(fp, blockSize ,SEEK_CUR)!= 0) {
			//	printf("Fseek failed during iteration %d with block size : %d\n",i,blockSize);
			}
		} 
		//printf("Searching for newLine .... \n");
		do {
			newLine = fgetc(fp);	
		//	printf("Cur Char = %c \n",newLine);
		} while(newLine!='\n' && newLine!=EOF);
		end = ftell(fp);
/*
		if(newLine=='\n')
			//printf("NewLine found at : %d \n", end);
		else
			//printf("EOF encountered at : %d \n",end);
*/
		(fBlocks+i)->sIndex = start;
		(fBlocks+i)->eIndex = end;
		remChars=size-end;
		tc--;
		start=end+1;
	}

/*
	for(i=0;i<threadCount;i++) 
		printf("Block num : %4d . sIndex : %4d eIndex : %4d\n",i,(fBlocks+i)->sIndex,(fBlocks+i)->eIndex);
*/
	fclose(fp);
	return size;
}

int findBlockIndicesFromContent(char *fileContent, int threadCount,int size,struct block * fBlocks ) {
//Fill in sIndex and eIndex of fBlocks[0..threadCount-1]

//        FILE *fp;
        int blockSize,i,start,end;
        char newLine;
  //      fp = fopen(fileName,"r");
    //    assert(fp);

      //  fseek(fp,0,SEEK_END);
        //size = ftell(fp);
        //assert(size!=0);
//        printf("Size of file is : %d\n",size);

//       fseek(fp,0,SEEK_SET);

        start=0;
        end=0;
        int tc=threadCount;
        int remChars = size;
        for(i=0;i<threadCount && end < size;i++) {
                blockSize = remChars/tc;
//                printf("i : %d . blockSize : %d remChars : %d remThread : %d \n",i,blockSize,remChars,tc);
                if( (start+blockSize-1 ) <= size ) {
                       // if (fseek(fp, blockSize ,SEEK_CUR)!= 0) {
                         //       printf("Fseek failed during iteration %d with block size : %d\n",i,blockSize);
                       // }

                        end=end+blockSize;
                }
    //            printf("Searching for newLine .... \n");
                do {
                        newLine = fileContent[end++];
                //      printf("Cur Char = %c \n",newLine);
                } while(newLine!='\n' && newLine!='\0');

  (fBlocks+i)->sIndex = start;
		if(newLine=='\0')
			end--;
                (fBlocks+i)->eIndex = end;
                remChars=size-end;
                tc--;
             start=end+1;
        }

/*
        for(i=0;i<threadCount;i++) 
                printf("Block num : %4d . sIndex : %4d eIndex : %4d\n",i,(fBlocks+i)->sIndex,(fBlocks+i)->eIndex);
  */
        return size;

}

void *sumup(void * id) {
	
	int s;
	int i;
	int index = (int)id;
	for(s=1;s<numThread;s=s*2) {

		if(index%(2*s)==0) {
			for(i=0;i<numProc;i++) {
				count[i][index] += count[i][index+s];
			}

		//	printf("before mutex for thread %d\n",index);
			pthread_mutex_lock(&levelWaitMutex);
			numThreadsInLevel++;
	
		//	printf("in if loop for thread %d and numthreads level is %d\n",index,numThreadsInLevel);
			if(numThreadsInLevel == numThread/(2*s) ) {
			//	printf("Before broadcast signal in thread %d and numthreads level is %d\n",index,numThreadsInLevel);
				numThreadsInLevel=0; // current level verified and woken up, reset to count the number of threads in next level
/*
				for(i=0;i<numThread/(2*s);i++)
					pthread_cond_signal(&levelWaitCond);*/
				pthread_mutex_unlock(&levelWaitMutex);
				pthread_cond_broadcast(&levelWaitCond);
			//	printf("After broadcast signal in thread %d and numthreads level is %d\n",index,numThreadsInLevel);
			} else {
			//	printf("before condition wait for thread : %d\n",index);
				pthread_cond_wait(&levelWaitCond,&levelWaitMutex);	
				pthread_mutex_unlock(&levelWaitMutex);
					
			//	printf("after condition wait for thread : %d\n",index);
			}
		}
	}

}


int main(int argc,char *argv[]) {

//	pthread_t t1,t2,t3,t4,t5;

	if(argc!=3) {
		printf("USAGE : ./log_stats path_to_log_file path_to_process_list_file\n");
		return -1;
	}
		
	strcpy(logFileName,argv[1]);
	char procFileName[50];
	strcpy(procFileName,argv[2]);


	numProc = getProcList(procFileName);
	numThread = 16;
	init(numProc,numThread);

	int fileSize = findBlockIndices(logFileName,numThread);
//	printf("Number of processes in the given list is : %d \n" , numProc);

	char *fullContent = (char *)malloc(fileSize + 1);
        FILE *fp;
        fp = fopen(logFileName,"r");
        assert(fp);
        fread(fullContent,1,fileSize,fp);
        fullContent[fileSize]='\0';
        fclose(fp);

/*
	int threadId=3;
	getStat(&threadId);
*/

	pthread_t *t;
	t=(pthread_t *)malloc(sizeof(pthread_t) * numThread);

	int i;
	for(i=0;i<numThread;i++)
		pthread_create(t + i ,NULL,getStat,(void *)&thrArray[i]);


	for(i=0;i<numThread;i++)
		pthread_join(t[i],NULL);

/*        int totalLogLines=0;
        for(i=0;i< numProc ; i++) {
                printf("pName: %s count : %d\n",psList[i],total[i]);
                totalLogLines+=total[i];
        }

        printf("Total Number of loglines: %d\n",totalLogLines);
*/
	//printDetails();
	 for(i=0;i<numThread;i++)
                pthread_create(t + i ,NULL,sumup,(void *)i);


        for(i=0;i<numThread;i++)
                pthread_join(t[i],NULL);
	

	printRedDetails();

	int  j=0,blockSize;
        int k,tmp=0,numLogLines=0;
	struct timeval sTime,eTime,elapsed;
	for(j=1;j<= MAX_THREAD;j=j*2) {
		free(t);
		free(thrArray);
		free(fBlocks);
		free(count);
		t=(pthread_t *)malloc(sizeof(pthread_t) * j );

		thrArray = (int *)malloc(sizeof(int) * j );
		fBlocks=(struct block*)malloc( sizeof(struct block) * j);

		count= (int **)calloc( numProc , sizeof(int *) );
		for(i=0;i<numProc;i++)
			count[i]=(int *) calloc( j , sizeof(int) );

		for(i=0;i<j;i++) {
			thrArray[i]=i;
			(fBlocks+i)->sIndex=-1;
			(fBlocks+i)->eIndex=-1;
		}

//		printf("111111111\n");
		numThread = j;
		blockSize = fileSize/j;
		findBlockIndicesFromContent(fullContent, numThread,fileSize,fBlocks );
//		printf("2222222222\n");

	        gettimeofday(&sTime,NULL); 
		for(i=0;i<j;i++)
			pthread_create(t + i ,NULL,getStat,(void *)&thrArray[i]);

//		printf("333333333333\n");

		for(i=0;i<j;i++)
			pthread_join(t[i],NULL);

//		printf("4444444444444\n");

	        for(i=0;i<numThread;i++)
        	        pthread_create(t + i ,NULL,sumup,(void *)i);

	        for(i=0;i<numThread;i++)
        	        pthread_join(t[i],NULL);

		numLogLines = 0;
		for(i=0;i<numProc;i++) {
			numLogLines+=count[i][0];
		}

		gettimeofday(&eTime,NULL);
	        timeval_subtract(&elapsed,&sTime,&eTime);
	        printf("blockSize: %d numOfThreads: %d totalCount: %d runningTime: %ld.%06ld \n",blockSize,j,numLogLines,elapsed.tv_usec);

	}

//	printf("Position of grep is %d \n",findMatch("sed"));
}
