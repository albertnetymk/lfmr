#include "mr.h"
#include "test.h"
#include "list.h"
#include "util.h"

struct hash_table
{
	long nbuckets;
	struct list **buckets;
};

void hash_table_init(struct hash_table **h, long buckets)
{
	int i;

	(*h) = (struct hash_table *)mapmem(sizeof(struct hash_table));
	(*h)->nbuckets = buckets;
	(*h)->buckets = 
		(struct list **)mapmem(sizeof(struct list *) * buckets);

	for (i = 0; i < buckets; i++)
		list_init( &(((*h)->buckets)[i]) );
}

void hash_table_destroy(struct hash_table **h)
{
	int i;

	for (i = 0; i < (*h)->nbuckets; i++)
		list_destroy( &(((*h)->buckets)[i]) );
	free((*h)->buckets);
	free(*h);
}

int hash_search(struct hash_table *h, long key)
{
	return search(h->buckets[key % h->nbuckets], key);
}

int hash_delete(struct hash_table *h, long key)
{
	return delete(h->buckets[key % h->nbuckets], key);
}

int hash_insert(struct hash_table *h, long key)
{
	return insert(h->buckets[key % h->nbuckets], key);
}
