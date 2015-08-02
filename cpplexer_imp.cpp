#define __CPPLEXER_IMP_CPP

//
//
//

#include "cpplexer.inc"


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
