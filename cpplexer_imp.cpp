/*
    linecnt - a source line counting utility

    Copyright (c) 2003-2020, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information
*/
#define __CPPLEXER_IMP_CPP

//
//
//

#include "cpplexer.inc"

///
/// Both file handles are passed to `yyrestart` verbatim.
///
CppFlexLexer::CppFlexLexer(FILE* arg_yyin, FILE* arg_yyout)
{
   yyrestart(arg_yyin);
}

CppFlexLexer::~CppFlexLexer(void)
{
}

int CppFlexLexer::yylex(void)
{
   return ::yylex();
}

const char *CppFlexLexer::YYText(void)
{
   return yytext;
}
