/*
    linecnt - a source line counting utility

    Copyright (c) 2003-2021, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information
*/
#ifndef CPPLEXER_H
#define CPPLEXER_H

#include "cpplexer_scanner.h"

#include <cstdio>
#include <string_view>

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
      ///
      /// @brief  Source line count result structure.
      ///
      struct Result {
         unsigned int linecnt = 0;           ///< Total line count.
         unsigned int cmntcnt = 0;           ///< Count of lines with comments.
         unsigned int cppcnt = 0;            ///< Count of lines with C++ comments
         unsigned int ccnt = 0;              ///< Count of lines with C-style comments.
         unsigned int codecnt = 0;           ///< Count of lines with code.
         unsigned int bracecnt = 0;          ///< Count of lines with a single brace.
         unsigned int emptycnt = 0;          ///< Count of empty lines.
      };

   private:
      FILE  *srcfile;                        ///< Source file handle (may be `nullptr`).

   public:
      /// Constructs a Flex scanner with a handle to the specified source file.
      CppFlexLexer(FILE* &&arg_yyin = nullptr);

      /// Constructs a Flex scanner with the specified source text.
      CppFlexLexer(const std::string_view& source);

      /// If Flex scanner was constructed with a file handle, closes that handle.
      ~CppFlexLexer(void);
      
      /// Runs the source through the Flex scanner and returns resulting counts.
      Result CountLines(void);
};

#endif // CPPLEXER_H
