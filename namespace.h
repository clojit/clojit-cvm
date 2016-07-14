#ifndef _NAMESPACE_H_
#define _NAMESPACE_H_

void add_symbol_table_record(struct sections* section, struct symbol_table_record *record);
void add_symbol_table_pair(struct sections* section, char * key, uint64_t num);
uint64_t get_symbol_table_record(struct sections* section, char* key);

#endif