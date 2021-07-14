LLVM_CONFIG?=llvm-config
CXX=clang++
CD=cd
PASSDIR=pass/

FLAGS+=$(shell $(LLVM_CONFIG) --cxxflags --ldflags --system-libs --libs)
COMMON_FLAGS=-Wall -Wextra






install:
	$(CXX) lp.cpp $(FLAGS) -o lp.bin 
	$(CD) $(PASSDIR) && make
	

clean:
	rm -f *.bin IR.bc output
	$(CD) $(PASSDIR) && make clean
