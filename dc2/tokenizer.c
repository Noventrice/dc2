#include <stdio.h>
#include <stdlib.h> //malloc, free, exit
#include <stdbool.h>
#include "errf.h"
//struct value, enum tok_kinds, union input_type, TOK_LINE_N
#include "tokenizer.h"
#include <float.h> //DBL_MAX_10_EXP, DBL_DECIMAL_DIG
#include <string.h> //strcpy; main has strlen, strcmp, strtok
#include "isatty.h"
#include "inttypes.h" //PRId64

#include <readline/readline.h>
#include <readline/history.h>


#define STACK_PREDEFINED
#define STACK_TYPE struct input_type //char* or FILE*
#define STACK_NAME(x) x ## _input
#include "stack.c"


long line_no = 0;

static void ignored_line_err(){
	errf("Line %" PRId64 " ignored.\n", line_no);
}


//Thanks to: https://stackoverflow.com/a/1701085
//maximum number of character for a double w/o scientific notation:
//0 minus sign,
//1 decimal point,
//DBL_DECIMAL_DIG digits of precision,
//DBL_MAX_10_EXP  digits for exponent,
//1 null character.
//Assumes base 10; I'll have to change that eventually.
#define DBL_CHARS 1 + DBL_DECIMAL_DIG + DBL_MAX_10_EXP + 1
#define INT_CHARS 19 + 1 //+1 for null char.



////////////////////////////////////////////////////////////////////////////
///In order to reuse the tokenization code,
///the type of input needs to be known.
///If it is a file or stdin you need to read the file,
///otherwise you can read a string.
///Either way, this is done with input_type.next_line().
///.next_line will need to be set to either of these functions to work, tho.
char *next_line_from_str([[maybe_unused]] char *line);
char *next_line_from_file(                char *line);

struct stack_input stack_input_s;

void init_str_input(struct input_type in){
	in.next_line = next_line_from_str;
	in.size = 80;
	push_input(in, &stack_input_s);
}

void init_file_input(struct input_type in){
	in.next_line = next_line_from_file;
	in.size = 80;
	push_input(in, &stack_input_s);
}

////////////////////////////////////////////////////////////////////////////
char *next_line_from_file(char *line){
	int ch;
	size_t i = 0;
	struct input_type in = peek_input(stack_input_s);
	while(true){
		switch( (ch = fgetc(in.file)) ){
		case '\0': case  EOF:
			line[i] = '\0';
			return line;
		case '\n':
			line[i] = '\n';
			return line;
		default:
			line[i] = ch;
		}

		if(++i == in.size){
			in.size *= 2;
			line = realloc(line, in.size);
			if(line == NULL){
				perror(NULL);
				return NULL;
			}
		}
	}
}

//This is designed to be used at the end of a loop,
//unlike the function for files.
char *next_line_from_str(char *line){
	(void)line;
	struct input_type *in = peek_ref_input(stack_input_s);
	while(true){
		switch( in->string[0] ){
		case '\0': return   in->string;
		case '\n': return ++in->string;
		default: ++in->string;
		}
	}
}

struct str_info{
	char *str;
	char *end; //end of str
};

//'[' and ']' tokens are added outside this function.
//All escape sequences are kept in the string until printing is done.
//Consequently, \n and \t aren't handled in this loop.
//MEMO: the resulting string should not be given directly to printf().
static struct str_info tokenize_string(char *line){
	char ch;
	size_t input_line_len = peek_input(stack_input_s).size;
	char *out=malloc(input_line_len);
	int in_index = 0;
	size_t out_index = 0;
	bool escape=false;
	int level=1;


	while(true){
		if(out_index+1 >= input_line_len){
			input_line_len *= 2;
			peek_ref_input(stack_input_s)->size = input_line_len;
			out = realloc(out, input_line_len);
			if(line == NULL){
				perror(NULL);
				return (struct str_info){ .str=NULL };
			}
		}

		ch = line[in_index++];

		if(ch == '\0' || ch == EOF){ //But not '\n'.
			out[out_index] = '\0';
			if(out_index+1 < 40){
				errf("String didn't end (%s).\n", out);
			}else{
				errf("String didn't end (%.40s...).\n", out);
			}
			free(out);
			return (struct str_info){ .str=NULL };
		}else if(ch == '\n'){
			++line_no;

			line = peek_input(stack_input_s).next_line(line);
			if(!line) return (struct str_info){ .str=NULL };
			in_index = 0;
		}

		if(!escape){
			if(ch == '\\'){
				escape = true;
				out[out_index++] = '\\';
				continue;
			}else if(ch == '['){
				++level;
				//Add a literal '[' below.
			}else if(ch == ']'){
				if(level == 1){
					out[out_index] = '\0';
					return (struct str_info){
						.str=out, .end=&(line[in_index])
					};
				}
				--level;
				//Add a literal ']' below.
			}
		} //escape = True

		escape = false;
		out[out_index++] = ch;
	}
}


static void add_char_token(
	struct value *token,
	enum tok_kinds kind,
	int ch
){
	*token = (struct value){ .kind=kind, .type='c', .c=ch };
}

static struct value *finalize_tokens(
	struct value *result,
	enum tok_kinds kind,
	int last_index
){
	result[0] = (struct value){ .kind=kind, .type='d', .d=last_index };
	return result;
}

#define TOK_LEN_CHECK(step, ret)                                           \
	if(tok_index+step >= TOK_LINE_N){                                      \
		errf("Ran out of room for tokens on line %" PRId64 ".\n", line_no);\
		ignored_line_err();                                                \
		finalize_tokens(tokens, ERROR_TOK, tok_index);                     \
		return ret;                                                        \
	}

#define PAREN_CHECK()                                         \
	if(parens){                                               \
		errf("Unmatched %c on line %" PRId64 ".\n",           \
			parens > 0 ? '(' : ')',                           \
			line_no                                           \
		);                                                    \
		return finalize_tokens(tokens, ERROR_TOK, tok_index); \
	}

struct value *init_tokens(){
	return malloc(sizeof(struct value)*TOK_LINE_N);
}

//strtok doesn't split at the \n at the end of the string.
struct value *tokenize(char *line, bool use_prompt, bool counting_lines){
	struct value *tokens = malloc(sizeof(struct value)*TOK_LINE_N);

	if(counting_lines) ++line_no; //Don't count lines for the x operator.
	if(use_prompt){
		//'<' + 3 digits + 1 safety digit + "> \0"
		char line_no_str[1+3+1+3];
		sprintf(line_no_str, "<%3ld> ", line_no);
		line = readline(line_no_str); //Prompt.
		if( line == NULL){ //EOF on prompt.
			return finalize_tokens(tokens, END_TOK, 0);
		}
		add_history(line);
	}

	char *p = line;
	char ch;

	//Setting this to START_TOK makes testing the previous kind easier.
	//It may be tempting to start with NEG_OP_TOK, or OPEN_PAREN_TOK;
	//but you need to have a way to distinguish them from START_TOK.
	//Afterall, you need to know if an unary operator was actually given,
	//so that you can use the operator when it exists.
	//Example: 1+--2 (|1|\n|+|\n|-|\n|-2|).
	//
	//Later, when executing tokens, you need to skip the 0th element.
	//tokens[0].d will contain the max index to `tokens' when returned.
	tokens[0].kind = START_TOK;

	int tok_index=0; //The index of the most recent token in `tokens'.
	int parens = 0;

	while(true){
		p = num_query(tokens, tok_index, p);
		tok_index = tokens->d;
		if(tokens_errored(tokens)) return tokens;
		ch = *(p++);

		TOK_LEN_CHECK(1, tokens) //;

		switch(ch){
		case '(':
			parens += 1;
			add_char_token(&tokens[++tok_index], OPEN_PAREN_TOK, ch);
			break;
		case ')':
			parens -= 1;
			add_char_token(&tokens[++tok_index], CLOSE_PAREN_TOK, ch);
			break;

		case '+': case '-':
		case '*': case '/': case '%':
		case '^': //'v' ?
		case 'x':
			add_char_token(&tokens[++tok_index], MATH_OP_TOK, ch);
			break;

		case '?':
			add_char_token(&tokens[++tok_index], INPUT_TOK, ch);
			break;

		#include "funct_cases.inc" //p, q, c, f, d, r, _, etc...
			add_char_token(&tokens[++tok_index], FUNC_OP_TOK, ch);
			break;

		case 's': case 'l':
			if(ch == 's'){
				add_char_token(&tokens[++tok_index], SAVE_OP_TOK, ch);
			}else{
				add_char_token(&tokens[++tok_index], LOAD_OP_TOK, ch);
			}

			ch = *(p++);

			//Should I disallow saving/loading with '\n' too?
			//How would OP_PENDING handle that?
			if(ch == '\0'){
				errf("Choosing to not use variable named null byte.\n");
				ignored_line_err();
				return finalize_tokens(tokens, ERROR_TOK, tok_index);
			}else if(ch == '['){
				errf("Choosing to not use variable named \"[\".\n");
				ignored_line_err();
				return finalize_tokens(tokens, ERROR_TOK, tok_index);
			}else if(ch == EOF){
				errf("Identifier was missing at the end of the input.\n");
				ignored_line_err();
				return finalize_tokens(tokens, ERROR_TOK, tok_index);
			}

			char *id = malloc(2);
			id[0] = ch ; id[1] = '\0';
			tokens[++tok_index] = (struct value){
				.kind=IDENTIFIER, .type='s', .s=id
			};
			continue;

		case '[': //TODO: Or " for text.
			//You have already checked if space was needed for 1 token.
			TOK_LEN_CHECK(3-1, tokens) //;
			add_char_token(&tokens[++tok_index], STR_TOK, '[');

			struct str_info s_info = tokenize_string(p);
			if(s_info.str == NULL){
				ignored_line_err();
				return finalize_tokens(tokens, ERROR_TOK, tok_index);
			}

			tokens[++tok_index] = (struct value){
				.kind = STR_TOK, .type='s', .s=s_info.str
			};

			//tokenize_string has already found ']'.
			p = s_info.end;
			add_char_token(&tokens[++tok_index], STR_TOK, ']');
			continue;

		case '#':
			PAREN_CHECK() //;
			add_char_token(&tokens[++tok_index], COMMENT, '#');
			return finalize_tokens(tokens, COMMENT, tok_index);

		case ' ': case '\t':
			continue;

		case '\n': case '\0':
			PAREN_CHECK() //;
			return finalize_tokens(tokens, EOL_TOK, tok_index);
		case EOF:
			PAREN_CHECK() //;
			return finalize_tokens(tokens, END_TOK, tok_index);

		case ']':
			errf("Unexpected ].\n");
			ignored_line_err();
			return finalize_tokens(tokens, ERROR_TOK, tok_index);

		default:
			add_char_token(&tokens[++tok_index], UNKNOWN, ch);
			continue;
		}
	} //No more tokens.
}

char *num_query(struct value *tokens, int tok_index, char *p){
	char num_buffer[DBL_CHARS];
	int buff_max = DBL_CHARS;
	int num_index;
	char ch;

	ch = *p;

	while(ch == '-' && tokens[tok_index].kind <= NEG_OP_TOK){
		TOK_LEN_CHECK(1, p) //;
		//Save negation as '.' to distinguish subtraction.
		add_char_token(&tokens[++tok_index], NEG_OP_TOK, '.');
		ch = *(++p);
	}

	if( (ch>='0' && ch<='9') || ch == '.' ){
		if(ch == '.'){
			tokens[++tok_index].kind = RADIX;
		}else{
			tokens[++tok_index].kind = INT;
			buff_max = INT_CHARS;
		}

		num_index = 0;
		TOK_LEN_CHECK(1, p) //;

		num_buffer[num_index++] = ch;
		ch = *(++p);

		while((ch>='0' && ch<='9') || ch == '.'){
			if(num_index == buff_max){
				num_buffer[num_index-1] = '\0';
				errf("Number is too big (%s...)\n", num_buffer);
				finalize_tokens(tokens, ERROR_TOK, tok_index);
				return p;
			}

			if(ch == '.'){
				if(tokens[tok_index].kind == RADIX){
					tokens[tok_index].kind = RADIX_ERROR;
				}else if(tokens[tok_index].kind < RADIX_ERROR){
					++(tokens[tok_index].kind);
					buff_max = DBL_CHARS;
				}
			}else{
				if(tokens[tok_index].kind == RADIX){
					tokens[tok_index].kind = DOUBLE;
					buff_max = DBL_CHARS;
				}
			}


			num_buffer[num_index++] = ch;
			ch = *(++p);
		}

		num_buffer[num_index]='\0';

		if(tokens[tok_index-1].kind >= RADIX){
			TOK_LEN_CHECK(1, p) //;
			tokens[tok_index+1] = tokens[tok_index];
			tokens[tok_index++] = (struct value){
				.type='c', .c=' ', .kind=PUSH_TOK
			};
		}

		if(tokens[tok_index].kind == INT){
			sscanf(num_buffer, "%" PRId64, &( tokens[tok_index].d ));
			tokens[tok_index].type='d';
		}else if(tokens[tok_index].kind == DOUBLE){
			sscanf(num_buffer, "%lg", &( tokens[tok_index].g ));
			tokens[tok_index].type='g';
		}else{
			errf("Bad token (%s) on line %" PRId64 ".\n",
				num_buffer, line_no
			);
			ignored_line_err();
			finalize_tokens(tokens, ERROR_TOK, tok_index);
			return p;
		}
	}

	tokens->d = tok_index;
	return p;
}

bool tokens_errored(struct value *tokens){
	return tokens[0].kind == ERROR_TOK;
}

void free_tokens(struct value *tokens){
	struct value *start=tokens;
	int last_index = tokens[0].d;
	for(int i=1; i<=last_index; ++i){
		free_value( *(++tokens) );
	}
	free(start);
	start = NULL;
}

#ifdef TOKENIZER_MAIN
static void print_tokens(struct value *tokens, bool use_kinds){
	int last_index = tokens[0].d;
	if(use_kinds){
		for(int i=1; i<=last_index; ++i){
			++tokens;
			switch(tokens->type){
			case 'c':
				printf("|%c|\t%2d %s\n",
					tokens->c, tokens->kind, kinds_array[tokens->kind]
				);
				if(tokens->c == 'q'){
					free(*kinds_array);
					exit(0);
				}
				break;
			case 'd':
				printf("|%" PRId64 "|\t%2d %s\n",
					tokens->d, tokens->kind, kinds_array[tokens->kind]
				);
				break;
			case 'g':
				printf("|%lg|\t%2d %s\n",
					tokens->g, tokens->kind, kinds_array[tokens->kind]
				);
				break;
			case 's':
				printf("|%s|\t%2d %s\n",
					tokens->s, tokens->kind, kinds_array[tokens->kind]
				);
				break;
			}
		}
	}else{
		for(int i=1; i<=last_index; ++i){
			++tokens;
			switch(tokens->type){
			case 'c':
				printf("|%c|\n",  tokens->c);
				if(tokens->c == 'q') exit(0);
				break;
			case 'd': printf("|%" PRId64 "|\n", tokens->d); break;
			case 'g': printf("|%lg|\n", tokens->g); break;
			case 's': printf("|%s|\n",  tokens->s); break;
			}
		}
	}
}

//[-k] | [-f file] | [-e expression]
int main(int argc, char **argv){
	char *line = NULL;
	bool kind_opt=false;
	struct input_type in_type;
	struct value *tokens;
	int ret_status = 0;

	stack_input_s = new_stack_input();

	tokens = init_tokens();
	if(tokens == NULL){
		perror(argv[0]);
		return 1;
	}

	while(argc > 1){
		int arg_index=0;
		while(argv[1][arg_index] != '\0'){
			switch(argv[1][arg_index]){
			case '-': break;
			case 'k':
				kind_opt=true;
				init_kinds_array();
				break;
			case 'e':
				line = argv[2];
				in_type.string = line;
				init_str_input(in_type);
				while(line[0] != '\0'){
					tokens = tokenize(line, false, true);
					if(tokens_errored(tokens)){
						ret_status = 1;
						break;
					}
					print_tokens(tokens, kind_opt);
					line = next_line_from_str(line);
				}

				free_tokens(tokens);
				return ret_status; //break;

			case 'f':;
				FILE *fh = fopen(argv[2], "r");
				in_type.file = fh;
				init_file_input(in_type);

				line = malloc( peek_input(stack_input_s).size );
				while(1){
					line = next_line_from_file(line);
					tokens = tokenize(line, false, true);

					if(tokens_errored(tokens)){
						ret_status = 1;
						break;
					}
					print_tokens(tokens, kind_opt);
					if(feof(fh)) break;
				}

				free_tokens(tokens);
				fclose(fh);
				free(line);
				return ret_status; //break;
			default:
				errf("%s: Option not recognized (%s).\n", argv[0], argv[1]);
				return 1;
			}
			++arg_index;
		}

		++argv, --argc; //shift
	} //options

	if(!isatty(0)) line = malloc( peek_input(stack_input_s).size );
	in_type.file = stdin;
	init_file_input(in_type);

	while(1){
		if(isatty(0)){
			tokens = tokenize(line, true, true);
		}else{
			line = next_line_from_file(line);
			tokens = tokenize(line, false, true);
		}

		ret_status = tokens_errored(tokens);
		if(!ret_status){
			print_tokens(tokens, kind_opt);
		}

		if(feof(stdin)) break;
	}

	free_tokens(tokens);
	free(line);
	return ret_status;
}
#endif //TOKENIZER_MAIN

