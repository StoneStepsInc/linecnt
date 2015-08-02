#ifndef __CPPLEXER_DECL_H
#define __CPPLEXER_DECL_H

#include <stdio.h>

//
//
//
class CppFlexLexer {
	public:
		CppFlexLexer(FILE* arg_yyin = NULL, FILE* arg_yyout = NULL );

		~CppFlexLexer(void);
        
		const char *YYText(void);

		int yylex(void);
};

#ifndef __CPPLEXER_IMP_CPP
#undef yyFlexLexer
#endif // __CPPLEXER_IMP_CPP

//
//
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

