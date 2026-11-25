#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "syntax_analyzer.h"

static FILE *st_file = NULL;   // opened "Symbol Table.txt"
static Token current_token;   // current token that is being checked
static int error_count = 0;
static int row_index = 0; // tracks the row of symbol table
static int in_panic = 0;

// Forward declarations
static void parse_program(void);
static void parse_statement(void);

static void parse_declaration_stmt(void);
static void parse_input_stmt(void);
static void parse_output_stmt(void);
static void parse_assignment_stmt(void);
static void parse_conditional_stmt(void);
static void parse_iterative_stmt(void);

static void parse_expression(void);
static void parse_or_expr(void);
static void parse_and_expr(void);
static void parse_rel_expr(void);
static void parse_add_expr(void);
static void parse_mult_expr(void);
static void parse_unary_expr(void);
static void parse_primary_expr(void);

//removes extra spaces/newlines in the symbol table 
static void trim(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' ||
                       str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[--len] = '\0';
    }

    size_t start = 0;
    while (str[start] == ' ' || str[start] == '\t') {
        start++;
    }

    if (start > 0) {
        memmove(str, str + start, strlen(str + start) + 1);
    }

}

//converts string from file to its token type in enum
static TokenType string_to_token_type(const char *str) {
    //chcecks every enum value
    for (int t = TOK_PERIOD; t <= TOK_COMMENT_BLOCK_CLOSER; t++) {
        if (strcmp(str, token_type_to_string((TokenType)t)) == 0) {
            return (TokenType)t; // if match found return the value
        }
    }
    return TOK_ERROR; // fallback if unknown
}

// Moves to the next token in symbol_table[], skipping noise & comments
static void advance_parser(void) {
    char line[1024];

    //reads lines until it find a non-noise/non-comment token or EOF
    while (st_file != NULL && fgets(line,sizeof(line), st_file) != NULL) {
        //finds the '|' separating the token type and lexeme
        char *bar1 = strchr(line, '|');
        if (!bar1) continue;

        //looks up this row's line number in .fol 
        int tok_line = (row_index < symbol_table_count) ? symbol_table[row_index].line : 1;
        row_index++;

    //extract token type
    char type_str[64];
    size_t len = (size_t)(bar1 - line); //counts the length of the token type before '|'
    if (len >= sizeof(type_str)) len = sizeof(type_str) - 1;
    strncpy(type_str, line, len);
    type_str[len] = '\0'; 
    trim(type_str);

    //extract lexeme
    char lex_str[256];
    strncpy(lex_str, bar1 + 1, sizeof(lex_str) - 1);
    lex_str[sizeof(lex_str) - 1] = '\0';
    trim(lex_str);

    //convert strings to their token types
    TokenType type = string_to_token_type(type_str);

    //skip noise words and comment tokens

    if (type == TOK_NOISE                ||
            type == TOK_COMMENT_OPENER       ||
            type == TOK_COMMENT_LINE         ||
            type == TOK_COMMENT_CLOSER       ||
            type == TOK_COMMENT_BLOCK_OPENER ||
            type == TOK_COMMENT_BLOCK        ||
            type == TOK_COMMENT_BLOCK_CLOSER) {
            continue;
        }

    //store the token type in current_token
    current_token.type = type;
    strcpy(current_token.lexeme, lex_str);
    current_token.line = tok_line;
    return;
    }

    // fgets() returned NULL -> reached the end of the Symbol Table file
    current_token.type = TOK_EOF;
    current_token.lexeme[0] = '\0';
    current_token.line = (row_index < symbol_table_count) ? symbol_table[row_index].line : 1;
}

// If a syntax error occurs, skip tokens until a safe delimiter is found
static void panic_recover(void) {
    in_panic = 1;
    while (current_token.type != TOK_PERIOD &&
           current_token.type != TOK_COMMA &&
           current_token.type != TOK_KW_EXEUNT &&
           current_token.type != TOK_KW_PERCHANCE &&
           current_token.type != TOK_KW_OTHERWISE &&
           current_token.type != TOK_KW_UNTIL &&
           current_token.type != TOK_EOF) {
        advance_parser();
    }

    // If parser stopped on a period, consume it to get ready for the next statement
    if (current_token.type == TOK_PERIOD) {
        advance_parser();
    }
}

// Checks if current_token matches the expected grammar rule
static int match(TokenType expected, const char *err_msg) {
    if (current_token.type == expected) {
        in_panic = 0;
        advance_parser(); // moves to next token if it matches
        return 1;
    }

    if (in_panic) return 0;

    printf("Syntax error on line %d near '%s': %s\n", current_token.line, current_token.type == TOK_EOF ? "<EOF>" : current_token.lexeme, err_msg);
    error_count++;
    panic_recover(); // skips to delimiter to check the next statements
    return 0;
}

static int is_data_type(TokenType t) { //for declaration statement
    return (t == TOK_KW_MEASURE || t == TOK_KW_FRACTION ||
            t == TOK_KW_TRUTH   || t == TOK_KW_TALE     ||
            t == TOK_KW_LETTER);
}


// Entry point called from folio.c
void parser(const char *symbol_table_file) {
    st_file = fopen(symbol_table_file, "r");

    if (st_file == NULL) { //checks if file exists
        perror(symbol_table_file);
        return;
    }

    //skips the two header lines
    char header[1024];
    fgets(header, sizeof(header), st_file);
    fgets(header, sizeof(header), st_file);

    error_count = 0;  
    row_index = 0;

    advance_parser(); //checks first token from file
    parse_program(); //validates all statements until TOK_EOF
    fclose(st_file);
    if (error_count == 0) {
        printf("\nSyntax analysis passed with 0 errors\n");
    } else {
        printf("\nSyntax analysis finished with %d error/s.\n", error_count);
    }
}

// <program> ::= { <statement> } EOF
static void parse_program(void) {
    while (current_token.type != TOK_EOF) {
        parse_statement();
    }
}

// Checks first token and routes to its corresponding grammar rule
static void parse_statement(void) {
    in_panic = 0;
    switch (current_token.type) {
        case TOK_KW_LET:
            parse_declaration_stmt();
            break;
        case TOK_KW_HEARKEN:
            parse_input_stmt();
            break;
        case TOK_KW_SPEAK:
            parse_output_stmt();
            break;
        case TOK_KW_ALAS:
            parse_conditional_stmt();
            break;
        case TOK_KW_WHILST:
            parse_iterative_stmt();
            break;
        case TOK_KW_REPEAT:
            parse_iterative_stmt();
            break;
        case TOK_IDENTIFIER:
            parse_assignment_stmt();
            break;
        default:
            printf("Syntax error on line %d near '%s': Unexpected token in statement.\n",
                   current_token.line,
                   current_token.type == TOK_EOF ? "<EOF>" : current_token.lexeme);
            error_count++;
            {
                panic_recover();
                // contiunues if stopped by a delimiter
                if (current_token.type == TOK_COMMA    ||
                current_token.type == TOK_KW_EXEUNT    ||
                current_token.type == TOK_KW_PERCHANCE ||
                current_token.type == TOK_KW_OTHERWISE ||
                current_token.type == TOK_KW_UNTIL) {
                advance_parser();
                }
            }
            break;
    }
}

//statement grammar rules

static void parse_declaration_stmt(void) {
    advance_parser(); // consumes "let"

    if (current_token.type == TOK_KW_LEGION){
        advance_parser(); //consumes "legion"

        if (!match(TOK_IDENTIFIER, "Identifier expected after 'legion'."))
        return;

        if (!match(TOK_KW_HOLD, "Keyword 'hold' expected after 'legion <identifier>'."))
        return;

        parse_expression();
        match(TOK_PERIOD, "Period (.) expected to terminate statement.");
        return;
    }

    if (is_data_type(current_token.type)) {
        advance_parser();
    } else {
        match(TOK_KW_MEASURE, "Data type (measure, fraction, truth, tale, letter) expected after 'let'.");
        return;
    }

    if (!match(TOK_IDENTIFIER, "Identifier expected after data type.")) return;
    if (!match(TOK_ASSIGN_BE, "Keyword 'be' expected after identifier.")) return;

    parse_expression(); // reads the value expression

    match(TOK_PERIOD, "Period (.) expected to terminate statement.");
}

static void parse_input_stmt(void) {
    advance_parser(); // consumes "hearken"

    if (current_token.type == TOK_KW_MEMBER) {
        // array member input: hearken member <expression> of <identifier>.
        advance_parser();
        parse_expression();

        if (!match(TOK_KW_OF, "Keyword 'of' expected in member access.")) return;
        if (!match(TOK_IDENTIFIER, "Identifier expected after 'of'.")) return;
    } else {
        // normal variable: hearken <identifier>.
        if (!match(TOK_IDENTIFIER, "Identifier expected after 'hearken'.")) return;
    }

    match(TOK_PERIOD, "Period (.) expected to terminate statement.");
}

static void parse_output_stmt(void) {
    advance_parser(); // consumes "speak"

    if (current_token.type == TOK_KW_TALE) {
        advance_parser(); // consumes optional "tale"
    }

    parse_expression();
    match(TOK_PERIOD, "Period (.) expected to terminate statement.");
}

static void parse_assignment_stmt(void) {
    advance_parser(); // consumes identifier

    if (!match(TOK_ASSIGN_BECAME, "Keyword 'became' expected after identifier.")) return;

    parse_expression();
    match(TOK_PERIOD, "Period (.) expected to terminate statement.");
}

static void parse_conditional_stmt(void) {
    advance_parser(); // consumes "alas"
    parse_expression();

    if (!match(TOK_COMMA, "Comma (,) expected to open conditional or loop block.")) return;

    while (current_token.type != TOK_KW_PERCHANCE &&
           current_token.type != TOK_KW_OTHERWISE &&
           current_token.type != TOK_KW_EXEUNT &&
           current_token.type != TOK_EOF) {
        parse_statement();
    }

    while (current_token.type == TOK_KW_PERCHANCE) {
        advance_parser(); // consumes "perchance"
        if (!match(TOK_COMMA, "Comma (,) expected after 'perchance'.")) return;
        parse_expression();
        if (!match(TOK_COMMA, "Comma (,) expected to open conditional or loop block.")) return;

        while (current_token.type != TOK_KW_PERCHANCE &&
               current_token.type != TOK_KW_OTHERWISE &&
               current_token.type != TOK_KW_EXEUNT &&
               current_token.type != TOK_EOF) {
            parse_statement();
        }
    }

    if (current_token.type == TOK_KW_OTHERWISE) {
        advance_parser(); // consumes "otherwise"
        if (!match(TOK_COMMA, "Comma (,) expected to open conditional or loop block.")) return;

        while (current_token.type != TOK_KW_EXEUNT &&
               current_token.type != TOK_EOF) {
            parse_statement();
        }
    }

    if (!match(TOK_KW_EXEUNT, "'exeunt.' expected to close block.")) return;
    match(TOK_PERIOD, "Period (.) expected after 'exeunt'.");
}

static void parse_iterative_stmt(void) {
    if (current_token.type == TOK_KW_WHILST) {advance_parser(); // consumes "whilst"
    parse_expression();

    if (!match(TOK_COMMA, "Comma (,) expected to open conditional or loop block.")) return;

    while (current_token.type != TOK_KW_EXEUNT &&
           current_token.type != TOK_EOF) {
        parse_statement();
    }

    if (!match(TOK_KW_EXEUNT, "'exeunt.' expected to close block.")) return;
    match(TOK_PERIOD, "Period (.) expected after 'exeunt'.");
    } else if (current_token.type == TOK_KW_REPEAT) {
        advance_parser(); // consumes "repeat"

        if (current_token.type == TOK_COMMA) {
            advance_parser();
        }

        while (current_token.type != TOK_KW_UNTIL && current_token.type != TOK_EOF) {
            parse_statement();
        }

        if (!match(TOK_KW_UNTIL, "Keyword 'until' expected to terminate repeat loop."))
        return;
        parse_expression();
        match(TOK_PERIOD, "Period (.) expected to terminate statement.");
    }
}


//expression grammar rules (lowest to highest precedence)

// Top-level expression
static void parse_expression(void) {
    parse_or_expr();
}

// Logical OR
static void parse_or_expr(void) {
    parse_and_expr();

    while (current_token.type == TOK_LOG_OR) {
        advance_parser(); // consumes OR
        parse_and_expr();
    }
}

// Logical AND
static void parse_and_expr(void) {
    parse_rel_expr();

    while (current_token.type == TOK_LOG_AND) {
        advance_parser(); // consumes AND
        parse_rel_expr();
    }
}

// Relational operators
static void parse_rel_expr(void) {
    parse_add_expr();

    while (current_token.type == TOK_REL_MATCHES      ||
           current_token.type == TOK_REL_BELIES       ||
           current_token.type == TOK_REL_GREATER_THAN ||
           current_token.type == TOK_REL_LESS_THAN    ||
           current_token.type == TOK_REL_AT_LEAST     ||
           current_token.type == TOK_REL_AT_MOST) {
        advance_parser(); // consumes relational operator
        parse_add_expr();
    }
}

// Addition, subtraction
static void parse_add_expr(void) {
    parse_mult_expr();

    while (current_token.type == TOK_OP_COMBINED ||
           current_token.type == TOK_OP_WITHOUT) {
        advance_parser(); // consumes operator
        parse_mult_expr();
    }
}

// Multiplication, division, modulo
static void parse_mult_expr(void) {
    parse_unary_expr();

    while (current_token.type == TOK_OP_MAGNIFIED ||
           current_token.type == TOK_OP_PARTED    ||
           current_token.type == TOK_OP_REMNANT   ||
           current_token.type == TOK_OP_GROUNDED  ||
           current_token.type == TOK_OP_VAULTED) {
        advance_parser(); // consumes operator
        parse_unary_expr();
    }
}

// Unary operators
static void parse_unary_expr(void) {
    if (current_token.type == TOK_LOG_NAY) {
        advance_parser(); // consumes 'nay'
        parse_unary_expr();
    } else {
        parse_primary_expr();
    }
}

// Operands (literals, identifiers, fraction, member access)
static void parse_primary_expr(void) {
    switch (current_token.type) {
        case TOK_IDENTIFIER:
        case TOK_TALE_LIT:
        case TOK_LETTER_LIT:
        case TOK_AYE:
        case TOK_NAUGHT:
            advance_parser();
            break;

        case TOK_MEASURE_LIT:
            advance_parser();

            if (current_token.type == TOK_KW_POINT) { // for fractions (3 point 14)
                advance_parser();
                match(TOK_MEASURE_LIT, "Measure literal expected after 'point'.");
            }
            break;

        case TOK_KW_MEMBER:
            advance_parser(); // consumes "member"
            parse_expression();

            if (!match(TOK_KW_OF, "Keyword 'of' expected in member access.")) return;
            match(TOK_IDENTIFIER, "Identifier expected after 'of'.");
            break;

        default:
            printf("Syntax error on line %d near '%s': Valid expression expected.\n",
                   current_token.line,
                   current_token.type == TOK_EOF ? "<EOF>" : current_token.lexeme);
            error_count++;
            panic_recover();
            break;
    }
}
