#include <stdio.h>
#include <stdbool.h>

#define STACK_N 4 //Short stack for demonstration purposes.
#include "stack.c"

void show_stack(struct stack s){
	while(s.i) printf("%d\n", pop(&s));
}

int main(void){
	printf("1 => push\n2 => pop\n3 => display\n4 => peek\n5 => delete & exit\n");
	int in, out=0;
	struct stack s=new_stack();
	if(s.vals==NULL){
		del_stack(&s);
		fprintf(stderr, "Out of memory\n");
		return 1;
	}

	while(out!=EOF){
		printf("%%d: ");
		out=scanf("%d: ", &in);
		switch(in){
		case 1:
			printf("push %%d: ");
			out=scanf("%d", &in);
			if(out<1){
				fprintf(stderr, "Bad input.\n");
				continue;
			}
			if(stack_full(s)){
				fprintf(stderr, "Can't push: stack full.\n");
				continue;
			}
			push(in, &s);
			break;
		case 2:
			if(stack_empty(s)){
				fprintf(stderr, "Can't pop: stack empty.\n");
				continue;
			}
			pop(&s);
			break;
		case 3:
			show_stack(s);
			break;
		case 4:
			if(stack_empty(s)){
				fprintf(stderr, "Can't peek: stack empty.\n");
				continue;
			}
			printf("%d\n", peek(&s));
			break;
		case 5:
			del_stack(&s);
			return 0;
		default:
			if(feof(stdin)) return 0;
			fprintf(stderr,
				"Commands are 1, 2, 3, 4 or 5;\n"
				"for: push, pop, display, peek, or exit\n"
				"You can press Ctrl+D to exit.\n"
			);
			while(getchar()!='\n'){}
			break;
		}
	}
}
