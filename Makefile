FILES = src/main.cpp src/ClassFile.cpp src/ClassOutput.cpp src/MethodOutput.cpp src/FieldOutput.cpp src/Helpers.cpp
OPTS  = -std=c++11 -Wall -Werror -Wfatal-errors
all:
	g++ -g -o bin/jdecompiler.exe $(OPTS) $(FILES)
