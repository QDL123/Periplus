CC = g++
CFLAGS = -g -Wall -std=c++17

# FAISS and Homebrew installation path
HOMEBREW_PREFIX = /opt/homebrew  # Adjust if you're on an Intel Mac
FAISS_PREFIX = /opt/homebrew/opt/faiss
LOMP_PREFIX = /opt/homebrew/opt/libomp
# CATCH2_PREFIX = /opt/homebrew/opt/catch2


# Update include and library paths
INCLUDES = -I$(FAISS_PREFIX)/include -I${LOMP_PREFIX}/include
LFLAGS = -L$(FAISS_PREFIX)/lib -L${LOMP_PREFIX}/lib

# Specify libraries to link against
LIBS = -lfaiss -lomp  # Ensure libomp is installed

# define the C++ source files
SRCS_MAIN = main.cpp db_client.cpp cache.cpp

# define the C++ source files needed for testing
# SRCS_TEST = test_cache.cpp

# define the C++ object files 
OBJS_MAIN = $(SRCS_MAIN:.cpp=.o)
# OBJS_TEST = $(SRCS_TEST:.cpp=.o)

# define the executable file 
MAIN = main
# TEST = test

.PHONY: depend clean

all:    $(MAIN) $(TEST)
	@echo  Compiled Successfully!

$(MAIN): $(OBJS_MAIN) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS_MAIN) $(LFLAGS) $(LIBS)

# $(TEST): $(OBJS_TEST)
# 	$(CC) $(CFLAGS) $(INCLUDES) -o ${TEST} ${OBJS_TEST} $(LFLAGS) $(LIBS)

# test_build: ${TEST}
# 	@echo "Compiled the tests"

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN) $(TEST)

depend: $(SRCS_MAIN) $(SRCS_TEST)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it
