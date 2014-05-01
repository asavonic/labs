CC      = g++
CFLAGS  = -O2 -fopenmp --std=c++11
INCLUDE = -I common -I openmp -I simple -I .
LIBS    = -lboost_program_options
SRC     = main.cpp

OBJ     = $(SRC:.c=.o)

MAIN    = radix

.PHONY: depend clean

all:    $(MAIN)

$(MAIN): $(OBJ) 
	$(CC) $(CFLAGS) $(INCLUDE) -o $(MAIN) $(OBJ) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

