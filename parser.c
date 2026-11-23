#include <stdio.h>
#include "syntax_analyzer.h"

static int pos = 0; //current index in symbol_table[]
static Token current_token; // current token that is being checked
static int error_count = 0;

// moves to the next token in symbol_table[]
static void advance_parser(void) {
    while (pos < symbol_table_count) {
        current_token = symbol_table[pos++];

        if (current_token.type == TOK_NOISE ||
            current_token.type == TOK_COMMENT_LINE || 
            current_token.type == TOK_COMMENT_BLOCK) {
                continue; //ignore comments and noise words
            }
        return;
    }
    current_token.type = TOK_EOF;
}

// if syntax error occurs, skip tokens until a delimiter is found
static void panic_recover(void) {
     while (current_token.type != TOK_PERIOD &&
           current_token.type != TOK_COMMA &&
           current_token.type != TOK_KW_EXEUNT &&
           current_token.type != TOK_KW_PERCHANCE &&
           current_token.type != TOK_KW_UNTIL &&
           current_token.type != TOK_EOF) {
        advance_parser();
    }

    //if parser stopped on a period, consume it and ready for the next statement
    if (current_token.type == TOK_PERIOD) {
        advance_parser();
    }

}

//checks if current_token matches the grammar rules
static int match(TokenType expected, const char *err_msg) {
    if (current_token.type == expected) { //moves to next token if matches
        advance_parser(); 
        return 1;
    }

    printf("Syntax error near '%s' : %s\n", current_token.type == TOK_EOF ? "<EOF>" : current_token.lexeme, err_msg);
    error_count++;
    panic_recover(); //skips to the delimiter to check for the next statements
    return 0;
}

static int is_data_type(TokenType t) {
    return (t == TOK_KW_MEASURE || t == TOK_KW_FRACTION ||
            t == TOK_KW_TRUTH   || t == TOK_KW_TALE     ||
            t == TOK_KW_LETTER);
}

static void parse_expression(void);
static void parse_statement(void);

static void parse_declaration_stmt(void) {
    advance_parser(); //consumes "let"

    if (is_data_type(current_token.type)) {
        advance_parser();
    } else {
        match(TOK_KW_MEASURE, "Data type (measure, fraction, truth, tale, letter) expected after 'let'.");
        return;
    }

    if (!match(TOK_IDENTIFIER, "Identifier expected after data type.")) 
    return;
    if (!match(TOK_ASSIGN_BE, "Keyword 'be' expected after identifier."))
    return;

    parse_expression(); //reads the value expression

    match(TOK_PERIOD, "Period (.) expected to terminate statement.");
}

static void parse_input_stmt(void) {
    advance_parser();

    if (current_token.type == TOK_KW_MEMBER) {
        advance_parser();
        parse_expression();
        
        if (!match(TOK_KW_OF, "Keyword 'of' expected in member access.")) 
        return;
    
        if(!match(TOK_IDENTIFIER, "Identifier expected after 'of'."))
        return;
    } else {
        if (!match(TOK_IDENTIFIER, "Identifier expected after 'hearken'."))
        return;
    }
    match(TOK_PERIOD, "Period (.) expected to terminate statement.");
}

static void parse_output_stmt(void) {
    advance_parser();

    if (current_token.type == TOK_KW_TALE) {
        advance_parser();
    }

    parse_expression();
    match(TOK_PERIOD, "Period (.) expected to terminate statement.");
}

static void parse_assignment_stmt(void) {
    advance_parser();

    if(!match(TOK_ASSIGN_BECAME, "Keyword 'became' expected after identifier."))
    return;

    parse_expression();

    match(TOK_PERIOD, "Period (.) expected to terminate statement.");
}