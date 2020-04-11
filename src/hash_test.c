#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "hashtable.h"

typedef struct 
{
    char *c;
} info;

static int compare (const void * a, const void * b) 
{
    int x = *(float *)a * 100;
    int y = *(float *)b * 100;

    return x - y; 
}

void print_table(hash_entry* hash_table)
{   
    hash_entry entry;
    info* data ;
    for(int i = 0; i < 10; i++)
    {   
        entry = hash_table[i];
        data = (info*) entry.data;
        if(data != NULL)
            printf("Key: %d, data: %s\n", entry.key, data->c);
        else
            printf("Key: %d, data: NULL\n", entry.key);
    }
}

int main(void)
{
    hash_entry* hash_table = hash_create(10);
    
    hash_entry test1, test2, test3;
    test1.data = (info*) malloc(sizeof(info));
    test1.key = 79;
    ((info*)test1.data)->c = (char* ) calloc(2, sizeof(char));
    strcpy(((info*)test1.data)->c, "a");
    
    test2.data = (info*) malloc(sizeof(info));
    test2.key = 9;
    ((info*)test2.data)->c = (char* ) calloc(2, sizeof(char));
    strcpy(((info*)test2.data)->c, "b");
    
    test3.data = (info*) malloc(sizeof(info));
    test3.key = 1239;
    ((info*)test3.data)->c = (char* ) calloc(2, sizeof(char));
    strcpy(((info*)test3.data)->c, "c"); 

    print_table(hash_table);

    // INSERT TESTS
    hash_insert(test1);
    hash_insert(test2);
    hash_insert(test3);
    printf("Insertions!\n");
    print_table(hash_table);

    assert(!strcmp(((info*)hash_find(79)->data)->c, "a"));
    assert(!strcmp(((info*)hash_find(9)->data)->c,  "b"));
    assert(!strcmp(((info*)hash_find(1239)->data)->c,  "c"));
    
    // POSITION TESTS
    assert(!strcmp(((info *) hash_table[9].data)->c, "a"));
    assert(!strcmp(((info *) hash_table[0].data)->c, "b"));
    assert(!strcmp(((info *) hash_table[1].data)->c, "c"));

    // DELETE TESTS]
    printf("Delete!\n");
    hash_entry* deleted = hash_delete(79);
    assert(deleted != NULL);
    assert(deleted->key == -1);
    
    print_table(hash_table);

    // float values[] = { 88.2, 56.42, 95.12, 2.32, 25.21 , 25.22};

    // printf("Before sorting the list is: \n");
    // for(int i = 0; i < 6; i++ ) 
    //     printf("%.2f ", values[i]);
    
    // qsort(values, 6, sizeof(float), compare);

    // printf("\nAfter sorting the list is: \n");
    // for(int i = 0; i < 6; i++ ) 
    //     printf("%.2f ", values[i]);

    // printf("\n");

    return 0;
}
