#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>


//tok_kinds will be an enum but it is handy to reverse-lookup by identifier.
//So, here is an array macro which is inserted into the body of the enum.
#define TOK_KINDS_ARRAY           \
	/* ////////////////////////////////////////////////////////////////
	** //Negation and subtraction are checked by if x <= NEG_OP_TOK. //
	** ////////////////////////////////////////////////////////////////
	** S-  Negation (S represents the start of the expression).
	** (-  Negation. Forces subtraction to be negation!
	** +-  Negation.
	** +-- Negation. */           \
	START_TOK,                    \
	OPEN_PAREN_TOK,               \
	MATH_OP_TOK,                  \
	NEG_OP_TOK,                   \
	/*
	** p-   Subtraction, p(- would be negation.
	** d-   Subtraction, by extension of the rule above.
	** ]-   Subtraction, but maybe should be negation.
	**          Actually, +[str]- makes sense for subtraction.
	**              + was planned as a string conversion operator,
	**              However, suffixing x already does this conversion.
	**              This may change in the future, though.
	**          If you want negation use ](-3).
	** x-   Subtraction. `lfx-2 #Subtract 2 from f().'
	** sxa- Subtraction.
	** 3-   Subtraction.
	** )-   Subtraction. */       \
	FUNC_OP_TOK,                  \
	SAVE_OP_TOK,                  \
	LOAD_OP_TOK,                  \
	CLOSE_PAREN_TOK,              \
	                              \
	STR_TOK,                      \
	IDENTIFIER,                   \
	UNKNOWN,                      \
	INPUT_TOK,                    \
	PUSH_TOK,                     \
	                              \
	/* FINAL TOKENS:
	** COMMENT, ERROR_TOK, EOL_TOK, and END_TOK are final tokens.
	** RADIX, and RADIX_ERROR get replaced with ERROR_TOK.
	** final tokens replace START_TOK at tokens[0].kind,
	** so that you can find them easily.
	** With this design,
	** you will only need to add 1 token to the end of the
	** struct value *tokens list for each token.
	** For, example you would otherwise have to add COMMENT and END_TOK
	** to the end of the list.
	** Since its at the beginning, you can also put the length of the list
	** inside `tokens[0].d'.
	** In practice, it's actually stored as the maximum index to the list,
	** rather than the length. */ \
	END_TOK,                      \
	EOL_TOK,                      \
	ERROR_TOK, /*Unrecoverable.*/ \
	COMMENT,                      \
	                              \
	                              \
	/* Increment from INT or DOUBLE every time you find a `.'.
	** Stop at RADIX_ERROR.
	*/ \
	RADIX,       /* . or -. */                 \
	INT,                                       \
	DOUBLE,      /* INT.INT or INT. or .INT */ \
	RADIX_ERROR, /* INT.INT.INT or .. */       \
	/**********************************************************************
	**Currently, number invalidity is checked with foo<RADIX.
	**So, It would be better to add stuff before RADIX if it isn't a number.
	**However, you can add after this point if you can guarantee
	**that the previous token ended with '\0'.
	**********************************************************************/\
	\
	TOK_KINDS_LEN, /*Length of TOK_KINDS_ARRAY. Not an actual kind.*/

enum tok_kinds{
	TOK_KINDS_ARRAY
};

extern char *kinds_array[TOK_KINDS_LEN];
void init_kinds_array();

struct value{
	union { //anonymous union
		char c;
		double g;
		int64_t d;
		char *s;
	};
	enum tok_kinds kind; //hash.c doesn't need to test this.
	//\0: init, d: int, s: string, c: operator and identifier, g: double
	char type;
};

extern void free_value(struct value value);
#endif
