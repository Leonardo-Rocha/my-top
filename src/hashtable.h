#ifndef _HASHTABLE_H
#define _HASHTABLE_H

// data must be allocated and freed manually
typedef struct 
{
    int key;
    void * data;
} hash_entry; 

// returns a key % hash_size
int get_hash_code(int key);

// creates a hash table of given size
hash_entry* hash_create(int size);

// inserts a new element 
hash_entry* hash_insert(hash_entry new_entry);

// find an entry by it's key and returns a pointer to it on success; NULL if not found.
hash_entry* hash_find(int key);

//TODO: implement this
//hash_entry* hash_insert_empty(int key);

/* finds an entry by it's key and invalidates it, returns it's pointer on success; NULL if not found.
 * The user MUST free the data after calling delete.
 */
hash_entry* hash_delete(int key);

#endif
