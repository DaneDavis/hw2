#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

/*Dict structure definition as defined in words.c*/

typedef struct dict {
  char *word;
  int count;
  struct dict *next;
} dict_t;

///////////////////////////////////////////////////////////////////

