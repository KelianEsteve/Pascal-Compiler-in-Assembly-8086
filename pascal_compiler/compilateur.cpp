#include <string>
#include <iostream>
#include <cstdlib>
#include <set>
#include <map>
#include <FlexLexer.h>
#include <cstring>
#include <vector>



using namespace std;

enum OPREL {EQU, DIFF, INF, SUP, INFE, SUPE, WTFR};
enum OPADD {ADD, SUB, OR, WTFA};
enum OPMUL {MUL, DIV, MOD, AND ,WTFM};
enum TYPES {INTEGER, BOOLEAN, INTEGER_ARRAY};
enum TOKEN {FEOF, UNKNOWN, KEYWORD, NUMBER, ID, STRINGCONST, RBRACKET, LBRACKET, RPARENT, LPARENT, COMMA, 
SEMICOLON, COLON, DOT, ADDOP, MULOP, RELOP, NOT, ASSIGN};


TOKEN current;


FlexLexer* lexer = new yyFlexLexer; // This is the flex tokeniser
// tokens can be read using lexer->yylex()
// lexer->yylex() returns the type of the lexicon entry (see enum TOKEN in tokeniser.h)
// and lexer->YYText() returns the lexicon entry as a string

	
map<string, enum TYPES> DeclaredVariables;	// Store declared variables and their types
unsigned long long TagNumber=0;

bool IsDeclared(const char *id){
	return DeclaredVariables.find(id)!=DeclaredVariables.end();
}


void Erreur(string s){
	cerr << "Ligne n°"<<lexer->lineno()<<", lu : '"<<lexer->YYText()<<"'("<<current<<"), mais ";
	cerr<< s << endl;
	exit(-1);
}
// To avoid cross references problems :
enum TYPES Expression(void);			// Called by Term() and calls Term()
void Statement(void);
void StatementPart(void);
void compileTableau(void);
		
enum TYPES Identifier(void){
	enum TYPES type;
	if(!IsDeclared(lexer->YYText()) and strcmp(lexer->YYText(),"ARRAY")!=0){
		cerr << "Erreur : Variable '"<<lexer->YYText()<<"' non déclarée"<<endl;
		exit(-1);
	}
	type=DeclaredVariables[lexer->YYText()];
	cout << "\tpush "<<lexer->YYText()<<endl;
	current=(TOKEN) lexer->yylex();
	return type;
}

enum TYPES Number(void){
	cout <<"\tpush $"<<atoi(lexer->YYText())<<endl;
	current=(TOKEN) lexer->yylex();
	return INTEGER;
}

enum TYPES Factor(void){
	enum TYPES type;
	if(current==RPARENT){
		current=(TOKEN) lexer->yylex();
		type=Expression();
		if(current!=LPARENT){
			Erreur("')' était attendu");		// ")" expected
		}
		else{
			current=(TOKEN) lexer->yylex();
		}
	}
	else {
		if (current==NUMBER){
			type=Number();
		}
	    else{
			if(current==ID){
				type=Identifier();
			}
			/*else{
				Erreur("'(' ou chiffre ou lettre attendue");
			}*/
		}
	
	}
	return type;
}

// MultiplicativeOperator := "*" | "/" | "%" | "&&"
OPMUL MultiplicativeOperator(void){
	OPMUL opmul;
	if(strcmp(lexer->YYText(),"*")==0)
		opmul=MUL;
	else if(strcmp(lexer->YYText(),"/")==0)
		opmul=DIV;
	else if(strcmp(lexer->YYText(),"%")==0)
		opmul=MOD;
	else if(strcmp(lexer->YYText(),"&&")==0)
		opmul=AND;
	else opmul=WTFM;
	current=(TOKEN) lexer->yylex();
	return opmul;
}

// Term := Factor {MultiplicativeOperator Factor}
enum TYPES Term(void){
	TYPES type1, type2;
	OPMUL mulop;
	type1=Factor();
	while(current==MULOP){
		mulop=MultiplicativeOperator();		// Save operator in local variable
		type2=Factor();
		if(type2!=type1)
			Erreur("types incompatibles dans l'expression");
		cout << "\tpop %rbx"<<endl;	// get first operand
		cout << "\tpop %rax"<<endl;	// get second operand
		switch(mulop){
			case AND:
				if(type2!=BOOLEAN)
					Erreur("le type doit être BOOLEAN dans l'expression");
				cout << "\tmulq	%rbx"<<endl;	// a * b -> %rdx:%rax
				cout << "\tpush %rax\t# AND"<<endl;	// store result
				break;
			case MUL:
				if(type2!=INTEGER)
					Erreur("le type doit être INTEGER dans l'expression");
				cout << "\tmulq	%rbx"<<endl;	// a * b -> %rdx:%rax
				cout << "\tpush %rax\t# MUL"<<endl;	// store result
				break;
			case DIV:
				if(type2!=INTEGER)
					Erreur("le type doit être INTEGER dans l'expression");
				cout << "\tmovq $0, %rdx"<<endl; 	// Higher part of numerator  
				cout << "\tdiv %rbx"<<endl;			// quotient goes to %rax
				cout << "\tpush %rax\t# DIV"<<endl;		// store result
				break;
			case MOD:
				if(type2!=INTEGER)
					Erreur("le type doit être INTEGER dans l'expression");
				cout << "\tmovq $0, %rdx"<<endl; 	// Higher part of numerator  
				cout << "\tdiv %rbx"<<endl;			// remainder goes to %rdx
				cout << "\tpush %rdx\t# MOD"<<endl;		// store result
				break;
			default:
				Erreur("opérateur multiplicatif attendu");
		}
	}
	return type1;
}

// AdditiveOperator := "+" | "-" | "||"
OPADD AdditiveOperator(void){
	OPADD opadd;
	if(strcmp(lexer->YYText(),"+")==0)
		opadd=ADD;
	else if(strcmp(lexer->YYText(),"-")==0)
		opadd=SUB;
	else if(strcmp(lexer->YYText(),"||")==0)
		opadd=OR;
	else opadd=WTFA;
	current=(TOKEN) lexer->yylex();
	return opadd;
}

// SimpleExpression := Term {AdditiveOperator Term}
enum TYPES SimpleExpression(void){
	enum TYPES type1, type2;
	OPADD adop;
	type1=Term();
	while(current==ADDOP){
		adop=AdditiveOperator();		// Save operator in local variable
		type2=Term();
		if(type2!=type1)
			Erreur("types incompatibles dans l'expression");
		cout << "\tpop %rbx"<<endl;	// get first operand
		cout << "\tpop %rax"<<endl;	// get second operand
		switch(adop){
			case OR:
				if(type2!=BOOLEAN)
					Erreur("le type doit être BOOLEAN dans l'expression");
				cout << "\taddq	%rbx, %rax\t# OR"<<endl;// operand1 OR operand2
				break;			
			case ADD:
				if(type2!=INTEGER)
					Erreur("le type doit être INTEGER dans l'expression");
				cout << "\taddq	%rbx, %rax\t# ADD"<<endl;	// add both operands
				break;			
			case SUB:	
				if(type2!=INTEGER)
					Erreur("le type doit être INTEGER dans l'expression");
				cout << "\tsubq	%rbx, %rax\t# SUB"<<endl;	// substract both operands
				break;
			default:
				Erreur("opérateur additif inconnu");
		}
		cout << "\tpush %rax"<<endl;			// store result
	}
	return type1;
}



enum TYPES Type(void){
	enum TYPES type;
	type=Term();
	if(current!=KEYWORD)
		Erreur("type attendu");
	if(strcmp(lexer->YYText(),"BOOLEAN")==0){
		current=(TOKEN) lexer->yylex();
		return BOOLEAN;
	}	
	else if(strcmp(lexer->YYText(),"INTEGER")==0){
		current=(TOKEN) lexer->yylex();
		if(strcmp(lexer->YYText(), "[")==0){ // Vérifier si c'est un tableau
            current = (TOKEN)lexer->yylex();
            if(current != NUMBER) Erreur("un nombre entier était attendu");
				current = (TOKEN)lexer->yylex();
            if(current != DOT) Erreur(".. attendu");
				current = (TOKEN)lexer->yylex();
			if(current != DOT) Erreur(".. attendu");
				current = (TOKEN)lexer->yylex();
            if(current != NUMBER) Erreur("un nombre entier était attendu");
				current = (TOKEN)lexer->yylex();
            if(current != RBRACKET) Erreur("']' attendu");
				return INTEGER_ARRAY; // Retourne le type INTEGER_ARRAY si c'est un tableau
        }
		return INTEGER;
	}
	else
		Erreur("type inconnu");	
		
	return INTEGER;
}

void compileTableau() {
    string ident = lexer->YYText();
    current = (TOKEN)lexer->yylex();
    if (current != LBRACKET) {
        Erreur("'[' attendu");
    }
    current = (TOKEN)lexer->yylex();
    int i = 0;
    while (current == NUMBER) {
        cout << "\tmovq $" << lexer->YYText() << ", " << ident << "+" << i << "*8(%rip)" << endl;
        i++;
        current = (TOKEN)lexer->yylex();
        if (current == COMMA) {
            current = (TOKEN)lexer->yylex();
        }
    }
    if (current != RBRACKET) {
        Erreur("']' attendu");
    }
}

// Declaration := Ident {"," Ident} ":" Type
void VarDeclaration(void){
	set<string> idents;
	enum TYPES type;
	if(current!=ID)
		Erreur("Un identificateur était attendu");
	idents.insert(lexer->YYText());
	current=(TOKEN) lexer->yylex();
	while(current==COMMA){
		current=(TOKEN) lexer->yylex();
		if(current!=ID)
			Erreur("Un identificateur était attendu");
		idents.insert(lexer->YYText());
		current=(TOKEN) lexer->yylex();
	}
	if(current!=COLON)
		Erreur("caractère ':' attendu");
	current=(TOKEN) lexer->yylex();
	type=Type();
	for (set<string>::iterator it=idents.begin(); it!=idents.end(); ++it){
	    cout << *it << ":\t.quad 0"<<endl;
            DeclaredVariables[*it]=type;
            if(strcmp(lexer->YYText(), "ARRAY")==0){
				compileTableau();
			}
            
	}
}

// VarDeclarationPart := "VAR" VarDeclaration {";" VarDeclaration} "."
void VarDeclarationPart(void){
	current=(TOKEN) lexer->yylex();
	VarDeclaration();
	while(current==SEMICOLON){
		current=(TOKEN) lexer->yylex();
		VarDeclaration();
	}
	if(current!=DOT)
		Erreur("'.' attendu");
	current=(TOKEN) lexer->yylex();
}

// RelationalOperator := "==" | "!=" | "<" | ">" | "<=" | ">="  
OPREL RelationalOperator(void){
	OPREL oprel;
	if(strcmp(lexer->YYText(),"==")==0)
		oprel=EQU;
	else if(strcmp(lexer->YYText(),"!=")==0)
		oprel=DIFF;
	else if(strcmp(lexer->YYText(),"<")==0)
		oprel=INF;
	else if(strcmp(lexer->YYText(),">")==0)
		oprel=SUP;
	else if(strcmp(lexer->YYText(),"<=")==0)
		oprel=INFE;
	else if(strcmp(lexer->YYText(),">=")==0)
		oprel=SUPE;
	else oprel=WTFR;
	current=(TOKEN) lexer->yylex();
	return oprel;
}

// Expression := SimpleExpression [RelationalOperator SimpleExpression]
enum TYPES Expression(void){
	enum TYPES type1, type2;
	unsigned long long tag;
	OPREL oprel;
	type1=SimpleExpression();
	if(current==RELOP){
		tag=++TagNumber;
		oprel=RelationalOperator();
		type2=SimpleExpression();
		if(type2!=type1)
			Erreur("types incompatibles pour la comparaison");
		cout << "\tpop %rax"<<endl;
		cout << "\tpop %rbx"<<endl;
		cout << "\tcmpq %rax, %rbx"<<endl;
		switch(oprel){
			case EQU:
				cout << "\tje Vrai"<<tag<<"\t# If equal"<<endl;
				break;
			case DIFF:
				cout << "\tjne Vrai"<<tag<<"\t# If different"<<endl;
				break;
			case SUPE:
				cout << "\tjae Vrai"<<tag<<"\t# If above or equal"<<endl;
				break;
			case INFE:
				cout << "\tjbe Vrai"<<tag<<"\t# If below or equal"<<endl;
				break;
			case INF:
				cout << "\tjb Vrai"<<tag<<"\t# If below"<<endl;
				break;
			case SUP:
				cout << "\tja Vrai"<<tag<<"\t# If above"<<endl;
				break;
			default:
				Erreur("Opérateur de comparaison inconnu");
		}
		cout << "\tpush $0\t\t# False"<<endl;
		cout << "\tjmp Suite"<<tag<<endl;
		cout << "Vrai"<<tag<<":\tpush $0xFFFFFFFFFFFFFFFF\t\t# True"<<endl;	
		cout << "Suite"<<tag<<":"<<endl;
		return BOOLEAN;
	}
	return type1;
}

// AssignementStatement := Identifier ":=" Expression
void AssignementStatement(void){
	enum TYPES type1, type2;
	string variable;
	if(current!=ID)
		Erreur("Identificateur attendu");
	if(!IsDeclared(lexer->YYText())){
		cerr << "Erreur : Variable '"<<lexer->YYText()<<"' non déclarée"<<endl;
		exit(-1);
	}
	variable=lexer->YYText();
	type1=DeclaredVariables[variable];
	current=(TOKEN) lexer->yylex();
	if(current!=ASSIGN)
		Erreur("caractères ':=' attendus");
	current=(TOKEN) lexer->yylex();
	type2=Expression();
	if(type2!=type1){
		cerr<<"Type variable "<<type1<<endl;
		cerr<<"Type Expression "<<type2<<endl;
		Erreur("types incompatibles dans l'affectation");
	}
	cout << "\tpop "<<variable<<endl;
}

// DisplayStatement := "DISPLAY" Expression


void DisplayStatement(void){
	enum TYPES type;
	unsigned long long tag=++TagNumber;
	current=(TOKEN) lexer->yylex();
	type=Expression();
	if(type==INTEGER){
		cout << "\tpop %rdx\t# The value to be displayed"<<endl;
		cout << "\tmovq $FormatString1, %rsi\t# \"%llu\\n\""<<endl;
	}
	else
		if(type==BOOLEAN){
			cout << "\tpop %rdx\t# Zero : False, non-zero : true"<<endl;
			cout << "\tcmpq $0, %rdx"<<endl;
			cout << "\tje False"<<tag<<endl;
			cout << "\tmovq $TrueString, %rsi\t# \"TRUE\\n\""<<endl;
			cout << "\tjmp Next"<<tag<<endl;
			cout << "False"<<tag<<":"<<endl;
			cout << "\tmovq $FalseString, %rsi\t# \"FALSE\\n\""<<endl;
			cout << "Next"<<tag<<":"<<endl;
		}
		else
			Erreur("DISPLAY ne fonctionne que pour les nombres entiers");
	cout << "\tmovl	$1, %edi"<<endl;
	cout << "\tmovl	$0, %eax"<<endl;
	cout << "\tcall	__printf_chk@PLT"<<endl;

}

// ForStatement := "For" ID ":=" Expression ("TO"|"DOWNTO") Expression "DO" Statement
void ForStatement() {
	enum TYPES type;
    const unsigned long long tag = TagNumber++;
    current = (TOKEN) lexer->yylex();
    if(current != ID){
		Erreur("Un identifiant était attendu");
	}
	string variable = lexer->YYText();
	
	current = (TOKEN) lexer->yylex();
	if(strcmp(lexer->YYText(),":=")!=0){
		Erreur("':=' était attendu");
	}
	current = (TOKEN) lexer->yylex();
    string a = "$";
    a+=lexer->YYText();

    if(Expression() != INTEGER){
    	Erreur("Un chiffre est attendu");
    }
    cout<<"\tmovl "<<a<<", "<<variable<<endl;

    if(current != KEYWORD or strcmp(lexer->YYText(),"TO")!=0 and strcmp(lexer->YYText(),"DOWNTO")!=0){
    	Erreur("TO/DOWNTO attendu");
    }
    string mot = lexer->YYText(); //on récupère le mot clé dans un const char*
    current = (TOKEN) lexer->yylex();
    string b ="$";
    b+=lexer->YYText();

    cout << "PourInit"<<tag<<":"<<endl;
    cout << "\tmovq " << a << ", %rax\t# Stocker la valeur dans la variable" << endl;
    
    cout << "PourTest"<<tag<<":"<<endl;
    cout << "\tcmpq "<<b<<", %rax\t# Comparer la valeur de la variable à la borne supérieure" << endl;
    if(mot=="TO"){	
		cout << "\tja FinPour"<<tag<<"\t# Si la variable est supérieure, sortir de la boucle" << endl;
	}
	else if(mot=="DOWNTO"){
		cout << "\tjb FinPour"<<tag<<"\t# Si la variable est inférieur, sortir de la boucle" << endl;
	}
    cout << "\tpushq %rax"<<endl;
    
    
    current = static_cast<TOKEN>(lexer->yylex());
    current = static_cast<TOKEN>(lexer->yylex());
    Statement();
    cout << "\tpopq %rax"<<endl;
    if(mot=="TO"){	
		cout << "\taddq $1, "<< "%rax\t# Incrémenter la variable " <<endl;
	}
	else if(mot=="DOWNTO"){
		cout << "\tsubq $1, "<< "%rax\t# Décrémenter la variable " <<endl;
	}
    cout << "\tmovq %rax, "<<variable<<endl;
    cout << "\tjmp PourTest"<<tag<<"\t# Revenir au début de la boucle" << endl;
    cout << "FinPour"<<tag<<":" <<endl;
}



// WhileStatement := "WHILE" Expression "DO" Statement
void WhileStatement() {
    const unsigned long long tag = TagNumber++;
    cout << "While" << tag << ":" << std::endl;
    current = static_cast<TOKEN>(lexer->yylex());
    if (Expression() != BOOLEAN) {
        Erreur("expression booléene attendue");
    }
    cout << "\tpop %rax\t# Get the result of expression" <<endl;
    cout << "\tcmpq $0, %rax" << endl;
    cout << "\tje EndWhile" << tag << "\t# if FALSE, jump out of the loop" << tag <<endl;
    if (current != KEYWORD || strcmp(lexer->YYText(), "DO") != 0) {
        Erreur("mot-clé DO attendu");
    }
    current = static_cast<TOKEN>(lexer->yylex());
    Statement();
    cout << "\tjmp While" << tag << std::endl;
    cout << "EndWhile" << tag << ":" << std::endl;
}

// BlockStatement := "BEGIN" Statement {";" Statement} "END"
void BlockStatement(void){
	current=(TOKEN) lexer->yylex();
	Statement();
	while(current==SEMICOLON){
		current=(TOKEN) lexer->yylex();	// Skip the ";"
		Statement();
	};
	if(current!=KEYWORD||strcmp(lexer->YYText(), "END")!=0)
		Erreur("mot-clé END attendu");
	current=(TOKEN) lexer->yylex();
}

// IfStatement := "IF" Expression "THEN" Statement ["ELSE" Statement]
void IfStatement(void){
	unsigned long long tag=TagNumber++;
	current=(TOKEN) lexer->yylex();
	if(Expression()!=BOOLEAN)
		Erreur("le type de l'expression doit être BOOLEAN");
	cout<<"\tpop %rax\t# Get the result of expression"<<endl;
	cout<<"\tcmpq $0, %rax"<<endl;
	cout<<"\tje Else"<<tag<<"\t# if FALSE, jump to Else"<<tag<<endl;
	if(current!=KEYWORD||strcmp(lexer->YYText(),"THEN")!=0)
		Erreur("mot-clé 'THEN' attendu");
	current=(TOKEN) lexer->yylex();
	Statement();
	cout<<"\tjmp Next"<<tag<<"\t# Do not execute the else statement"<<endl;
	cout<<"Else"<<tag<<":"<<endl; // Might be the same effective adress than Next:
	if(current==KEYWORD&&strcmp(lexer->YYText(),"ELSE")==0){
		current=(TOKEN) lexer->yylex();
		Statement();
	}
	cout<<"Next"<<tag<<":"<<endl;
}

void CaseStatement() {
    const unsigned long long tag = TagNumber++;
    string label;
    current = (TOKEN) lexer->yylex();
    if (current != ID && current != NUMBER) {
        Erreur("identificateur ou nombre attendu");
    }
    string caseValue = lexer->YYText();
    if (current == ID) {
        cout << "\tmovb " << caseValue << ", %al" << endl;
    } else {
        cout << "\tmovb $" << caseValue << ", %al" << endl;
    }
    current = (TOKEN) lexer->yylex();
    //cout<<lexer->YYText()<<endl;
    cout << "Cas" << tag << ":" << endl;
    if (strcmp(lexer->YYText(),"OF")!=0) {
        Erreur("mot-clé OF attendu");
    }
    current = (TOKEN) lexer->yylex();
    //cout<<lexer->YYText()<<endl;
    while (current == NUMBER) {
        cout << "\tcmpb $" << lexer->YYText() << ", %al" << endl;
        cout << "\tjne Cas" << tag << "_" << lexer->YYText() << endl;
        label = lexer->YYText();
        current = (TOKEN) lexer->yylex();
        if (strcmp(lexer->YYText(),":")!=0) {
            Erreur("deux-points attendu");
        }
        current = (TOKEN) lexer->yylex();
        Statement();
        cout << "\tjmp FinCas" << tag << endl;
        cout << "Cas" << tag << "_" << label << ":" << endl;
        current = (TOKEN) lexer->yylex();
    }
    cout << "\tjmp Cas" << tag << "Defaut" << endl;
    cout << "Cas" << tag << "Defaut:" << endl;
    if (current == KEYWORD && strcmp(lexer->YYText(), "ELSE") == 0) {
        current = (TOKEN) lexer->yylex();
        Statement();
    }
    cout << "FinCas" << tag << ":" << endl;
}



// Statement := AssignementStatement|DisplayStatement
void Statement(void){
    if(current==KEYWORD or strcmp(lexer->YYText(),"CASE")==0){
        if(strcmp(lexer->YYText(),"DISPLAY")==0)
            DisplayStatement();
        else if(strcmp(lexer->YYText(),"IF")==0)
            IfStatement();
        else if(strcmp(lexer->YYText(),"FOR")==0)
            ForStatement();
        else if(strcmp(lexer->YYText(),"WHILE")==0)
            WhileStatement();
        else if(strcmp(lexer->YYText(),"BEGIN")==0)
            BlockStatement();
        else if(strcmp(lexer->YYText(),"CASE")==0)
            CaseStatement();
        else
            Erreur("mot clé inconnu");
    }
    else if(current==ID)
        AssignementStatement();
    else
        Erreur("instruction attendue");   
}

// StatementPart := Statement {";" Statement} "."
void StatementPart(void){
	cout << "\t.align 8"<<endl;	// Alignement on addresses that are a multiple of 8 (64 bits = 8 bytes)
	cout << "\t.text\t\t"<<endl;
	cout << "\t.globl main\t"<<endl;
	cout << "main:\t\t\t"<<endl;
	cout << "\tmovq %rsp, %rbp\t"<<endl;
	Statement();
	while(current==SEMICOLON){
		current=(TOKEN) lexer->yylex();
		Statement();
	}
	if(current!=DOT)
		Erreur("caractère '.' attendu");
	current=(TOKEN) lexer->yylex();
}

// Program := [VarDeclarationPart] StatementPart
void Program(void){
	if(current==KEYWORD && strcmp(lexer->YYText(),"VAR")==0)
		VarDeclarationPart();
	StatementPart();	
}

int main(void){	// First version : Source code on standard input and assembly code on standard output
	// Header for gcc assembler / linker
	cout << ".data"<<endl;
	cout << "FormatString1:\t.string \"%llu\\n\"\t"<<endl; 
	cout << "TrueString:\t.string \"TRUE\\n\"\t"<<endl; 
	cout << "FalseString:\t.string \"FALSE\\n\"\t"<<endl; 
	// Let's proceed to the analysis and code production
	current=(TOKEN) lexer->yylex();
	Program();
	// Trailer for the gcc assembler / linker
	cout << "\tmovq %rbp, %rsp\t\t"<<endl;
	cout << "\tret\t\t\t"<<endl;
	if(current!=FEOF){
		cerr <<"Caractères en trop à la fin du programme : ["<<current<<"]";
		Erreur("."); // unexpected characters at the end of program
	}
}
