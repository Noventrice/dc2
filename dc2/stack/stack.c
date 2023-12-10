///This code is now intended to be included as a .c file.
///Define the STACK_TYPE, STACK_SUFFIX, and STACK_N macros as needed.
///STACK_TYPE and STACK_SUFFIX will be #undef'd at the end of the file,
///so that you can include the code again with different results.


#include <stdio.h>
#include <stdlib.h> //malloc
#include <stdbool.h>

//NOTE: STACK_N is not modified by STACK_NAME(x).
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


//TLDR: if you get an error message about redefining the stack, then try:
//#definine STACK_PREDEFINED
//
//Example scenario:
//The arrows represent #include:
// stack.h  stack.c
//  \        \
//   > obj.h -> obj.c
//     \
//      > main.c
//If STACK_PREDEFINED isn't #define'd,
//then obj.c will try to #include a definition of struct stack
//from stack.c and stack.h.
//Despite the definitions being identical, this is not allowed.
#ifndef STACK_PREDEFINED
struct STACK_NAME(stack){
	int i;
	STACK_TYPE *vals;
};
#else
	#undef STACK_PREDEFINED
#endif

bool STACK_NAME(stack_empty)(struct STACK_NAME(stack) s){
	return s.i == 0;
}

bool STACK_NAME(stack_full)(struct STACK_NAME(stack) s){
	return s.i == STACK_N;
}

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
	++s->i;
	*(s->vals++)=x;
}

STACK_TYPE STACK_NAME(pop)(struct STACK_NAME(stack) *s){
	--s->i;
	--s->vals;
	return *s->vals;
}

STACK_TYPE STACK_NAME(peek_x)(struct STACK_NAME(stack) s, int x){
	return *(s.vals-x);
}

STACK_TYPE STACK_NAME(peek)(struct STACK_NAME(stack) s){
	return STACK_NAME(peek_x)(s, 1);
}

//eg. peek_x_ref(my_stack, x).member = foo;
STACK_TYPE *STACK_NAME(peek_x_ref)(struct STACK_NAME(stack) s, int x){
	return s.vals-x;
}

STACK_TYPE *STACK_NAME(peek_ref)(struct STACK_NAME(stack) s){
	return STACK_NAME(peek_x_ref)(s, 1);
}

#undef STACK_N
#undef STACK_TYPE
#undef STACK_NAME
