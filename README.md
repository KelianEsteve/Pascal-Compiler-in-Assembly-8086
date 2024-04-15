# Pascal-Compiler-in-Assembly-8086

This project is a compiler written in C++ that translates source code in a specific language into assembly code. The compiler consists of two main files: compiler.cpp and tokeniser.l, along with a Makefile for compilation.

# Language Overview

The specific language targeted by this compiler is a custom language designed for educational purposes. It features basic programming constructs such as variable declarations, assignments, control flow statements (if-else, loops), and simple expressions.

# Supported Constructs

Variable Declarations: Users can declare variables of different types, including integers, booleans, and integer arrays.
Assignment Statements: Variables can be assigned values using the := operator.
Display Statements: The DISPLAY statement allows printing integer values to the console.
Control Flow: The language supports IF-THEN-ELSE statements for conditional execution and FOR and WHILE loops for iterative execution.
Case Statements: It includes support for CASE statements for multi-way branching based on the value of an expression.

# Supported Types

Integer: Represented by the INTEGER keyword.
Boolean: Represented by the BOOLEAN keyword.
Integer Array: Declared using the ARRAY keyword, allowing the declaration of arrays of integers.
Compilation Process

# The compilation process involves several steps:

Tokenization: The source code is tokenized using Flex, a lexical analyzer generator. Tokens represent the smallest units of the language, such as keywords, identifiers, numbers, and operators.
Parsing: The tokenized input is parsed to analyze its structure and verify its correctness according to the grammar rules of the specific language.
Code Generation: Based on the parsed input, the compiler generates equivalent assembly code. This code is tailored to the target architecture and is designed to efficiently execute the operations specified in the source code.
Output: The generated assembly code can be outputted to a file or directly executed, depending on the compiler's configuration and the user's requirements.
