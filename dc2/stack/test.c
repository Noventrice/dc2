#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#define STACK_N 2
#include "stack.c"
struct s{
	int d;
	char c;
};

#define STACK_TYPE struct s
#define STACK_NAME(x) x ## _s
#include "stack.c"


struct stack   stack;
struct stack_s stack_s;

int main(void){
	stack = new_stack();
	stack_s = new_stack_s();
	int x;
	struct s s;

	assert(stack_empty(stack));
	(void)peek(&stack);
	assert(stack_empty(stack));

	push(3, &stack);
	push(5, &stack);
	assert(stack_full(stack)); //Short stack!
	x = peek(&stack);
	assert(!stack_empty(stack));
	assert(stack_full(stack));
	assert(x == 5);
	assert(peek_x(&stack, 2) == 3);

	x = pop(&stack);
	assert(!stack_empty(stack));
	assert(x == 5);
	assert(!stack_empty(stack));
	del_stack(&stack);

	s = (struct s){.d = 3, .c = 'a'};
	push_s(s, &stack_s);
	s = pop_s(&stack_s);
	assert(s.d == 3 && s.c == 'a');

	puts("All tests passed.");
	return 0;
}
