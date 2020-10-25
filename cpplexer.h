/*
    linecnt - a source line counting utility

    Copyright (c) 2003-2020, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information
*/
#ifndef CPPLEXER_H
#define CPPLEXER_H

#include "cpplexer_scanner.h"

#include <cstdio>

///
/// @brief  Flex parser for counting lines in C-like languages.
///
/// @remark Flex generates C++ scanners with a comment that starts with
/// "The c++ scanner is a mess." and ends with "We will address this in
/// a future release of flex, or omit the C++ scanner altogether.", so
/// this class is named similar to the default `yyFlexLexer` naming
/// convention, but uses a C scanner instead and implements only the
/// absolute minimum set of methods to allow the line counting code to
/// read the sequence of tokens in source files that are being parsed.
/// 
class CppFlexLexer {
   public:
      /// Constructs a Flex scanner with the specified input and output files.
      CppFlexLexer(FILE* arg_yyin = nullptr, FILE* arg_yyout = nullptr);

      ~CppFlexLexer(void);
      
      /// Returns a poiner to the current token string.
      const char *YYText(void);

      /// Returns the next parsed token identifier.
      int yylex(void);
};

#endif // CPPLEXER_H
