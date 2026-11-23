#ifndef SYNTAX_ANALYZER_H
#define SYNTAX_ANALYZER_H
#include "lexical_analyzer.h"

void parser(const char *symbol_table_file);

#define MAX_TOKENS 2048
extern Token symbol_table[MAX_TOKENS];
extern int symbol_table_count;

#endif