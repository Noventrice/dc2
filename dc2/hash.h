#include "value.h"

typedef struct hash{
	struct hash *collisions;
	char *name; //Technically, I could have used char name.
	struct value value;
} hash;

extern struct value plain;

hash *new_hash_leaf();
hash *new_hash_table();
hash *set_hash(hash *table, char *name, struct value value);
hash *lookup_hash(hash *table, char *name);
void free_hash_table(hash *table);
