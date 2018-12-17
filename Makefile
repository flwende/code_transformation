CXX = g++
CFLAGS = -std=c++11 -fno-rtti -I./include

LLVM=llvm-8
LLVM_SRC_PATH = /usr/lib/$(LLVM)
LLVM_BIN_PATH = $(LLVM_SRC_PATH)/bin
LLVM_LIBS=core mc
LLVM_CONFIG_COMMAND = $(LLVM_BIN_PATH)/llvm-config --cxxflags --ldflags --libs $(LLVM_LIBS)
CLANG_BUILD_FLAGS = -I$(LLVM_SRC_PATH)/include

#CLANGLIBS = \
#  -lclangFrontendTool -lclangFrontend -lclangDriver \
#  -lclangSerialization -lclangCodeGen -lclangParse \
#  -lclangSema -lclangStaticAnalyzerFrontend \
#  -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore \
#  -lclangAnalysis -lclangARCMigrate -lclangRewrite \
#  -lclangEdit -lclangAST -lclangLex -lclangBasic \
#  -lclangTooling -lclangASTMatchers

CLANGLIBS = \
	/usr/lib/$(LLVM)/lib/libclangFrontendTool.a \
	/usr/lib/$(LLVM)/lib/libclangFrontend.a \
	/usr/lib/$(LLVM)/lib/libclangDriver.a \
	/usr/lib/$(LLVM)/lib/libclangSerialization.a \
	/usr/lib/$(LLVM)/lib/libclangCodeGen.a \
	/usr/lib/$(LLVM)/lib/libclangParse.a \
	/usr/lib/$(LLVM)/lib/libclangSema.a \
	/usr/lib/$(LLVM)/lib/libclangStaticAnalyzerFrontend.a \
	/usr/lib/$(LLVM)/lib/libclangStaticAnalyzerCheckers.a \
	/usr/lib/$(LLVM)/lib/libclangStaticAnalyzerCore.a \
	/usr/lib/$(LLVM)/lib/libclangAnalysis.a \
	/usr/lib/$(LLVM)/lib/libclangARCMigrate.a \
	/usr/lib/$(LLVM)/lib/libclangRewrite.a \
	/usr/lib/$(LLVM)/lib/libclangEdit.a \
	/usr/lib/$(LLVM)/lib/libclangAST.a \
	/usr/lib/$(LLVM)/lib/libclangLex.a \
	/usr/lib/$(LLVM)/lib/libclangBasic.a \
	/usr/lib/$(LLVM)/lib/libclangTooling.a \
	/usr/lib/$(LLVM)/lib/libclangASTMatchers.a

all: bin/test_proxy_gen.x

bin/test_proxy_gen.x: obj/test_proxy_gen.o
	$(CXX) -o $@ $< $(CLANGLIBS) `$(LLVM_CONFIG_COMMAND)`

obj/test_proxy_gen.o: src/test_proxy_gen.cpp
	$(CXX) $(CFLAGS) $(CLANG_BUILD_FLAGS) -o $@ -c $<

clean:
	rm -rf *~ obj/*.o bin/test_proxy_gen.x
