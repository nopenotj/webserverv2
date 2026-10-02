
run: a.out
	./a.out

a.out: *.cpp lib/*.cpp lib/*.h
	g++ -g lib/*.cpp *.cpp


test_runner: test/* lib/*
	g++  lib/*.cpp test/*.cpp -o test_runner

test: test_runner
	./test_runner
