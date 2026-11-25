#include "lexical_analyzer.h"
#include "syntax_analyzer.h"

int main(){
    const char *input = "folio_input.fol";

    lexer(input);
    parser("Symbol Table");
    
    return 0;
}