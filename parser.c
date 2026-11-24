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
        
        //array member input
        if (!match(TOK_KW_OF, "Keyword 'of' expected in member access.")) 
        return;
    
        if(!match(TOK_IDENTIFIER, "Identifier expected after 'of'."))
        return;
    } else {
        //normal variable: hearken <identifier>
        if (!match(TOK_IDENTIFIER, "Identifier expected after 'hearken'."))
        return;
    }
    match(TOK_PERIOD, "Period (.) expected to terminate statement.");
}

static void parse_output_stmt(void) {
    advance_parser();

    if (current_token.type == TOK_KW_TALE) {
        advance_parser(); //consumes "speaks"
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

static void parse_conditional_stmt(void) {
    advance_parser(); //consume "alas"
    parse_expression();

    if (!match(TOK_COMMA, "Comma (,) expected to open conditional or loop block.")) 
    return;

    while (current_token.type != TOK_KW_PERCHANCE && current_token.type != TOK_KW_EXEUNT && current_token.type != TOK_EOF) {
        parse_statement();
    }

    if (current_token.type == TOK_KW_PERCHANCE) {
        advance_parser(); //consumes "perchance"
        if (!match(TOK_COMMA, "Comma (,) expected to open conditional or loop block."))
        return;

        while (current_token.type != TOK_KW_EXEUNT &&
               current_token.type != TOK_EOF) {
            parse_statement();
        }
    }
    if (!match(TOK_KW_EXEUNT, "'exeunt.' expected to close block.")) 
    return;
    
    match(TOK_PERIOD, "Period (.) expected after 'exeunt'.");
}

static void parse_iterative_stmt(void) {
    advance_parser(); // consume "whilst"
    parse_expression();

    if (!match(TOK_COMMA, "Comma (,) expected to open conditional or loop block.")) 
    return;

    while (current_token.type != TOK_KW_EXEUNT && current_token.type != TOK_EOF) {
        parse_statement();
    }

    if (!match(TOK_KW_EXEUNT, "'exeunt.' expected to close block.")) return;
    
    match(TOK_PERIOD, "Period (.) expected after 'exeunt'.");
}

// expressions 

//operands (literals, identfiers, fraction, member access)
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

            if(current_token.type == TOK_KW_POINT) { //for fractions (3 point 14)
                advance_parser();
                match(TOK_MEASURE_LIT, "Measure literal expected after 'point'.");
            }
        break;

        case TOK_KW_MEMBER:
            advance_parser(); //consumes "member"
            parse_expression();
            
            if (!match(TOK_KW_OF, "Keyword 'of' expected in member access.")) 
            return;
            match(TOK_IDENTIFIER, "Identifier expected after 'of'.");
            break;
        
        default:
            printf("Syntax error near '%s' : Valid expression expected.\n", current_token.type == TOK_EOF ? "<EOF>" : current_token.lexeme);
            error_count++;
            panic_recover();
            break;
    }
}

//unary operators
static void parse_unary_expr(void) {
    if (current_token.type == TOK_LOG_NAY) {
        advance_parser(); //consumes 'nay'
        parse_unary_expr();
    } else {
        parse_primary_expr();
    }
}

//multiplication, division, modulo
static void parse_mult_expr(void) {
    parse_unary_expr();

    while (current_token.type == TOK_OP_MAGNIFIED || current_token.type == TOK_OP_PARTED || current_token.type == TOK_OP_REMNANT) {
        advance_parser(); // consumes operator
        parse_unary_expr();
    }
}

//addition, subtraction
static void parse_add_expr(void) {
    parse_mult_expr();

    while (current_token.type == TOK_OP_COMBINED || current_token.type == TOK_OP_WITHOUT) {
        advance_parser(); //consumes operator
        parse_mult_expr(); 
    }
}

//relational operators
static void parse_rel_expr(void) {
    parse_add_expr();

    while (current_token.type == TOK_REL_MATCHES || current_token.type == TOK_REL_BELIES || current_token.type == TOK_REL_GREATER_THAN || current_token.type == TOK_REL_LESS_THAN || current_token.type == TOK_REL_AT_LEAST || current_token.type == TOK_REL_AT_MOST) {
        advance_parser(); // consumes relational operator
        parse_add_expr();
    }
}

//logical AND
static void parse_and_expr(void) {
    parse_rel_expr();

    while (current_token.type == TOK_LOG_AND) {
        advance_parser(); // consunmes AND
        parse_rel_expr();
    }
}

static void parse_or_expr(void) {
    parse_and_expr();

    while (current_token.type == TOK_LOG_OR) {
        advance_parser(); //consumes OR
        parse_and_expr();
    }
}

//top level expression
static void parse_expression(void) {
    parse_or_expr();
}
