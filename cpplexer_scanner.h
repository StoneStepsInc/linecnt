/*
    linecnt - a source line counting utility

    Copyright (c) 2003-2021, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information
*/
#ifndef CPPLEXER_SCANNER_H
#define CPPLEXER_SCANNER_H

//
// CppFlexLexer token identifiers
//

#define TOKEN_EMPTY_LINE                     1
#define TOKEN_BRACE_LINE                     2

#define TOKEN_CODE_EOL                       3
#define TOKEN_C_COMMENT_EOL                  4
#define TOKEN_CPP_COMMENT_EOL                5
#define TOKEN_C_CPP_COMMENT_EOL              6
#define TOKEN_CODE_C_COMMENT_EOL             7
#define TOKEN_CODE_CPP_COMMENT_EOL           8
#define TOKEN_CODE_C_CPP_COMMENT_EOL         9

#define TOKEN_EOF                            1000

#endif // CPPLEXER_SCANNER_H
