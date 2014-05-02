all:
	g++ -o bin/jdecompiler.exe -std=c++11 -Wall -Werror -Wfatal-errors src/ClassFile.cpp src/main.cpp
