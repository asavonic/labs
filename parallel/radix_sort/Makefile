CC      = mpic++
CFLAGS  = -fopenmp --std=c++11
OPT     = -O2
INCLUDE = -I common -I openmp -I simple -I tbb -I mpi -I .
LIBS    = -lboost_program_options -lboost_mpi -lboost_serialization -ltbb
SRC     = main.cpp

OBJ     = $(SRC:.c=.o)

MAIN    = radix

.PHONY: depend clean


all: $(MAIN)

debug: OPT = -O0 -g
debug: $(MAIN)

debugopt: OPT = -O2 -g
debugopt: $(MAIN)

$(MAIN): $(OBJ) 
	$(CC) $(CFLAGS) $(OPT) $(INCLUDE) -o $(MAIN) $(OBJ) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(OPT) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

