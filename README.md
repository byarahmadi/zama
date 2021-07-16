For using this compiler you need to have llvm 6.0.1 on your computer.

To install the compiler you need to run the following command :

`make install`

To run you can use the run.sh script with the input file 

For instance :
`run.sh examples/example1.txt`
`a=1`
`b=1`
`c=1`
`The result is 1`

The compiler has three parts :
- A lexer and a parser **lp.cpp**
- An LLVM pass **EmitCodePass.cpp**
- A simulator **sym.cpp**

lp.cpp contaions a lexer for tokenizing the language input and a recursive decent parser for parsing and generating the AST. It also generates the LLVM IR code (**IR.bc**) from 
the generated AST. The recursive decent parser is a modification of Kaleidoscope language parser :https://releases.llvm.org/6.0.0/docs/tutorial/LangImpl02.html

The llvm pass (EmitCodePass.cpp) is resposible for lowering and transofrming LLVM IR to the nand machine code. It generates an **output** file contatining machine code as well as a file called 
**loads** containig the variables in the program

The simulator executes the generated machine code. If there are variables in the program, it asks the user for values from the standard input. Values in logic are either true of false. The user can input 
1 for indicating true and 0 for indicating false.

Some comments :

It was possible to generate the final machine code from the AST as the machine language was so simple and straightforward(unlimited number of registers). However, transforming the language to the intermidiete code like LLVM IR  makes the compiler extensible. Also, it provdides using hundereds of  optimizations available in the reach compiler ecosystem like LLVM. 

The high level language has three operations (**not**, **and** , **or**). However, the nand machine has ony one operation which is **nand**. Most logic operations can be made by nand which in this project have been used. https://en.wikipedia.org/wiki/NAND_logic   
