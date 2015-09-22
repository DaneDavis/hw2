#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>


typedef struct dict{ //used similarly to sharedobject 
 char *word;
 int count;
	struct dict *next; //points to the next dic

	bool flag;

 pthread_mutex_t flaglock;//mutex for flag
 pthread_cond_t flag_true;//variable for flag=true
 pthread_cond_t flag_false;//variable for flag=false

} dict_t;//Name of shared object type

////////////////////////////////////////////////////////////

typedef struct darg{ //As created in lectures 
																					//To be used by consumer
	long did;
	dict_t *diptr;
}darg_t;

///////////////////////////////////////////////////////////

/*Makes a nessecary word used in dict struct*/
char *
make_word( char *word ) {
  return strcpy( malloc( strlen( word )+1 ), word );
}

////////////////////////////////////////////////////////////

/*Below takes in a word and creates a dict*/
dict_t *
make_dict(char *word) {
  dict_t *nd = (dict_t *) malloc( sizeof(dict_t) );
  nd->word = make_word( word );
  nd->count = 1;
  nd->next = NULL;
  return nd;
}

////////////////////////////////////////////////////////////

/*inserts a work into a dict type*/
dict_t *
insert_word( dict_t *d, char *word ) {
  
  //   Insert word into dict or increment count if already there
  //   return pointer to the updated dict
  
  dict_t *nd;
  dict_t *pd = NULL;		// prior to insertion point 
  dict_t *di = d;		// following insertion point
  // Search down list to find if present or point of insertion
  while(di && ( strcmp(word, di->word ) >= 0) ) { 
    if( strcmp( word, di->word ) == 0 ) { 
      di->count++;		// increment count 
      return d;			// return head 
    }
    pd = di;			// advance ptr pair
    di = di->next;
  }
  nd = make_dict(word);		// not found, make entry 
  nd->next = di;		// entry bigger than word or tail 
  if (pd) {
    pd->next = nd;
    return d;			// insert beond head 
  }
  return nd;
}

////////////////////////////////////////////////////////////

/*prints out dic*/
void print_dict(dict_t *d) {
  while (d) {
    printf("[%d] %s\n", d->count, d->word);
    d = d->next;
  }
}

////////////////////////////////////////////////////////////

/*runs through a file a gets the word within the textfile*/
int
get_word( char *buf, int n, FILE *infile) {
  int inword = 0;
  int c;  
  while( (c = fgetc(infile)) != EOF ) {
    if (inword && !isalpha(c)) {
      buf[inword] = '\0';	// terminate the word string
      return 1;
    } 
    if (isalpha(c)) {
      buf[inword++] = c;
    }
  }
  return 0;			// no more words
}

////////////////////////////////////////////////////////////

#define MAXWORD 1024

/*runs through a file and continues to insert_word while
get_word is working*/
dict_t *
words( FILE *infile ) {
  dict_t *wd = NULL;
  char wordbuf[MAXWORD];
  while( get_word( wordbuf, MAXWORD, infile ) ) {
    wd = insert_word(wd, wordbuf); // add to dict
  }
		wd->flag = false;
  return wd;
}

///////////////////////////////////////////////////////////

/*waits until dict->flag = true*/
bool
waittilltrue(dict_t *dic, int did){
	int status;
	if((status = pthread_mutex_lock(&dic->flaglock)) != 0){
		printf("Error in wait till true");
	}

	while(dic->flag != true){
		printf("DID %d waiting till true \n",did);

		pthread_cond_wait(&dic->flag_true, &dic->flaglock);
	}

	printf("DID %d got true \n",did);
	return(true);
}

///////////////////////////////////////////////////////////

/*waits until dict->flag = false*/
bool
waittillfalse(dict_t *dic, int did){
	int status;

	if((status = pthread_mutex_lock(&dic->flaglock)) != 0){
		printf("Error in wait till false");	
	}

	while(dic->flag == true){
		printf("DID %d waiting till false \n",did);
		pthread_cond_wait(&dic->flag_false, &dic->flaglock);
	}

	printf("DID %d got false \n",did);
	return(true);
}

///////////////////////////////////////////////////////////

/*unlocks mutex lock*/
int releasetrue(dict_t *dic, int did){
	dic->flag = true;
	printf("DID %d set true \n",did);
	pthread_cond_signal(&dic->flag_true);
	return(pthread_mutex_unlock(&dic->flaglock));

}

///////////////////////////////////////////////////////////

/*locks mutex lock*/
int releasefalse(dict_t *dic, int did){
	dic->flag = false;
	printf("DID %d set false \n",did);
	pthread_cond_signal(&dic->flag_false);
	return(pthread_mutex_unlock(&dic->flaglock));
}

///////////////////////////////////////////////////////////

int release_exit(dict_t *dic, int did){
	pthread_cond_signal(&dic->flag_true);
	return(pthread_mutex_unlock(&dic->flaglock));
}

///////////////////////////////////////////////////////////
void *
producer(void *arg){
	#define PROD_ID 100

	int status;
 dict_t *dic = (dict_t *)arg;
	int *ret = malloc (sizeof(int));
	int i = 0;
	char *line; //next line
	printf("Producer starting\n"); //Used for testing can remove later
	while((line = dic->word)){
		waittillfalse(dic,PROD_ID); //wait till flag is flase
		dic->count = i++;
		dic->word = line; //line into shared buffer

		fprintf(stdout,"Prod: [%d] %s \n",i,line);

		if((status = releasetrue(dic,PROD_ID)) != 0){
			printf("Error in producer");		
		}
	}
}

///////////////////////////////////////////////////////////

void * 
consumer(void *arg){
	int status;
	darg_t *darg = (darg_t *)arg;
	long did = darg->did;
	dict_t *dic = darg->diptr;
 int *ret = malloc(sizeof(int));
	int i = 0;
	int len;
	char *line;
	printf("Consumer %ld starting\n",did);

	while(waittilltrue(dic,did) && (line = dic->word)){
		len = strlen(line);
		printf("Consumer %ld: [%d:%d] %s\n", did,i++,dic->count,line);

	if((status = releasefalse(dic,did))!= 0){
		printf("Error in consumer");
	}
	

	printf("Cons %ld:%d",did,i);

	release_exit(dic,did);
	*ret = i;
	pthread_exit(ret);
	}
}

///////////////////////////////////////////////////////////

int
main( int argc, char *argv[] ) {

	int status;
	pthread_t prod; //producer thread
	pthread_t cons[4]; //consumer threads - 4 created
	darg_t ddarg[4];

	void *ret;
	dict_t *dic = malloc(sizeof(dict_t));

	dic = NULL;
	FILE *infile = stdin;

	if (argc >= 2) {
   	infile = fopen (argv[1],"r");
  }
  if( !infile ) {
    printf("Unable to open %s\n",argv[1]);
    exit( EXIT_FAILURE );
  }


	dic = words(infile); //possibly later

	/*check creation of producer thread*/
	if((status = pthread_mutex_init(&dic->flaglock,NULL)) != 0){
		printf("Error mutex init");	
	}

	if((status = pthread_cond_init(&dic->flag_true,NULL)) != 0){
		printf("Error cond init true");
	}

	if((status = pthread_cond_init(&dic->flag_false,NULL)) != 0){
		printf("Error cond init false");
	}

	if((status = pthread_create(&prod,NULL,producer,(void *)dic)) != 0){
		printf("Error in creating producer thread");
	}


	/*loops 4 times to create 4 child proccesses*/
	for(int i =0;i<4;i++){
		ddarg[i].did = i;
		ddarg[i].diptr = dic;

		if((status = pthread_create(&cons[i],NULL,consumer,&ddarg[i])) != 0){
			printf("Error in creating consumer thread");
		}
	}
	

	printf("Producer and consumer threads created successfully\n");


	if((status = pthread_join(prod,&ret)) != 0){
		printf("Error in join producer thread");
	}


	for(int i =0;i<4;i++){
		if((status = pthread_join(cons[i],&ret))!= 0){
			printf("Error in join consumer thread");		
		}
		print_dict(dic);
	}

	if((status = pthread_mutex_destroy(&dic->flaglock)) != 0){
		printf("Error in destroy mutex");	
	}

	if((status = pthread_cond_destroy(&dic->flag_true))!= 0){
		printf("Error in destroy flag true");
	}

	if((status = pthread_cond_destroy(&dic->flag_false)) != 0){
		printf("Error in destroy flag false");
	}

	free(dic);
	pthread_exit(NULL);

	print_dict( dic );
  fclose( infile );

}

///////////////////////////////////////////////////////////
