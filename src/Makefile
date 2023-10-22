CPP  := g++
CPPFLAGS := -O3 -mtune=native -march=native -mfpmath=both
OBJS := env.o eval.o lexer.o object.o parser.o token.o main.o

compile: $(OBJS)
	$(CPP) $(CPPFLAGS) $(OBJS) -o mlisp

clean:
	rm $(OBJS) mlisp
