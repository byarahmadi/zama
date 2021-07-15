LLVM_CONFIG?=llvm-config
CXX=clang++
CD=cd
PASSDIR=pass/
SYMDIR=sym/
FLAGS+=$(shell $(LLVM_CONFIG) --cxxflags --ldflags --system-libs --libs)
COMMON_FLAGS=-Wall -Wextra






install:
	$(CXX) lp.cpp $(FLAGS) -O0 -o lp.bin 
	$(CD) $(PASSDIR) && make
	$(CXX) $(SYMDIR)sym.cpp -std=c++11 -o $(SYMDIR)sym.bin
	

clean:
	rm -f *.bin IR.bc IR.ll output loads sym/sym.bin
	$(CD) $(PASSDIR) && make clean
