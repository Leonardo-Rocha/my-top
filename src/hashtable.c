#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"

hash_entry* hash_table;
int hash_size;


int get_hash_code(int key) { return hash_size == 0 ? 0 : key % hash_size; }

hash_entry* hash_create(int size)
{
    hash_size = size;
    hash_table = (hash_entry*) calloc(size, sizeof(hash_entry));
    while(size--)
        hash_table[size].key = -1;

    return hash_table; 
}

hash_entry* hash_insert_update(hash_entry new_entry)
{   
    int search_size;
    int hash_code = get_hash_code(new_entry.key);
    
    // finds the proper key to update it
    for(search_size = 0; hash_table[hash_code].key != new_entry.key &&
         search_size < hash_size; search_size++)
    {
        hash_code++;
        hash_code = hash_code % hash_size;
    }
    
    // finds empty spaces if the correct key wasn't found
    if(hash_table[hash_code].key != new_entry.key)
    {
        hash_code = get_hash_code(new_entry.key);
        for(search_size = 0;  hash_table[hash_code].key != -1 &&
            search_size < hash_size; search_size++)
        {
            hash_code++;
            hash_code = hash_code % hash_size;
        }
    }
        

    if(search_size == hash_size) 
        return NULL;
    else 
    {
        hash_table[hash_code] = new_entry;
        return &hash_table[hash_code]; 
    }
}

hash_entry* hash_find(int key)
{
    int search_size;
    int hash_code = get_hash_code(key);

    for(search_size = 0; hash_table[hash_code].key != key && 
        search_size < hash_size; search_size++)
    {
        hash_code++;
        hash_code = hash_code % hash_size;
    }

    
    if(search_size == hash_size) 
        return NULL;
    else 
        return &hash_table[hash_code]; 
}

hash_entry* hash_delete(int key)
{
    hash_entry* entry = hash_find(key);

    if(entry != NULL) 
    {   
        entry->key = -1;
        return entry;    
    }
    else
        return NULL;   
}