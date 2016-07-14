
#include "namespace.h"

///////////////////////// SYMBOL TABLE //////////////////////////////
void add_symbol_table_record(struct sections* section,
                             struct symbol_table_record *record) {
    HASH_ADD_KEYPTR(hh,section->symbol_table,record->symbol,strlen(record->symbol),record);
}

void add_symbol_table_pair(struct sections* section, char * key, uint64_t num) {

    struct symbol_table_record *record;
    record = (struct symbol_table_record*)malloc(sizeof(struct symbol_table_record));

    record->symbol = key;
    record->number = num;

    HASH_ADD_KEYPTR( hh, section->symbol_table, record->symbol, strlen(record->symbol), record );
}

uint64_t get_symbol_table_record(struct sections* section, char* key) {
    struct symbol_table_record *v;
    HASH_FIND_STR(section->symbol_table,key, v);

    return v->number;
}