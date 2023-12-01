#include <stdio.h>
#include "../tokenizer.h" //TOK_LINE_N

int main(){
	printf("[");
	int i=1;
	//Add 3 extra to test consume().
	while(i++ < TOK_LINE_N+3) printf("f");
}
