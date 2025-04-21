OUT = main.exe
SRC = main.cpp
CORE = core/core.hpp core/tokenizer.hpp core/json.hpp core/ruleset.hpp core/ruleparser.hpp core/compiler.hpp core/runtimebase.hpp core/runtime.hpp
CORE2 = core2/core.hpp core2/tokenizer.hpp core2/astruleset.hpp
TINYBASIC = tinybasic/tinybasicparser.hpp tinybasic/tinybasiccompiler.hpp
HEAD = $(CORE) $(CORE2) $(TINYBASIC)

$(OUT): $(SRC) $(HEAD)
	g++ -std=c++17 -O0 -gdwarf -Wall -o $(OUT) $(SRC)

run: $(OUT)
	./$(OUT)

clean:
	rm -f *.out *.exe
