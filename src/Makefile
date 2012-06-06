# 
# ECE454 Project 1
# Ravil Baizhiyenov
# Robin Vierich
#

CXX = g++
CXXFLAGS = -Wall
LFLAGS = -Wall

OBJS = main.o Cli.o Client.o

LIBS = -lboost_thread -lboost_system

PROJECT_NAME = ece454_project1

OUT_DIR = ../build
OUT_OBJS = $(addprefix $(OUT_DIR)/,$(OBJS))
EXEC = $(addprefix $(OUT_DIR)/,$(PROJECT_NAME))

$(EXEC) : $(OUT_OBJS)
	$(CXX) -o $@ $(LFLAGS) $(LIBS) $^

#$(OUT_DIR)/%.o: %.cpp
#	$(CXX) $(CXXFLAGS) $(DEBUG) -c -o $@ $<

$(OUT_DIR)/main.o: main.cpp CliCmdHandler.h Client.h
	$(CXX) $(CXXFLAGS) $(DEBUG) -c -o $(OUT_DIR)/main.o main.cpp

$(OUT_DIR)/Cli.o: Cli.h Cli.cpp CliCmdHandler.h
	$(CXX) $(CXXFLAGS) $(DEBUG) -c -o $(OUT_DIR)/Cli.o Cli.cpp

$(OUT_DIR)/Cli.o: Client.h
	$(CXX) $(CXXFLAGS) $(DEBUG) -c -o $(OUT_DIR)/Client.o Client.cpp


.PHONY : clean debug
clean :
	rm -f $(OUT_DIR)/*.o $(EXEC)
debug : 
	$(MAKE) $(MAKEFILE) DEBUG="-g -D DEBUG"

# Crate the build directory. Sub-optimal impl, but should do it for now.
$(shell   mkdir -p $(OUT_DIR))