/*
    linecnt - a source line counting utility

    Copyright (c) 2003-2020, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information
*/
#ifndef __CPPLEXER_DECL_H
#define __CPPLEXER_DECL_H

#include <stdio.h>

///
/// @brief  Flex parser for counting lines in C++ like languages.
///
class CppFlexLexer {
   public:
      /// Constructs a Flex parser with the specified input and output files.
      CppFlexLexer(FILE* arg_yyin = NULL, FILE* arg_yyout = NULL );

      ~CppFlexLexer(void);
      
      /// Returns a poiner to the current token string.
      const char *YYText(void);

      /// Returns the next parsed token identifier.
      int yylex(void);
};

#ifndef __CPPLEXER_IMP_CPP
#undef yyFlexLexer
#endif // __CPPLEXER_IMP_CPP

//
// Lexer token identifiers
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

#endif // __CPPLEXER_DECL_H

