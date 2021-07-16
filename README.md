For using this compiler you need to have llvm 6.0.1 on your computer.

To install the compiler you need to run the following command :

`make install`

To run you can use the run.sh script with the input file 

For instance :
`run.sh examples/example1.txt`

The compiler has three parts :
- A lexer and a parser **lp.cpp**
- An LLVM pass **EmitCodePass.cpp**
- A simulator **sym.cpp**

lp.cpp contaions a lexer for tokenizing the language input and a recursive decent parser for parsing and generating the AST. It also generates the LLVM IR code (**IR.bc**) from 
the generated AST. The recursive decent parser is a modification of Kaleidoscope language parser :https://releases.llvm.org/6.0.0/docs/tutorial/LangImpl02.html

The llvm pass (EmitCodePass.cpp) is resposible for lowering and transofrming LLVM IR to the nand machine code. It generates an **output** file contatining machine code as well as a file called 
**loads** containig the variables in the program

The simulator executes the generated machine code. If there are variables in the program, it asks the user for values from the standard input. 
