// type_analysis.cpp : 定义控制台应用程序的入口点。
//

//#include "stdafx.h"
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include<iostream>
using namespace std;
#include "pl0.h"
#include "set.c"

vector<tuple<int, int> > tk_table;	//token table. <0>:sym   <1>:size

vector<type_list* > id_table;		//including all real and formal


// void error(int n)
// {
// 	int i;

// 	printf("      ");
// 	for (i = 1; i <= cc - 1; i++)
// 		printf(" ");
// 	printf("^\n");
// 	printf("Error %3d: %s\n", n, err_msg[n]);
// 	err++;
// } // error

void error(int cc, int msgn){	//cc: character count   msgn:msg number
	int i;
	for (i = 0; i < cc; i++){
		cout << " ";
	}
	cout << "^" << endl;
	cout << "Type error " << msgn << ": " << err_msg[msgn] << endl;
	err++;

}


//仅在 getsym() 中被调用
void getch(void)	//返回下一个字符. 实际一次读入一行, 然后每次调用返回一个字符, 直到该行的字符全部返回完, 此时读取下一行
					//ll 是该行总字符数, cc 是已经返回到哪个字符的计数
{
	if (cc == ll)	//此 if 结构只在要读入新的一行时被执行
	{
		///识别且跳过行结束符
		if (feof(infile))
		{
			printf("\nPROGRAM INCOMPLETE\n");
			system("pause");
			exit(1);
		}
		ll = cc = 0;
		printf("%5d  ", cx++);	//产生一份程序列表，输出相应行号或指令计数器的值
		while ((!feof(infile)) //读取该行的下一个字符
			&& ((ch = getc(infile)) != '\n'))	//识别且跳过回车符
		{
			printf("%c", ch);	//将输入源文件复写到输出文件
			line[++ll] = ch;	//本次读取的字符存入 line[] 中
		} // while
		printf("\n");
		line[++ll] = ' ';		//该行结束,line[] 的最后存入空格, 同时输出换行
	}
	ch = line[++cc];	//每调用一次 getch(), 返回的字符存放在 ch 中
} // getch

  //////////////////////////////////////////////////////////////////////

  // gets a symbol from input stream.
void getsym(void)
{
	int i, k;
	char a[MAXIDLEN + 1];
	while (ch == ' ' || ch == '\t')	//跳过分隔符(如空格，回车，制表符)(回车符在 getch() 中已跳过)
		getch();

	//识别保留字或者标识符id（这里只识别INT VOID）2018-12-9
	if (isalpha(ch))
	{ // symbol is a reserved word or an identifier.
		k = 0;
		do    //完整读取该记号,当下一个字符不为字母或数字时结束读取
		{
			if (k < MAXIDLEN)
				a[k++] = ch;
			getch();
		} while (isalpha(ch) || isdigit(ch));

		a[k] = 0;
		strcpy(id, a);	//将记号从临时存放处 a[] 拷贝到 id[] 中(strcpy的拷贝以0作为结束符)
		word[0] = id;
		i = NRW;		// NRW 是保留字的数量
		while (strcmp(id, word[i--]));	//将这个记号与各个保留字进行比较判断其是不是保留字
		if (++i)
			sym = wsym[i]; 			//该记号是保留字
		else
			sym = SYM_IDENTIFIER;   //该记号是标识符
	}

	//判断正在读入的记号是否为数字
	//识别数字序列，当前值赋给全局量 NUM，sym 则置为 SYM_NUMBER
	else if (isdigit(ch))
	{ // symbol is a number.
		k = num = 0;
		sym = SYM_NUMBER;
		do
		{
			num = num * 10 + ch - '0';
			k++;
			getch();
		} while (isdigit(ch));
		if (k > MAXNUMLEN)
			error(cc, 25);     // The number is too great.
	}

	//以下各分支判断正在读入的记号是否为 * [ ] ( )
	else if (ch == '*')
	{
		getch();
		sym = SYM_POINTER;

	}
	else if (ch == '(')
	{
		getch();
		sym = SYM_LPAREN;

	}
	else if (ch == ')')
	{
		getch();
		sym = SYM_RPAREN;
	}
	else if (ch == '[')
	{
		getch();
		sym = SYM_LP;

	}
	else if (ch == ']')
	{
		getch();
		sym = SYM_RP;
	}
	else if (ch == ';') {
		getch();
		sym = SYM_SEMICOLON;
	}
	else if(ch == EOF){
		sym = SYM_EOF;
	}
	//未知的记号
	else//
	{ // other tokens

		i = NSYM;
		csym[0] = ch;
		while (csym[i--] != ch);
		if (++i)
		{
			sym = ssym[i];
			getch();
		}
		else
		{
			printf("Fatal Error: Unknown character.\n");
			system("pause");

			exit(1);
		}
	}
} // getsym



int type_check_aux(int cl, type_list *l, bool is_in_function = false) {
	int p_num = 0; //parental numbers. used when dealing with formal parameters.
	int size = l->size();
	for (int i = 1; i <size; i++) {
		int type = get<0>((*l)[i]);
		if (type == TYPE_MAP) {
			cl += 4;
			if(get<0>((*l)[i-1]) != TYPE_FUNCTION)
				cl -= 3;	//mitigate the deprecated "X"
			int s_nxt = get<0>((*l)[i + 1]);
			if (s_nxt == TYPE_ARRAY || \
				s_nxt == TYPE_FUNCTION) {
				error(cl, 33);
				return 0;
			}
		}
		else if (type == TYPE_INT) {
			cl += 3;
		}
		else if (type == TYPE_VOID) {
			cl += 4;
		}
		else if (type == TYPE_ARRAY) {
			char str[20];
			p_num++;
			int num = get<1>((*l)[i]);
			_itoa(num, str, 10);
			cl += 8 + strlen(str);
			int s_nxt = get<0>((*l)[i + 1]);
			if (s_nxt == TYPE_FUNCTION) {
				error(cl, 34);
				return 0;
			}
		}
		else if (type == TYPE_FUNCTION) {
			p_num++;
			if (i == 1 && is_in_function) {
				error(cl, 35);
				return 0;
			}
			cl += 9;
		}
		else if (type == TYPE_VARIABLE) {
			type_list *n_l = id_table[get<1>((*l)[i])];
			int r;
			r = type_check_aux(cl, n_l, true);
			if (r == 0) {
				return 0;
			}
			cl = r + 3;
		}
		else if (type == TYPE_POINTER) {
			cl += 8;
			p_num++;
		}
	}
	return cl + p_num;
}




int type_check(type_list *l){		//return value: cl
	int cl = 8;
	cl += strlen((table[get<1>((*l)[0])]).name);
	int r_value = type_check_aux(cl, l);
	if(r_value != 0){
		cout << "Type checking... OK." << endl;
	}
	return r_value;
	
}

int print_type_size(type_list *l) {
	int l_size = l->size();
	int size = 1;
	for (int i = 1; i < l_size; i++) {
		if (get<0>((*l)[i]) == TYPE_ARRAY) {
			size *= get<1>((*l)[i]);
		}
		else break;
	}
	return size;
}


void print_type_list(type_list * l, int s){

	if(s >= l->size()){
		return;
	}
	int type = get<0>((*l)[s]);
	if(type == TYPE_POINTER){
		cout << "pointer(";
		print_type_list(l, s+1);
		cout << ")";
	}
	else if(type == TYPE_ARRAY){
		cout << "array(" << get<1>((*l)[s]) << ", ";
		print_type_list(l, s+1);
		cout << ")";
	}
	else if(type == TYPE_INT){
		cout << "int";
		print_type_list(l, s+1);
	}
	else if(type == TYPE_VOID){
		cout << "void";
		print_type_list(l, s+1);
	}
	else if(type == TYPE_VARIABLE){
		int index = get<1>((*l)[s]);
		type_list *nlist = id_table[index];
		print_type_list(nlist, 1);
		cout << " X ";
		print_type_list(l, s+1);
	}
	else if(type == TYPE_FUNCTION){
		cout << "function(";
		print_type_list(l, s+1);
		cout << ")";
	}
	else if(type == TYPE_MAP){
		if(get<0>((*l)[s-1]) != TYPE_FUNCTION)
			cout << "\b\b=> ";
		else
			cout << " => ";
		print_type_list(l, s+1);
	}
	else{
		cout << "Error. print_type_list(). line 207" << endl;
		exit(1);
	}
}


void enter(int kind)
{
	mask* mk;

	tx++;
	strcpy(table[tx].name, id);
	table[tx].kind = kind;
	switch (kind)
	{
	case ID_CONSTANT:
		if (num > MAXADDRESS)
		{
			error(cc, 25); // The number is too great.
			num = 0;
		}
		table[tx].value = num;
		break;
	case ID_VARIABLE:
		mk = (mask*)&table[tx];
		mk->level = level;
		mk->address = dx++;
		break;
	case ID_PROCEDURE:
		mk = (mask*)&table[tx];
		mk->level = level;
		break;
	} // switch
} // enter


//clear parameter list with () in tk_table
void clear_parameter_list(){
	int start, end;
	end = tk_table.size() - 1;
	if(get<0>(tk_table[end]) != SYM_RPAREN){
		cout << "Error in clear_parameter_list()" << endl;
		exit(1);
	}
	int cnt = 1;
	int i;
	for( i = end-1; i >= 0; i--){
		int symbol = get<0>(tk_table[i]);
		if( symbol == SYM_RPAREN){
			cnt++;
		}
		else if(symbol == SYM_LPAREN){
			cnt--;
		}
		if(cnt == 0){
			break;
		}
	}
	start = i;
	tk_table.erase(tk_table.begin() + start, tk_table.begin() + end+1);

}

bool render_type(type_list *l){
	int i;
	int symbol;
	for(i = tk_table.size() - 1; i >= 0; i--){
		if(get<0>(tk_table[i]) == SYM_IDENTIFIER) break;
	}
	if (i == -1) return false;
	if(i < tk_table.size() - 1){
		symbol = get<0>(tk_table[i+1]);
		if ( symbol == SYM_LP){
			int size = get<1>(tk_table[i+2]);
			auto t = make_tuple(TYPE_ARRAY, size);
			l->push_back(t);
			tk_table.erase(tk_table.begin() + i+1, tk_table.begin() + i+4);			//erase: [start, end)
		}
		else if(symbol == SYM_RPAREN){
            int k;
			for(k = i - 1; k >= 0; k--){
				if(get<0>(tk_table[k]) == SYM_LPAREN){
					break;
				}
				auto t = make_tuple((TYPE_POINTER), 1);
				l->push_back(t);
			}
			tk_table.erase(tk_table.begin() + i+1);
			tk_table.erase(tk_table.begin() + k, tk_table.begin() + i);

		}
	}
	else{
            int k;
			for( k = i - 1; k >= 0; k--){
				symbol = get<0>(tk_table[k]);
				if( symbol == SYM_LPAREN || symbol == SYM_COMMA){
					break;
				}
				auto t = make_tuple((TYPE_POINTER), 1);
				l->push_back(t);
			}
			k++;
			tk_table.erase(tk_table.begin() + k, tk_table.begin() + i);

	}
	return true;


}


void declarator(type_list *);
void declaration_specifiers(int &type);
int num_of_comma = 0;
void parameter_declaration(type_list *t_list) {
	int type;
	declaration_specifiers(type);
	declarator(t_list);
	render_type(t_list);		//!! added for situation like: **p
	if(type == SYM_INT){
		type = TYPE_INT;
	}
	else type = TYPE_VOID;
	t_list->push_back(make_tuple(type, 1));


}


void C(type_list *t_list) {
	if (sym == SYM_COMMA) {
		tk_table.push_back(make_tuple(sym, 0));
		getsym();

		type_list *pt_list = new type_list;
		pt_list->push_back(make_tuple(0,0));
		parameter_declaration(pt_list);
		//
		id_table.push_back(pt_list);
		t_list->push_back(make_tuple(TYPE_VARIABLE, id_table.size()-1));

		C(t_list);
	}
	else {//空产生式
		//getsym();
		return;
	}
}



// void init_declarator(int type) {
// 	tk_table.clear();
// 	type_list *t_list = new type_list;
// 	t_list->push_back(make_tuple(0, 0));

// 	declarator(t_list);

// 	render_type(t_list);
// 	if(type == SYM_VOID){
// 		type = TYPE_VOID;
// 	}
// 	else type = TYPE_INT;
// 	t_list->push_back(type, 1);
// 	id_table.push_back(t_list);
// }



void parameter_list(type_list *t_list) {
	type_list *pt_list = new type_list;
	pt_list->push_back(make_tuple(0,0));
	parameter_declaration(pt_list);

	id_table.push_back(pt_list);
	t_list->push_back(make_tuple(TYPE_VARIABLE, id_table.size()-1));

	C(t_list);
}


void parameter_type_list(type_list *t_list) {
	parameter_list(t_list);
}


void B(type_list *t_list) {
	if (sym == SYM_LP) {//'['
		tk_table.push_back(make_tuple(sym, 0));
		getsym();
		if (sym == SYM_NUMBER) {
			enter(ID_CONSTANT);
			tk_table.push_back(make_tuple(sym, num));
			getsym();
			if (sym == SYM_RP) {//']'         [number]
				tk_table.push_back(make_tuple(sym, 0));
				render_type(t_list);
				getsym();
				B(t_list);
				return;
			}
			else {
				cout << "B() error" << endl;		//error msg can be strengthened
			}
		}
		else {
			cout << "B() error" << endl;
		}
	}
	else if (sym == SYM_LPAREN) {//'('
		tk_table.push_back(make_tuple(sym, 0));
		t_list->push_back(make_tuple(TYPE_FUNCTION, 1));
		getsym();
		if (sym == SYM_VOID || sym == SYM_INT) {// direct_declarator '(' parameter_type_list ')'


			parameter_type_list(t_list);
			// if (sym == SYM_RPAREN) {//'(' parameter_type_list ')
			// 	int i;
			// 	for(i = tk_table.size(); ; i--){
			// 		if(get<0>(tk_table[i]) == SYM_LPAREN) break;

			// 	}
			if(sym != SYM_RPAREN){
				cout << "Error. After parameter_type_list() in B(). line 383" << endl;
			}
			tk_table.push_back(make_tuple(sym, 0));
			clear_parameter_list();
			t_list->push_back(make_tuple(TYPE_MAP, 1));
			getsym();
			B(t_list);

		}
		else if (sym == SYM_RPAREN) {//')'   direct_declarator '(' ')'
			tk_table.push_back(make_tuple(sym, 0));
			clear_parameter_list();
			t_list->push_back(make_tuple(TYPE_MAP, 1));
			getsym();
			B(t_list);
		}
		else {
			cout << "B() error" << endl;
		}
	}
	else {
		//getsym();
		return;//do nothing
	}
}


void direct_declarator(type_list *t_list) {
	if (sym == SYM_IDENTIFIER) {//id
		enter(ID_VARIABLE);		//enter the symbol into the symbol table
		int type = get<0>((*t_list)[0]);


		(*t_list)[0] = make_tuple(type, tx);
		tk_table.push_back(make_tuple(sym, tx));
		getsym();
		B(t_list);
	}
	else if (sym == SYM_LPAREN) {//'('
		tk_table.push_back(make_tuple(sym, 0));
		getsym();
		declarator(t_list);
		if (sym == SYM_RPAREN) {
			tk_table.push_back(make_tuple(sym, 0));
			render_type(t_list);
			getsym();
			B(t_list);
		}
		else {
			cout << "direct_declarator() error" << endl;
		}
	}
	else {
		cout << "direct_declarator() error" << endl;
	}
}


void pointer(type_list *t_list) {
	if (sym == SYM_POINTER) {
		tk_table.push_back(make_tuple(sym, 0));
		getsym();
	}
	if(sym == SYM_POINTER) {
		pointer(t_list);
	}
}


void declarator(type_list *t_list) {
	if (sym == SYM_POINTER) {
		/*int cc_a = cc + 1;
		while (cc_a <= ll) {
			if (line[cc_a] == '*') {
				cc_a++;
				num_of_pointer++;
			}
			else break;
		}*/
		pointer(t_list);
		direct_declarator(t_list);
	}
	else if(sym == SYM_IDENTIFIER || sym == SYM_LPAREN){
		direct_declarator(t_list);			//12/14
	}
	else{
		cout << "declarator() error" << endl;
		exit(1);		// !! can do some error recovery here
	}
}


void typespecifier(int &type) {
	if (sym == SYM_VOID) {
		type = sym;
		getsym();
	}
	else if (sym == SYM_INT) {
		type = sym;
		getsym();
	}
	else {
		cout << "typespecifier() error" << endl;
	}
}


void init_declarator(int type) {
	tk_table.clear();
	type_list *t_list = new type_list;
	t_list->push_back(make_tuple(1, 0));

	declarator(t_list);

	render_type(t_list);
	if(type == SYM_VOID){
		type = TYPE_VOID;
	}
	else type = TYPE_INT;
	t_list->push_back(make_tuple(type, 1));
	id_table.push_back(t_list);
}
/*
void init_declarator_list() {
	while (num_of_comma--) {
		init_declarator_list();
	}
	init_declarator();
	getsym();
}*/
void A(int type) {
	if (sym == SYM_COMMA) {
		getsym();
		init_declarator(type);
		A(type);
		// getsym();		12/14 no need
	}
	else {
		return;//do nothing
	}
}


void init_declarator_list(int type) {

	init_declarator(type);
	A(type);
	//getsym();    //A最终推出空产生式，不需要再getsym()
}


void declaration_specifiers(int &type) {
	typespecifier(type);
//	getsym();
}


void declaration() {
	int type;
	declaration_specifiers(type);
	/*
	//getsym();
	//下面计算出逗号的个数，以便在init_declarator_list()中避免左递归
	num_of_comma = 0;
	int cc_a = cc + 1;
	while (cc_a <= ll) {
		if (line[cc_a] == ',') num_of_comma++;
		cc_a++;
	}
	*/
	init_declarator_list(type);
	if (sym == SYM_SEMICOLON) {// sym = ';'
		getsym();
		return;
	}
	else {
		cout << "declaration() error" << endl;
	}
	getsym();
}


void translation_unit() {
	while (sym == SYM_INT || sym == SYM_VOID) {// translation_unit -> declaration
		start_tx = tx;
		tk_table.clear();
		declaration();
		cout << endl;
	}
	for(int i = 0; i < id_table.size(); i++){
		if(get<0>((*id_table[i])[0]) == 1){		// is real variable
		    int ti = get<1>((*id_table[i])[0]);
            char *name = table[ti].name;
            cout << "type(" << name << "): ";
			print_type_list(id_table[i], 1);
			cout << endl;
			if (type_check(id_table[i])) {
				cout << "size(" << name << "): " << print_type_size(id_table[i]) <<endl;
			}
			cout << endl << endl;
		}
	}
	cout << "END" << endl;
}


int main()
{
	FILE* hbin;
	char s[80];
	int i;

	printf("Please input source file name: "); // get file name to be compiled
	scanf("%s", s);
	if ((infile = fopen(s, "r")) == NULL)
	{
		printf("File %s can't be opened.\n", s);
		system("puase");
		exit(1);
	}
	err = cc = cx = ll = 0; // initialize global variables
	ch = ' ';

	getsym();
	translation_unit();
	system("pause");
    return 0;
}

