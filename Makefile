EXE = git_controller.out
SOURCES = gc_controller.c gc_broker.c gc_config.c gh_string.c gh_log.c gh_buffer.c
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

CXX = gcc
CXXFLAGS1 = -g -Wall -Wformat
CXXFLAGS2 = 
CXXFLAGS2LIB =
LIBS = -I. -I../nginx/src/core -I/usr/local/include/libbson-1.0 -I/usr/local/include/libmongoc-1.0 \
-I../redis/ -L../libgit2/build/ -lgit2 -L../redis/ -lhiredis -levent -lmongoc-1.0 -lbson-1.0 \
-I/usr/include/lua5.3/ -L/usr/local/lib -llua -lm -ldl

%.o: %.c
	$(CXX) $(CXXFLAGS1) $(LIBS) -c $< -o $@

$(EXE): $(OBJS)
	$(CXX) -o libgithttp.so $^ $(CXXFLAGS2LIB) $(LIBS)
	$(CXX) -o $@ $^ $(CXXFLAGS2) $(LIBS)
	
clean:
	rm -f $(EXE), $(OBJS), libgithttp.so
