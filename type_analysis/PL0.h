
#include <stdio.h>
#include <vector>
#include <tuple>

#define MAX_CONTINUOUS_ASSIGN 10//MAX_CONTINUOUS_ASSIGN
#define NRW        16     // number of reserved words
#define TXMAX      500    // length of identifier table
#define MAXNUMLEN  14     // maximum number of digits in numbers
#define NSYM       11     // maximum number of symbols in array ssym and csym
#define MAXIDLEN   10     // length of identifiers

#define MAXADDRESS 32767  // maximum address
#define MAXLEVEL   32     // maximum depth of nesting block
#define CXMAX      500    // size of code array

#define MAXSYM     30     // maximum number of symbols

#define STACKSIZE  1000   // maximum storage

enum symtype
{
	SYM_NULL,
	SYM_IDENTIFIER,
	SYM_NUMBER,
	SYM_PLUS,
	SYM_MINUS,
	SYM_TIMES,
	SYM_SLASH,
	SYM_ODD,
	SYM_EQU,
	SYM_NEQ,
	SYM_LES,
	SYM_LEQ,
	SYM_GTR,
	SYM_GEQ,
	SYM_LPAREN,
	SYM_RPAREN,
	SYM_COMMA,
	SYM_SEMICOLON,
	SYM_PERIOD,
	SYM_BECOMES,
	SYM_BEGIN,
	SYM_END,
	SYM_IF,
	SYM_THEN,
	SYM_WHILE,
	SYM_DO,
	SYM_CALL,
	SYM_CONST,
	SYM_VAR,
	SYM_PROCEDURE,
	SYM_NOT,//2017-09-24
	SYM_AND,
	SYM_OR,
	SYM_ELSE,//2018-11-22
	SYM_BREAK,
	SYM_CONTINUE,
	SYM_INT,//
	SYM_POINTER,
	SYM_LP,
	SYM_RP,
	SYM_VOID,//2018-12-10
	SYM_EOF		//12/14
};

enum vartype{
	TYPE_VOID,
	TYPE_INT,
	TYPE_POINTER,
	TYPE_ARRAY,
	TYPE_FUNCTION,
	TYPE_VARIABLE,			//only for formal parameters
	TYPE_MAP
};



enum idtype
{
	ID_CONSTANT, ID_VARIABLE, ID_PROCEDURE
};

enum opcode
{
	LIT, OPR, LOD, STO, CAL, INT, JMP, JPC
};

enum oprcode
{
	OPR_RET, OPR_NEG, OPR_ADD, OPR_MIN,
	OPR_MUL, OPR_DIV, OPR_ODD, OPR_EQU,
	OPR_NEQ, OPR_LES, OPR_LEQ, OPR_GTR,
	OPR_GEQ, OPR_NOT, OPR_AND, OPR_OR //2017-09-24
};


typedef struct
{
	int f; // function code
	int l; // level
	int a; // displacement address
} instruction;

//////////////////////////////////////////////////////////////////////
char* err_msg[] =
{
/*  0 */    "",
/*  1 */    "Found ':=' when expecting '='.",
/*  2 */    "There must be a number to follow '='.",
/*  3 */    "There must be an '=' to follow the identifier.",
/*  4 */    "There must be an identifier to follow 'const', 'var', or 'procedure'.",
/*  5 */    "Missing ',' or ';'.",
/*  6 */    "Incorrect procedure name.",
/*  7 */    "Statement expected.",
/*  8 */    "Follow the statement is an incorrect symbol.",
/*  9 */    "'.' expected.",
/* 10 */    "';' expected.",
/* 11 */    "Undeclared identifier.",
/* 12 */    "Illegal assignment.",
/* 13 */    "':=' expected.",
/* 14 */    "There must be an identifier to follow the 'call'.",
/* 15 */    "A constant or variable can not be called.",
/* 16 */    "'then' expected.",
/* 17 */    "';' or 'end' expected.",
/* 18 */    "'do' expected.",
/* 19 */    "Incorrect symbol.",
/* 20 */    "Relative operators expected.",
/* 21 */    "Procedure identifier can not be in an expression.",
/* 22 */    "Missing ')'.",
/* 23 */    "The symbol can not be followed by a factor.",
/* 24 */    "The symbol can not be as the beginning of an expression.",
/* 25 */    "The number is too great.",
/* 26 */    "you can only assign an value to a left_value expression",
/* 27 */    "",
/* 28 */    "",
/* 29 */    "",
/* 30 */    "",
/* 31 */    "",
/* 32 */    "There are too many levels.",
/* 33 */	"Array or function cannot be the return value of a function.",
/* 34 */	"Functions cannot be the elements of an array.",
/* 35 */    "The parameter of a function cannot be a function."
};

//////////////////////////////////////////////////////////////////////
char ch;         // last character read
int  sym;        // last symbol read
char id[MAXIDLEN + 1]; // last identifier read
int  num;        // last number read
int  cc;         // character count
int  ll;         // line length
int  kk;
int  err;
int  cx;         // index of current instruction to be generated.
int  level = 0;
int  tx = 0;
int dx = 0;
int start_tx;
int exsit_break = 0;
int exsit_continue = 0;

int previous_sym;//2018-11-24
int now_sym;
int conflict=0;//2018-11-25
int break_cx = 0, continue_cx = 0;


char line[80];

instruction code[CXMAX];

char* word[NRW + 1] =
{
	"", /* place holder */
	"begin", "call", "const", "do", "end","if",
	"odd", "procedure", "then", "var", "while","else","break","continue","int", "void"//2018-12-10
};

int wsym[NRW + 1] =//int void
{
	SYM_NULL, SYM_BEGIN, SYM_CALL, SYM_CONST, SYM_DO, SYM_END,
	SYM_IF, SYM_ODD, SYM_PROCEDURE, SYM_THEN, SYM_VAR, SYM_WHILE,
	SYM_ELSE,SYM_BREAK,SYM_CONTINUE, SYM_INT, SYM_VOID//2018-12-10
};

int ssym[NSYM + 1] =//
{
	SYM_NULL, SYM_PLUS, SYM_MINUS, SYM_TIMES, SYM_SLASH,
	SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA, SYM_PERIOD, SYM_SEMICOLON,SYM_NOT
};

char csym[NSYM + 1] =//
{
	' ', '+', '-', '*', '/', '(', ')', '=', ',', '.', ';', '!' //2017-09-24
};

#define MAXINS   8
char* mnemonic[MAXINS] =
{
	"LIT", "OPR", "LOD", "STO", "CAL", "INT", "JMP", "JPC"
};

typedef struct
{
	char name[MAXIDLEN + 1];
	int  kind;
	int  value;
} comtab;

comtab table[TXMAX];

typedef struct
{
	char  name[MAXIDLEN + 1];
	int   kind;
	short level;
	short address;
} mask;

FILE* infile;

typedef tuple<int, int> typenode;
	//int type;
	// int size;	//or tx of variable in table; or index for a parameter in id_table



typedef vector<typenode> type_list;
//type_list[0]:
// 					type == 1: is real
//					type == 0: is formal




//11/26
typedef struct control_node{	//why not use cpp...
	int cx;
	struct control_node *next;
} control_node;

typedef struct control_list{
	control_node *h, *r;	//head and rear
} control_list;

void clear_control_list(control_list *l){
	control_node *h = l->h;
	control_node *p;
	while(h){
		p = (h)->next;
		free(h);
		h = p;
	}
	l->h = l->r = NULL;
}

void delete_control_list(control_list *l){
	control_node *h = l->h;
	control_node *p;
	while(h){
		p = (h)->next;
		free(h);
		h = p;
	}
	free(l);
}

void add_item_control_list(control_list *l, control_node *n){
	control_node *p1;
	p1 = l->r;
	if(p1){
		p1->next = n;
		l->r = n;
	}
	else{
		l->h = l->r = n;
	}
	n->next = NULL;


}



void init_control_list(control_list *l){
	l->h = l->r = NULL;
}

int is_empty_control_list(control_list *l){
	if(l->h)	return 1;
	else return 0;
}

// EOF PL0.h
