SRC_DIR = src
OBJ_DIR = obj
HEADERs = $(wildcard $(SRC_DIR)/*.h)
CPPs = $(wildcard $(SRC_DIR)/*.cpp)
OBJs = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(CPPs))
LDFLAGs = -ltidy -lcurl
CPPFLAGs = -std=c++17 -O3

main: $(OBJs)
	g++ $(LDFLAGs) $(CPPFLAGs) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	g++ $(CPPFLAGs) -c -o $@ $<

.PHONY: init clean
init:
	mkdir temp index $(OBJ_DIR)
clean:
	rm main DOCMETA temp/* index/* $(OBJ_DIR)/*
