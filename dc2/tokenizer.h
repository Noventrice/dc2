#ifndef TOKENIZER_H
#define TOKENIZER_H
#include "value.h"
#define TOK_LINE_N 1024L //Maximum length of tokenized commands on a line.

extern struct input_type input_type;
struct input_type{
	union {
		FILE *file;
		char *string;
	};
	char * (*next_line)(char *line);
	size_t size;
};

char *next_line_from_file(char *line);
char *next_line_from_str(char *line);

#define STACK_TYPE struct input_type
#define STACK_NAME(x) x ## _input
#include "stack.h"
extern struct stack_input stack_input_s;

void init_str_input(struct input_type in);
void init_file_input(struct input_type in);

void free_tokens(struct value *tokens);
_Bool tokens_errored(struct value *tokens);
extern long line_no;

char *num_query(struct value *tokens, int tok_index, char *p_addr);
struct value *init_tokens();
struct value *tokenize(char *line, _Bool promptable, _Bool counting_lines);
#endif //TOKENIZER_H
