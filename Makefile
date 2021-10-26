SRC_DIR = src
OBJ_DIR = obj
HEADERS = $(wildcard $(SRC_DIR)/*.h)
CPPs = $(wildcard $(SRC_DIR)/*.cpp)
OBJs = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(CPPs))
CPPFLAGs = -std=c++17 -O3

main: $(OBJs)
	g++ -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	g++ $(CPPFLAGs) -c -o $@ $<

.PHONY: init clean
init:
	test -e $(OBJ_DIR) || mkdir $(OBJ_DIR)
	cp ../inverted-index/index/* index/
clean:
	rm main $(OBJ_DIR)/*
