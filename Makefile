CPP := g++
CPPFLAGS := -O3 -mtune=native -march=native -mfpmath=both
OBJS := main.o

compile: $(OBJS)
	$(CPP) $(CPPFLAGS) $(OBJS) -o mlisp

clean:
	rm $(OBJS) mlisp
