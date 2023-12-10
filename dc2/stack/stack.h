///The .c file is also meant to be #include'd.
///This file is for translation units which need to link to the .c functs.
///You will need to redefine the macros which the .c file used (if any).

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



struct STACK_NAME(stack){
	int i;
	STACK_TYPE *vals;
};

struct STACK_NAME(stack) STACK_NAME(new_stack)(void);
_Bool STACK_NAME(stack_empty)(struct STACK_NAME(stack) s);
_Bool STACK_NAME(stack_full)(struct STACK_NAME(stack) s);
void STACK_NAME(del_stack)(struct STACK_NAME(stack) *s);

void STACK_NAME(push)(STACK_TYPE x, struct STACK_NAME(stack) *s);
STACK_TYPE STACK_NAME(pop)(struct STACK_NAME(stack) *s);

STACK_TYPE STACK_NAME(peek_x)(struct STACK_NAME(stack) s, int x);
STACK_TYPE STACK_NAME(peek)(struct STACK_NAME(stack) s);

//eg. peek_x_ref(my_stack, x).member = foo;
STACK_TYPE *STACK_NAME(peek_x_ref)(struct STACK_NAME(stack) s, int x);
STACK_TYPE *STACK_NAME(peek_ref)(struct STACK_NAME(stack) s);

#undef STACK_N
#undef STACK_TYPE
#undef STACK_NAME
