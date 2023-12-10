///This code is now intended to be included as a .c file.
///Define the STACK_TYPE, STACK_SUFFIX, and STACK_N macros as needed.
///STACK_TYPE and STACK_SUFFIX will be #undef'd at the end of the file,
///so that you can include the code again with different results.

//////Retrospective thoughts on this version:
///This code was designed to use a global variable
///to check if a stack tried something it couldn't do.
///This variable has the same name for all stacks.
///That kinda sucks.
///Also, why not check the stack before trying to do stuff?
///What was I thinking?


#include <stdio.h>
#include <stdlib.h> //malloc
#include <stdbool.h>

#ifndef STACK_N
	#define STACK_N 255
#endif

#ifndef STACK_TYPE
	#define STACK_TYPE int
#endif

#ifndef STACK_NAME
	#define STACK_NAME(x) x
/* Referring to a macro in a concat macro definition doesn't work in C.
** Define your own STACK_NAME macro.
#else
	//eg. STACK_TYPE STACK_NAME(pop, _struct)(){
	#define STACK_NAME(x) x ## STACK_SUFFIX
*/
#endif

bool stack_idled;

struct STACK_NAME(stack){
	int i;
	STACK_TYPE *vals;
};

struct STACK_NAME(stack) STACK_NAME(new_stack)(void){
	return (struct STACK_NAME(stack)){
		.i=0, .vals = malloc(STACK_N*sizeof(STACK_TYPE))
	};
}

//Doesn't free items on the stack, only deletes the entire stack.
void STACK_NAME(del_stack)(struct STACK_NAME(stack) *s){
	s->vals -= s->i;
	free(s->vals);
	s->vals=NULL;
	s->i=0;
	//The stack struct is not on the heap.
	//So free(s) can't be done.
	//Getting a pointer from malloc to put it on the heap
	//may be worth it, but I'm not convinced.
}



void STACK_NAME(push)(STACK_TYPE x, struct STACK_NAME(stack) *s){
	if(s->i != STACK_N){
		++s->i;
		*(s->vals++)=x;
	}else{
		stack_idled=true;
	}
}

//This will return 0 if it can't pop; check stack_idled.
STACK_TYPE STACK_NAME(pop)(struct STACK_NAME(stack) *s){
	//s->i-- is wrong b/c you can call this function multiple times.
	if(s->i){
		--s->i;
		--s->vals;
		return *s->vals;
	}else{
		stack_idled=true;
		return (STACK_TYPE){0};
	}
}

STACK_TYPE STACK_NAME(peek_x)(struct STACK_NAME(stack) s, int x){
	if(s.i){
		return *(s.vals-x);
	}else{
		stack_idled=true;
		return (STACK_TYPE){0};
	}
}

STACK_TYPE STACK_NAME(peek)(struct STACK_NAME(stack) s){
	return STACK_NAME(peek_x)(s, 1);
}

#undef STACK_N
#undef STACK_TYPE
#undef STACK_NAME
