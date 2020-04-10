#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "hashtable.h"

static int compare (const void * a, const void * b) 
{
    int x = *(float *)a * 100;
    int y = *(float *)b * 100;

    return x - y; 
}

int main(void)
{
    hash_entry* hash_table = hash_create(10);
    
    hash_entry test1, test2, test3;
    test1.data = (char *) malloc(2 * sizeof(char));
    test1.key = 79;
    test1.data = "a";
    test2.data = (char *) malloc(2* sizeof(char));
    test2.key = 9;
    test2.data = "b";
    test3.data = (char *) malloc(2* sizeof(char));
    test3.key = 1239;
    test3.data = "c"; 

    // INSERT TESTS
    hash_insert_update(test1);
    hash_insert_update(test2);
    hash_insert_update(test3);

    assert(!strcmp((char *) hash_find(79)->data, "a"));
    assert(!strcmp((char *) hash_find(9)->data,  "b"));
    assert(!strcmp((char *) hash_find(1239)->data,  "c"));
    
    // UPDATE TESTS
    test1.data = "A";
    test2.data = "B";
    test3.data = "C";
    hash_insert_update(test1);
    hash_insert_update(test2);
    hash_insert_update(test3);
    assert(!strcmp((char *) hash_find(79)->data,     "A"));
    assert(!strcmp((char *) hash_find(9)->data,      "B"));
    assert(!strcmp((char *) hash_find(1239)->data,   "C"));
    
    // POSITION TESTS
    assert(!strcmp((char *) hash_table[9].data, "A"));
    assert(!strcmp((char *) hash_table[0].data, "B"));
    assert(!strcmp((char *) hash_table[1].data, "C"));

    // DELETE TESTS
    hash_entry* deleted = hash_delete(79);
    assert(deleted != NULL);
    assert(deleted->key == -1);
    
    // STRANGE BEHAVIORS
    test2.data = "Z";
    hash_insert_update(test2);
    assert(!strcmp((char *) hash_table[0].data, "Z"));
    assert(!strcmp((char *) hash_find(9)->data, "Z"));
    
    float values[] = { 88.2, 56.42, 95.12, 2.32, 25.21 , 25.22};

    printf("Before sorting the list is: \n");
    for(int i = 0; i < 6; i++ ) 
        printf("%.2f ", values[i]);
    
    qsort(values, 6, sizeof(float), compare);

    printf("\nAfter sorting the list is: \n");
    for(int i = 0; i < 6; i++ ) 
        printf("%.2f ", values[i]);

    printf("\n");

    return 0;
}
