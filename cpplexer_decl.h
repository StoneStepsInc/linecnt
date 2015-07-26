#ifndef __CPPLEXER_DECL_H
#define __CPPLEXER_DECL_H

#include <stdio.h>

#ifndef __CPPLEXER_IMP_CPP
#include "cpplexer_decl.h"
#endif
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

#define TOKEN_CPP_END			0
#define TOKEN_C_COMMENT_EOL		7
#define TOKEN_CPP_COMMENT_EOL		8
#define TOKEN_EMPTY_LINE		9
#define TOKEN_BRACE_LINE		10
#define TOKEN_CODE_C_COMMENT_EOL	11
#define TOKEN_CODE_CPP_COMMENT_EOL	12
#define TOKEN_CODE_C_CPP_COMMENT_EOL	13
#define TOKEN_CODE_EOL			14

#endif // __CPPLEXER_DECL_H

