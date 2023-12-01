#include <stdio.h>
#include <stdlib.h> //strtol
#include "../value.h"

int main(int argc, char **argv){
	init_kinds_array();
	if(argc == 1){
		for(int i = 0; i < TOK_KINDS_LEN; ++i)
		printf("%d\t%s\n", i, kinds_array[i]);
		return 0;
	}

	char *end;
	long x;

	for(int i = 1; i < argc; ++i){
		x = strtol(argv[i], &end, 10);
		if(end == argv[i]){
			fprintf(stderr, "%s: invalid number (%s).\n", argv[0], argv[i]);
			return 1;
		}

		printf("%s\n", kinds_array[x]);
	}
}
