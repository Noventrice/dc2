//It was annoying when I forgot to change the header file.
//Just include it here so that there is only one place to change.
#include "value.h" //struct value
#include <stdlib.h> //free, malloc, NULL, size_t
#include <string.h> //strdup, strtok

#define ARGS_STR_(_, ...) #__VA_ARGS__
#define ARGS_STR(_, ...) ARGS_STR_(, __VA_ARGS__) //empty arg not portable

char *kinds_array[TOK_KINDS_LEN];
void init_kinds_array(){
	kinds_array[0] = strdup(ARGS_STR(, TOK_KINDS_ARRAY));
	(void) strtok(kinds_array[0], ", ");
	for(int i=1; i < TOK_KINDS_LEN; ++i)
	kinds_array[i] = strtok(NULL, ", ");
}

void free_value(struct value value){
	if(value.type=='s'){
		free(value.s);
		value.s=NULL;
	}
}
