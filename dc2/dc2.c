//dc (desk calculator) but with infix arithmetic.
//Special thanks to:
	//https://www.youtube.com/watch?v=4FeOstlzpqg

///NOTES:
///2+3p * (6+7p)p
///The math will be evaluated with the normal order of operations rules.
///However, the left side of the * will be output first,
///despite having lower paren-precedence.
///This happens because the input is read from left to right,
///considering precedence locally.
///This is also how normal order of operations works in mathematics.
///
///Here is a walk through the command:
///Push 2 to the argument stack.
///Push + to the operator stack.
///Push 3 to the argument stack.
///Check if the next operator has a higher priority.
///The addition is a higher priority than the next operator, `p', so add.
///Pop +.
///Push p to the operator stack.
///Output the top of the argument stack (5) and pop p.
///Push * to the operator stack.
///Push ( to the operator stack.
///Push 6 to the argument stack.
///Push + to the operator stack.
///Push 7 to the argument stack.
///Check if the next operator has a higher priority.
///The addition is a higher priority than the next operator, `p', so add.
///Push p to the operator stack.
///Push ( to the operator stack (in theory, but in practice its unnecessary).
///Evaluate the stacks until you can pop (.
	///Output the top of the argument stack (13) and pop p.
///Pop (.
///Multiply and pop *.
///Push p to the operator stack.
///Print the result (65) and pop p.


//TODO: https://stackoverflow.com/questions/4518011/algorithm-for-powfloat-float
//TODO: add I/O number bases.
	//TODO: Change sscanf to a custom-built function.
		//You don't need to validate the input within the function,
		//It has already been checked.
	//TODO: Mess with DBL_CHARS and INT_CHARS (in tokenizer.c)
//TODO: add " strings.
//TODO: add ' units.
//TODO: add new string conversion operator (~).
//TODO: add new numerical conversion operator (+ (unary)).
	//Actually, the x operator already does this.
		//ACTUALLY, the x operator does this UNSAFELY.
		//You wouldn't want to execute arbitrary code from a user.
//TODO: add if statements.
	//use '[' for blocks.
//TODO: add linewise char count in errors.
	//You would probably have to save spaces and other useless tokens.
	//Actually, you could make an array of indices.
//TODO: OP_PENDING mode.
	//3 + \n
	//3 [+]x 5
	//-----
	//3s\na #No!
//TODO: consider adding an end of expression operator (;) for easy negation.
	//You could just write on a newline.
	//This operator's functionality overlalps with PUSH_TOK.
//TODO: Make BIN_OP use `.kind'.
//TODO: make % use fmod?
//TODO: Test strdup for NULL.
//FIXME: Strings with literal newlines don't behave consistiently
	//These allow literal newlines:
		//tokenizer -e $'[foo\nbar]p'
		//printf '[foo\nbar]p' | tokenizer
	//But, putting a newline in stdin from the prompt causes an error.

#include <stdio.h>
#include "tokenizer.h" //tokenizer, input_type
#include "inttypes.h" //PRId64
#include <stdbool.h>
#include <string.h> //strlen, strcmp, strcpy
#include "isatty.h"
#include <stdlib.h> //exit, malloc, free
#include <assert.h>

#include "hash.h"
#include "math.h" //pow

#include "errf.h"

#include "value.h" //struct value
#define STACK_TYPE struct value
#include "stack.c"  //-iquote ../stack/

struct stack arg_stack;
struct stack op_stack;
hash *table;

static void show_stack(struct stack s){
	struct value value;
	while(s.i){
		value=pop(&s);
		switch(value.type){
			case 'g': printf("%lg\n", value.g); break;
			case 'd': printf("%" PRId64 "\n", value.d); break;
			case 's': printf("%s\n", value.s); break;
			case 'c': printf("%c\n", value.c); break; //For debugging.
			default: assert(0);
		}
	}
}


static void print_esc(char *s){
	int level = 1;

	for(char *p=s; *p; ++p){
		if(*p=='\\' && level == 1){
			++p;
			switch(*p){
			case 'n': printf("%c", '\n'); break;
			case 't': printf("%c", '\t'); break;
			//case '[': case ']': case '\\': //FALLTHROUGH
			default:  printf("%c", *p); break;
			}

			continue;
		}

		if     (*p=='[') ++level;
		else if(*p==']') --level;

		printf("%c", *p);
	}
}

enum precedence{
	UNIMPLEMENTED_OP,
	FUNCT_OP,
	ADD_OP,
	MUL_OP,
	NEG_OP,
	POW_OP,
	EXEC_OP, //x, s, l operators
	PAREN_OP
};

static enum precedence get_preced(char c){
	switch(c){
	case '(': return PAREN_OP;
	case 's': case 'l': case 'x': return EXEC_OP;
	case '.': return NEG_OP; //The negation op is saved internally as '.'.
	case '^': return POW_OP;

	case '*': case '/': case '%':
		return MUL_OP;
	case '+': case '-':
		return ADD_OP;

	case '#': case ' ':
	#include "funct_cases.inc" //p, q, c, f, d, r, _, etc...
		return FUNCT_OP;

	default:
		errf("Unimplemented token (%c).\n", c);
		return UNIMPLEMENTED_OP;
	}
}


//This looks like the meme-worthy bad code at:
//https://thedailywtf.com/articles/What_Is_Truth_0x3f_
//But, I actually like it here.
//It's akin to returning numbers greater than 1 from main.
enum returns{
	PASS,
	FAIL,
	//You could test for halt by determining if your op stack only contains
	//a terminal operator (ie. `;').
	//This language doesn't have a terminal operator.
	HALT
	//TODO: OP_PENDING //Should 3+dp be valid in the future?
};

static enum returns push_and_check(struct value value, struct stack *stack){
	if(stack_full(*stack)){
		errf("Stack full.\n");
		return FAIL;
	}
	push(value, stack);
	return PASS;
}

static enum returns pop_precheck(struct stack *stack){
	if(stack_empty(*stack)){
		errf("Stack empty.\n");
		return FAIL;
	}
	//Works with peek too.
	//pop(stack);
	return PASS;
}

#define BIN_OP(name, op)                   \
static void name(struct value a, struct value b){ \
	if     (a.type=='g' && b.type=='g'){ a.g       op##= b.g ;            }\
	else if(a.type=='d' && b.type=='g'){ a.g = a.d op    b.g ; a.type='g';}\
	else if(a.type=='g' && b.type=='d'){ a.g = a.g op    b.d ;            }\
	else{ /*a.type=='d' && b.type=='d'*/ a.d       op##= b.d ;            }\
	push(a, &arg_stack);                                                   \
}

//The name div was taken by stdlib.
BIN_OP(add_set, +) //;
BIN_OP(sub_set, -) //;
BIN_OP(mul_set, *) //;

static void div_set(struct value a, struct value b){
	if     (a.type=='g' && b.type=='g'){ a.g               /=         b.g; }
	else if(a.type=='d' && b.type=='g'){ a.g =         a.d /          b.g; }
	else if(a.type=='g' && b.type=='d'){ a.g =         a.g /          b.d; }
	else{ /*a.type=='d' && b.type=='d'*/ a.g = (double)a.d /  (double)b.d; }
	a.type = 'g';
	a.kind = DOUBLE;
	push(a, &arg_stack);
}

static void mod_set(struct value a, struct value b){
	if(a.type == 'g') a.d = (int)a.g;
	if(b.type == 'g') b.d = (int)b.g;
	a.d %= b.d;
	push(a, &arg_stack);
}

static void pow_set(struct value a, struct value b){
	if     (a.type=='g' && b.type=='g'){ a.g = pow(a.g, b.g); a.type='g'; }
	else if(a.type=='d' && b.type=='g'){ a.g = pow(a.d, b.g); a.type='g'; }
	else if(a.type=='g' && b.type=='d'){ a.g = pow(a.g, b.d); a.type='g'; }
	else{
		if(b.d >= 0){
			int result = 1;
			for(int i=b.d; i>0; --i) result *= a.d;
			a.d = result;
		}else{
			double result = 1;
			//Repeatedly dividing would give less accuracy than
			//setting a.g = 1/result at the end.
			for(int i=b.d; i<0; ++i) result /= a.d;
			a.g = result;
			a.type = 'g';
		}
	}

	push(a, &arg_stack);
}

static enum returns exec(struct value *tokens, bool nested);

static enum returns eval(){
	if( stack_empty(op_stack) ){
		errf("Missing operator on line %" PRId64 ".\n", line_no);
		return FAIL;
	}
	char op = pop(&op_stack).c;

	struct value a, b; //b gets popped first.
	enum precedence preced = get_preced(op);
	//If operator takes two operands...
	if(op == 'r' || op == 's'){
		if(arg_stack.i<2){
			//+3 at the start, or 3+\n
			//But 3\n+5 is valid.
			errf("Missing operand to %c.\n", op);
			return FAIL;
		}

		b = pop(&arg_stack);
		a = pop(&arg_stack);
	}else if( preced >= ADD_OP && preced <= POW_OP && preced != NEG_OP ){
		if(arg_stack.i<2){
			//+3 at the start, or 3+\n
			//But 3\n+5 is valid.
			errf("Missing operand to %c.\n", op);
			return FAIL;
		}

		b=peek(arg_stack);
		a=peek_x(arg_stack, 2);
		if( //type check
			(a.type != 'd' && a.type != 'g') ||
			(b.type != 'd' && b.type != 'g')
		){
			errf("Mismatched types with %c operator.\n", op);
			errf("Stopped execution for line %" PRId64 ".\n", line_no);
			return FAIL;
		}
		(void)pop(&arg_stack);
		(void)pop(&arg_stack);
	}

	switch(op){
	case '*': mul_set(a, b); break;
	case '/': div_set(a, b); break;
	case '+': add_set(a, b); break;
	case '-': sub_set(a, b); break;
	case '%': mod_set(a, b); break;
	case '^': pow_set(a, b); break;
	case '.':
		if(pop_precheck(&arg_stack) == FAIL) return FAIL;
		a = pop(&arg_stack);

		if(a.type == 'd'){
			a.d *= -1;
			push(a, &arg_stack);
		}else if(a.type == 'g'){
			a.g *= -1;
			push(a, &arg_stack);
		}else{
			errf("Error: tried to negate \"%s\" on line %" PRId64 ".\n",
				a.s, line_no
			);
			return FAIL;
		}
		break;

	case 'p': case 't':
		if(pop_precheck(&arg_stack) == FAIL) return FAIL;
		a = peek(arg_stack);
		if     (a.type=='g') printf("%lg", a.g);
		else if(a.type=='d') printf("%" PRId64 "", a.d);
		else if(a.type=='s') print_esc(a.s);

		if(op == 'p'){
			printf("\n");
			break; //exit case
		}

		//The prompt can appear before stdin otherwise.
		if(isatty(0)) fflush(stdout);
		break;

	case 'f': show_stack(arg_stack); break;
	case 'c':
		while(arg_stack.i) free_value(pop(&arg_stack));
		break;
	case 'q': return HALT;
	case '#': break; //Handled by the tokenizer.
	case 'd':
		a=peek(arg_stack);
		if(a.type == 's') a.s = strdup(a.s);
		if(push_and_check(a, &arg_stack) == FAIL) return FAIL;
		break;
	case '_':
		if(pop_precheck(&arg_stack) == FAIL) return FAIL;
		free_value(pop(&arg_stack));
		break;
	case ' ': break; //push nums
	case 'r':
		push(b, &arg_stack);
		push(a, &arg_stack);
		break;
	case 's': set_hash(table, b.s, a); break;
	case 'l':
		; //Clang doesn't like declarations after labels.
		char *name = pop(&arg_stack).s;
		hash *element = lookup_hash(table, name);
		if(element==NULL){
			set_hash(table, name, plain);
			push(plain, &arg_stack);
		}else{
			push(element->value, &arg_stack);
		}
		break;
	case 'x': ; //Clang doesn't like declarations after labels.
		if(pop_precheck(&arg_stack) == FAIL) return FAIL;
		a=pop(&arg_stack);
		if(a.type != 's'){
			errf("The x operator needs a string to execute.\n");
			return FAIL;
		}

		init_str_input((struct input_type){.string=a.s});
		struct value *subtokens = tokenize(a.s, false, false);
		enum returns status;
		if(tokens_errored(subtokens)){
			status = FAIL;
		}else{
			status = exec(subtokens, true);
		}

		free_tokens(subtokens);
		pop_input(&stack_input_s);

		if(status == FAIL) return status;
		break;
	default: assert(0); //Bad ops are handled in exec.
	}

	return PASS;
}

static enum returns exec(struct value *tokens, bool is_nested){
	enum returns status = PASS;
	struct value value;

	for(int kind_index=1; kind_index <= tokens[0].d; ++kind_index){
		switch(tokens[kind_index].kind){
		case OPEN_PAREN_TOK: //'('
			value = (struct value){.c='(', .type='c', .kind=OPEN_PAREN_TOK};
			push(value, &op_stack);
			break;

		case CLOSE_PAREN_TOK: //')'
			//Keep solving until you get to '('.
			while(peek(op_stack).c != '('){
				status = eval();
				if(status != PASS) return status;
			}
			pop(&op_stack);
			break;

		case MATH_OP_TOK: //'+', '*', etc...
			while(
				op_stack.i &&
				peek(op_stack).c != '(' &&
				get_preced(peek(op_stack).c) >= \
					get_preced(tokens[kind_index].c)
			){
				status = eval();
				if(status != PASS) return status;
			}

			if(push_and_check(tokens[kind_index], &op_stack) == FAIL){
				return FAIL;
			}
			break;

		//These cases are unary;
		//so they should run before the next token
		//(eg. a number) is evaluated.
		//Other commands allow pushing numbers to the stack,
		//before running said command.
		case FUNC_OP_TOK: //'p', 'f', 'c', etc... But not 's' or 'l'.
		case COMMENT:     //'#'
		case PUSH_TOK:    //3+4 5 #push addition result first.
			while(op_stack.i && peek(op_stack).c != '('){
				status = eval();
				if(status != PASS) return status;
			}

			push(tokens[kind_index], &op_stack);
			status = eval();
			if(status != PASS) return status;
			break;

		//The identifier can be any char<256 except \0.
		//Saving to a number requires that you don't iterate the loop.
		case SAVE_OP_TOK:
			//Before you save, you need to solve for the value.
			while(op_stack.i && peek(op_stack).c != '('){
				status = eval();
				if(status != PASS) return status;
			}

			//Save op.
			value = tokens[kind_index];
			if(push_and_check(value, &op_stack) == FAIL) return FAIL;
			++kind_index;

			//Save identifier.
			//You need to copy names for subsequent calls to exec.
			value = (struct value){
				.s = strdup(tokens[kind_index].s),
				.kind=IDENTIFIER,
				.type='s',
			};

			if(push_and_check(value, &arg_stack) == FAIL) return FAIL;

			status = eval();
			if(status != PASS) return status;
			continue;

		case LOAD_OP_TOK:
			//Save op.
			value = tokens[kind_index];
			if(push_and_check(value, &op_stack) == FAIL) return FAIL;
			++kind_index;

			//Save identifier.
			value = tokens[kind_index];
			if(push_and_check(value, &arg_stack) == FAIL) return FAIL;

			status = eval();
			if(status != PASS) return status;
			continue;

		case INT:
			value = tokens[kind_index];
			if(push_and_check(value, &arg_stack) == FAIL) return FAIL;
			break;
		case DOUBLE:
			value = tokens[kind_index];
			if(push_and_check(value, &arg_stack) == FAIL) return FAIL;
			break;
		case NEG_OP_TOK:
			value = (struct value){ .kind=NEG_OP_TOK, .type='c', .c='.' };
			if(push_and_check(value, &op_stack) == FAIL) return FAIL;
			break;

		case INPUT_TOK:;
			struct value input_toks[TOK_LINE_N];
			input_toks[0].kind = START_TOK;
			int tok_end_ind;

			init_file_input((struct input_type){.file=stdin});
			char *line = malloc(80);
			char *shifty_line = line;

			line = next_line_from_file(line);

			//num_query tokens can contain negation tokens.
			//This prevents -3^2p outputting 9.
			shifty_line = num_query(input_toks, 0, shifty_line);
			tok_end_ind = input_toks->d;
			pop_input(&stack_input_s);

			if(
				tokens_errored(input_toks) ||
				//If you only get '-', then .type would be 'c'.
				input_toks[tok_end_ind].type == 'c' || (
					shifty_line[0] != '\n' && shifty_line[0] != '\0'
				)
			){
				for(int i=0; i<30; ++i){
					if(line[i] == '\n') line[i] = '\0';
					else if(line[i] == '\0') break;
				}
				errf("Expected a number from input (%.30s).\n", line);
				free(line);
				return FAIL;
			}

			free(line);

			for(int i = 1; i < tok_end_ind; ++i){
				push(input_toks[i], &op_stack); //negation ops.
			}

			push(input_toks[tok_end_ind], &arg_stack);
			break;

		case STR_TOK: //'[', ']'
			//The tokenizer checks if a whole string was given.

			//If tokenization failed then you wouldn't be in this funct.
			++kind_index;
			value = (struct value){
				.s=strdup(tokens[kind_index].s), .type='s',
				.kind=STR_TOK
			};
			if(push_and_check(value, &arg_stack) == FAIL) return FAIL;
			++kind_index;
			break;

		default:
			errf("Bad token (%c).\n", tokens[kind_index].c);
		}
	} //for kind_index

	//-([3]x)
	//3 gets converted to an int in a nested call to exec.
	//However, '(' is on the stack when you get to this point in the code.
	//You need to evaluate '(' from the outer call to exec.
	if(is_nested) return PASS;

	while( op_stack.i ){
		status = eval();
		if(status != PASS) return status;
	}

	if(tokens[0].kind == END_TOK) return HALT;
	return PASS;
}

static _Noreturn void usage(char *prog){
	errf("Usage: %s [-e CMD] | [-f FILE]\n", prog);
	exit(1);
}


int main(int argc, char **argv){
	arg_stack = new_stack();
	op_stack = new_stack();
	stack_input_s = new_stack_input();
	struct value *tokens;
	char *line = NULL;
	enum returns exec_status = FAIL;

	table = new_hash_table();
	if(table  == NULL){
		perror(argv[0]);
		return 1;
	}

	tokens = init_tokens();
	if(tokens == NULL){
		perror(argv[0]);
		return 1;
	}

	if(argc == 3){
		if(strcmp("-e", argv[1]) == 0){
			line = argv[2];
			init_str_input( (struct input_type){.string=argv[2]} );

			while(line[0] != '\0'){
				tokens = tokenize(line, false, true);
				if(tokens_errored(tokens)){
					exec_status = FAIL;
					break;
				}

				exec_status = exec(tokens, false);
				if(
					exec_status == FAIL ||
					exec_status == HALT ||
					feof(stdin)
				){
					break;
				}

				line = next_line_from_str(line);
			}

			free_tokens(tokens);
			free_hash_table(table);
			del_stack(&arg_stack);
			del_stack(&op_stack);
			return exec_status == FAIL;

		}else if(strcmp("-f", argv[1]) == 0){
			FILE *fh = fopen(argv[2], "r");
			init_file_input( (struct input_type){.file=fh} );

			line = malloc(80);
			while(1){
				line = next_line_from_file(line);
				tokens = tokenize(line, false, true);

				if(tokens_errored(tokens)){
					exec_status = FAIL;
					break;
				}

				exec_status = exec(tokens, false);
				if(
					exec_status == FAIL ||
					exec_status == HALT ||
					feof(stdin)
				){
					break;
				}

				if(feof(fh)) break;
			}

			free_tokens(tokens);
			fclose(fh);
			free(line);
			free_hash_table(table);
			del_stack(&arg_stack);
			del_stack(&op_stack);
			return exec_status == FAIL;
		}else{
			usage(argv[0]);
		} //options (-e, -f, ...)
	}else if(argc == 1){
		if(!isatty(0)) line = malloc(80);
		init_file_input( (struct input_type){.file=stdin} );
	}else{
		usage(argv[0]);
	}

	while(1){
		if(isatty(0)){
			tokens = tokenize(line, true, true);
		}else{
			line = next_line_from_file(line);
			tokens = tokenize(line, false, true);
		}

		if(tokens_errored(tokens)){
			exec_status = FAIL;
			if(feof(stdin)) break;
			continue;
		}

		exec_status = exec(tokens, false);
		if( exec_status == HALT || feof(stdin)){
			break;
		}
	}

	free_tokens(tokens);
	free_hash_table(table);
	del_stack(&arg_stack);
	del_stack(&op_stack);
	return exec_status == FAIL;
}
