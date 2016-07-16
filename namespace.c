
#include "namespace.h"

///////////////////////// SYMBOL TABLE //////////////////////////////


void namespace_start() {

     GHashTable* hash = g_hash_table_new(g_str_hash, g_str_equal);

     g_hash_table_insert(hash, "key1", "fuck yeah finally a hashmap");

     printf("There are %d keys in the hash\n", g_hash_table_size(hash));

     printf("Lookup key1 %s\n", g_hash_table_lookup(hash, "key1"));

     g_hash_table_remove(hash, "key1");

     printf("There are %d keys in the hash\n", g_hash_table_size(hash));

     g_hash_table_destroy(hash);

}