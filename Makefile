CC = g++
CFLAGS = -g -Wall -std=c++17

# FAISS and Homebrew installation path
HOMEBREW_PREFIX = /opt/homebrew  # Adjust if you're on an Intel Mac
FAISS_PREFIX = ${HOMEBREW_PREFIX}/opt/faiss
LOMP_PREFIX = ${HOMEBREW_PREFIX}/opt/libomp
CATCH2_PREFIX = ${HOMEBREW_PREFIX}/opt/lcatch2


# Update include and library paths
INCLUDES = -I$(FAISS_PREFIX)/include -I${LOMP_PREFIX}/include -I${CATCH2_PREFIX}/include
LFLAGS = -L$(FAISS_PREFIX)/lib -L${LOMP_PREFIX}/lib -L${CATCH2_PREFIX}/lib

# Specify libraries to link against
LIBS = -lfaiss -lomp -lcatch2  # Ensure libomp is installed

# define the C++ source files
SRCS = main.cpp db_client.cpp


# define the C++ object files 
OBJS = $(SRCS:.cpp=.o)

# define the executable file 
MAIN = main

.PHONY: depend clean

all:    $(MAIN)
	@echo  Compiled Successfully!

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it
