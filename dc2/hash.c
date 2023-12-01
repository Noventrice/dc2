///Technically, I could have made an array 256 chars long.
///I made this so that I could extend the variable names.
///At the moment, that requires a syntax change.

#include <stdio.h>  //perror
#include <stdlib.h> //malloc
#include <string.h> //strcmp, (strlen in main)
#include <stdbool.h>
#include "value.h"

#define HASH_MOD 31

//Initial value for struct value statements.
const struct value plain = { .d=0, .type='d' }; //Designated initializer.

//The table is basically `struct hash table[HASH_MOD]',
//except you cant save VLAs on the heap.
//So, an array of struct hash is malloc'd with the proper size.
typedef struct hash{
	struct hash *collisions;
	char *name; //Technically, I could have used `char name'.
	struct value value;
} hash;

hash *new_hash_leaf(){
	hash *leaf = malloc(sizeof(hash));
	if(!leaf){
		//perror(NULL);
		return leaf;
	}

	*leaf = (hash){.name=NULL, .value=plain, .collisions=NULL};
	return leaf;
}

hash *new_hash_table(){
	//hash h[HASH_MOD+1] = ... //VLA is saved on the stack (undesirable).
	hash *h = malloc(sizeof(hash) * (HASH_MOD + 1));
	if(!h){
		//perror(NULL);
		return h;
	}

	for(int i=0; i <= HASH_MOD; ++i){
		h[i] = (hash){.name=NULL, .value=plain, .collisions=NULL};
	}
	return h;
}

static int index_hash(char *name){
	int index = 0;
	for(char *p=name; *p; ++p) index += *p;
	return index % HASH_MOD;
}

hash *set_hash(hash *table, char *name, struct value value){
	hash *node = &(table[index_hash(name)]);
	bool match = false; //Does the name match a previous entry?

	if(node->name != NULL){
		while(
			//It was confusing to think about when the loop would break,
			//so I added this negation.
			!(//break when...
				//If you don't check the name before collisions,
				//then the || short-circuits too early to define match.
				(match = strcmp(name, node->name) == 0) ||
				node->collisions == NULL
			)
		){
			node = node->collisions;
		}
	}

	if(match){
		if(node->value.type == 's') free(node->value.s);
		if(value.type == 's'){
			char *s = malloc(strlen(value.s)+1);
			strcpy(s, value.s);
			node->value = (struct value){.s=s, .type='s'};
		}else{
			node->value=value;
		}
	
		return node;
	}else if(node->name){ //If you're at a used leaf...
		node->collisions = new_hash_leaf();
		if(node->collisions == NULL) return NULL;

		node = node->collisions;
	}

	//In the outer scope, name may change.
	char *s = malloc(strlen(name)+1);
	strcpy(s, name);

	*node = (hash){ .name=s, .value=value, .collisions=NULL};
	return node;
}

hash *lookup_hash(hash *table, char *name){
	hash *node = &(table[index_hash(name)]);

	if(node->name == NULL) return NULL;

	while(strcmp(node->name, name) != 0){
		if(node->collisions == NULL) return NULL;
		node = node->collisions;
	}

	return node;
}

static void free_hash_node(hash *node, hash *prev){
	free(node->name);
	free_value( node->value );
	if(prev == NULL) return;

	prev->collisions = node->collisions;
	free(node);
	node=NULL;
}

static void free_hash_table_(hash *node, hash *prev){
	if(node->collisions) free_hash_table_(node->collisions, node);
	free_hash_node(node, prev);
}

void free_hash_table(hash *table){
	for(int i=0; i<=HASH_MOD; ++i){
		if(table[i].collisions)
		free_hash_table_(table[i].collisions, &(table[i]));
		free_hash_node(&(table[i]), NULL);
	}
	free(table);
	table=NULL;
}

#ifdef HASH_MAIN
#include "errf.h"
#include <assert.h>

int main(int argc, char **argv){ //-q: stderr only (quiet)
	bool quiet_opt=false;
	if(argc == 2 && strcmp(argv[1], "-q")==0) quiet_opt=true;

	hash *table = new_hash_table();
	if(table == NULL){
		perror(argv[0]);
		return 1;
	}

	char *names[] = {"a", "b", "c"};
	////////////////////////////////////////////////////
	struct value value1, value2, value3;              //
	value1 = (struct value){.d=3, .type='d'};         //
	value2 = (struct value){.d=5, .type='d'};         //
	value3 = (struct value){.d=7, .type='d'};         //
	struct value values[] = {value1, value2, value3}; //Used multiple times.
	////////////////////////////////////////////////////

	///Sanity test:
	for(int i=0; i<3; ++i){
		//If you can't malloc...
		if( !set_hash(table, names[i], values[i]) ){
			perror(argv[0]);
			return 1;
		}
	}

	for(int i=0; i<3; ++i){
		hash *elem;
		if( (elem = lookup_hash(table, names[i])) ){
			assert(elem->value.d == values[i].d);
		}else{
			errf("%s: Variable \"%s\" is not recognized.\n",
				argv[0], names[i]
			);
		}
	}


	///Lookup fail test:
	assert(!lookup_hash(table, "d")); //Assume you fail to find d.

	///Value copy test:
	assert(
		lookup_hash(table, names[2])->value.d !=
		(values[2].d = 11)
	);
	values[2].d = 7;

	///Collision test:
	///'x'-HASH_MOD = 'Y' (assuming HASH_MOD=31 still).
	char var = 'x';
	char collide_name[2];
	collide_name[1] = '\0';

	for(int i=0; i<2; ++i){
		collide_name[0] = var - HASH_MOD*i;
		if( !set_hash(table, collide_name, values[i]) ){
			perror(argv[0]);
			return 1;
		}

		assert(
			lookup_hash(table, collide_name)->value.d ==
			values[i].d
		);
	}

	//Lookup x again.
	collide_name[0]=var;
	assert(
		lookup_hash(table, collide_name)->value.d == values[0].d
	);


	///Long name test (for fun):
	char *long_names[3] = {"ae", "bd", "cc"};
	for(int i=0; i<3; ++i){
		if( !set_hash(table, long_names[i], values[i]) ){
			perror(argv[0]);
			return 1;
		}

		assert(
			lookup_hash(table, long_names[i])->value.d ==
			values[i].d
		);
	}

	//Lookup the first two long names again.
	for(int i=0; i<2; ++i)
	assert(
		lookup_hash(table, long_names[i])->value.d ==
		values[i].d
	);

	/////////////////////////////////////////////////////////////////////////
	value1 = (struct value){.s="foo", .type='s'}; values[0] = value1; //
	value2 = (struct value){.s="bar", .type='s'}; values[1] = value2; //
	value3 = (struct value){.s="baz", .type='s'}; values[2] = value3; //
	/////////////////////////////////////////////////////////////////////////

	for(int j=1; j<=2; ++j) //Tests malloc/free on reassignment of strings.
	for(int i=0; i<3; ++i){
		if( !set_hash(table, long_names[i], values[i]) ){
			perror(argv[0]);
			return 1;
		}

		assert(
			0 == strcmp(
				lookup_hash(table, long_names[i])->value.s,
				values[i].s
			)
		);
	}

	///free table test:
	free_hash_table(table);

	if(!quiet_opt) puts("All hashing tests passed.");
}
#endif
