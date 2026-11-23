#ifndef LEXICAL_ANALYZER_H
#define LEXICAL_ANALYZER_H

#include <stddef.h>

typedef enum {
    // Delimiters 
    TOK_PERIOD,
    TOK_COMMA,
    
    // Literals 
    TOK_MEASURE_LIT,
    TOK_TALE_LIT,
    TOK_LETTER_LIT,
    TOK_AYE,
    TOK_NAUGHT,
    
    // Identifiers 
    TOK_IDENTIFIER,
    
    // Primitive Types 
    TOK_KW_MEASURE,
    TOK_KW_FRACTION,
    TOK_KW_LETTER,
    TOK_KW_TRUTH,
    TOK_KW_POINT,
    
    // Collection Types 
    TOK_KW_LEGION,
    TOK_KW_TALE,
    TOK_KW_HOLD,
    TOK_KW_MEMBER,
    TOK_KW_OF,
    
    // Control Flow 
    TOK_KW_ALAS,
    TOK_KW_PERCHANCE,
    TOK_KW_WHILST,
    TOK_KW_REPEAT,
    TOK_KW_UNTIL,
    TOK_KW_EXEUNT,
    
    // I/O 
    TOK_KW_HEARKEN,
    TOK_KW_SPEAK,
    
    // Declarations & Assignment 
    TOK_KW_LET,
    TOK_ASSIGN_BE,
    TOK_ASSIGN_BECAME,
    
    // Arithmetic Operators 
    TOK_OP_COMBINED,
    TOK_OP_WITHOUT,
    TOK_OP_MAGNIFIED,
    TOK_OP_PARTED,
    TOK_OP_REMNANT,
    
    // Relational Operators 
    TOK_REL_MATCHES,
    TOK_REL_BELIES,
    TOK_REL_GREATER_THAN,
    TOK_REL_LESS_THAN,
    TOK_REL_AT_LEAST,
    TOK_REL_AT_MOST,
    
    // Logical Operators 
    TOK_LOG_AND,
    TOK_LOG_OR,
    TOK_LOG_NAY,
    
    // Special 
    TOK_EOF,
    TOK_ERROR,

    // Noise
    TOK_NOISE,

    // Comments
    TOK_COMMENT_LINE,
    TOK_COMMENT_BLOCK
} TokenType;

// Token Structure 
typedef struct {
    TokenType type;
    char lexeme[256];
} Token;

// Lexer State 
typedef struct {
    const char *src;
    size_t pos;
    size_t start_pos;
} Lexer;


void lexer_init(Lexer *l, const char *source);
Token get_next_token(Lexer *l);
const char *token_type_to_string(TokenType type);
void lexer(const char *filename);

#define MAX_TOKENS 2048
extern Token symbol_table[MAX_TOKENS];
extern int symbol_table_count;


#endif