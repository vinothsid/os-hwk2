#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#define NUMTHREADS 20

typedef struct list{
char *item;
struct list *next;
}Node;

Node *begin = NULL;
Node *end = NULL;

int searchThreads = 0;
int insertThreads = 0;
int deleteThreads = 0;
int waitingSearchThreads = 0;
int waitingDeleteThreads = 0;

pthread_mutex_t addDelLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t delRetLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t searchCountLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  retrieverPresent = PTHREAD_COND_INITIALIZER;
pthread_cond_t  deleterPresent = PTHREAD_COND_INITIALIZER;

bool addToList(char *item){
	bool success = false;
	
	Node *new = NULL;
	new = malloc(sizeof(Node));
	if(new == NULL){
		fprintf(stderr,"addToList: Malloc failed");
		return false;
	}

	char * buf = malloc(strlen(item) + 1);
	if(buf == NULL){
		fprintf(stderr,"addToList: Malloc failed");
		return false;
	}

	strcpy(buf,item);
	new->item = buf;
	new->next = NULL;

	if(begin == NULL || end == NULL){
		begin = new;
		end = new;
		return true;
	}

	end->next = new;
	end = end->next;
	return true;
}

void printList(){
	if(begin == NULL)
		return;

	Node *temp = begin;

	printf("==============\n");
	while(temp!=NULL){
		printf("%s\n" , temp->item);
		temp = temp->next;
	}
}

bool removeFromList(char *item){
	if(end == NULL){
		return false;
	}

	Node *temp = begin;
	Node *prev = NULL;

	while(temp != NULL){
		if(strcmp(temp->item,item)== 0){
			if(prev == NULL){ 
				begin = begin->next;
				free(temp->item);
				free(temp);
				return true;
				}
			
			if(temp->next == NULL){ 
				end = prev;
			}

			prev->next = temp->next;
			free(temp->item);
			free(temp);
			return true;
		}	
		prev = temp;	
		temp = temp->next;
	}
	return false;
}

bool searchList(char *item){
	Node * temp;
	temp = begin;
	
	if(temp == NULL){
		return false;
	}

	while(temp != NULL){
		if(strcmp(temp->item,item) == 0){
			return true;
		}
		temp = temp->next;
	}
	return false;
}

void printThreadInfo(char* operation, char* value, bool success, pthread_t tid){
	int len = strlen(value);
	//value[len-1] = '\0'; //remove the endline char
	if(success)
		printf("[%08x]    Success %s [ %s ] Retrievers : %i Adders : %i Deleters : %i\n" ,tid, operation,value,searchThreads,insertThreads,deleteThreads);
	else	
		printf("[%08x]    Fail %s [ %s ] Retrievers : %i Adders : %i Deleters : %i\n" , tid , operation,value,searchThreads,insertThreads,deleteThreads);

}

static pid_t gettid(void) {
            return (pid_t) syscall(SYS_gettid);
}

void *adders(void *word) {

	char *w = (char *)word;
	pthread_mutex_lock(&addDelLock);
	insertThreads++;
	bool result = addToList(w);

	printThreadInfo("Add", w, result ,gettid());
	insertThreads--;
	pthread_mutex_unlock(&addDelLock);
}

void *retrievers(void *word) {

	char *w = (char *)word;
	pthread_mutex_lock(&delRetLock);
	while(deleteThreads!=0) {
		waitingSearchThreads++;
		pthread_cond_wait(&deleterPresent,&delRetLock);
		waitingSearchThreads--;
	}

	pthread_mutex_unlock(&delRetLock);

	pthread_mutex_lock(&searchCountLock);
	searchThreads++;
	pthread_mutex_unlock(&searchCountLock);

	bool result = searchList(w);
        printThreadInfo("Retrieve", w, result ,gettid());


	pthread_mutex_lock(&searchCountLock);
	searchThreads--;
	pthread_mutex_unlock(&searchCountLock);

	if(searchThreads == 0 ) {
		while(waitingDeleteThreads !=0) {
			pthread_cond_signal(&retrieverPresent);
		}
	}
}

void *deleters(void *word) {

        char *w = (char *)word;

	pthread_mutex_lock(&delRetLock);
	while(searchThreads != 0 ) {
		waitingDeleteThreads++;
		pthread_cond_wait(&retrieverPresent,&delRetLock);
		waitingDeleteThreads--;
	}	
        pthread_mutex_lock(&addDelLock);
        deleteThreads++;
        bool result = removeFromList(w);

        printThreadInfo("Delete", w, result ,gettid());
        deleteThreads--;

//	if(deleteThreads==0) {
		while(waitingSearchThreads != 0) {
			pthread_cond_signal(&deleterPresent);
		}
//	}
        pthread_mutex_unlock(&addDelLock);
	pthread_mutex_unlock(&delRetLock);
}


int main(int argc , char** argv) {
	pthread_t tid[NUMTHREADS];
	int i=0;
	char oper[NUMTHREADS] = "AARRDAADDDDRRRRDRRRR";
	char words[NUMTHREADS][7] =  {"word1" , "word2" , "word1" , "word2","word1" , "word3" ,"word4" , "word3" ,"word4" , "word3" , "word1" , "word2" , "word2" , "word3" , "word1", "word4" , "word1" , "word1" , "word2" , "word2"  };
	for(i=0;i<NUMTHREADS;i++) {
//		char *word = "new";
		char *word = words[i];
		if(oper[i] == 'A' )
			pthread_create(&tid[i],NULL,adders,(void *)word);
		else if(oper[i] == 'D' )
			pthread_create(&tid[i],NULL,deleters,(void *)word);
		else if(oper[i] == 'R')
			pthread_create(&tid[i],NULL,retrievers,(void *)word);
		else
			printf("Invalid operation\n");
	}
	
	/*for(i=10;i<NUMTHREADS;i++) {
		char *word = "new";
		pthread_create(&tid[i],NULL,deleters,(void *)word);
	}*/

	for(i=0;i<NUMTHREADS;i++) {
		pthread_join(tid[i],NULL);
	}
	printList();
}

