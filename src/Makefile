# 
# ECE454 Project 1
# Ravil Baizhiyenov
# Robin Vierich
#

PLATFORM = LINUX

ifdef SystemRoot # This is only defined on windows
   PLATFORM = WIN
endif

CXX = g++
CXXFLAGS = -Wall
LFLAGS = -Wall

LIBS = -lboost_system
OBJS = main.o Cli.o Client.o Server.o

ifeq ($(PLATFORM), WIN)
	INCLUDES = -I"C:\boost\boost_1_49_0"
	LIBPATH = -L"C:\boost\boost_1_49_0\stage\lib"
	WINLIBEXTENSION = -mgw46-mt-d-1_49 # minGW linker - built with MultiThreading - debug enabled - v1.49
	LIBS = -lboost_thread$(WINLIBEXTENSION) -lboost_system$(WINLIBEXTENSION) -lws2_32 # need to link with winsock32 as well
else
ifneq (,$(findstring x86_64,$(shell uname -a)))
	LIBS += -lboost_thread-mt
else
	LIBS += -lboost_thread
endif
endif


PROJECT_NAME = ece454_project1

OUT_DIR = ../build
OUT_OBJS = $(addprefix $(OUT_DIR)/,$(OBJS))
EXEC = $(addprefix $(OUT_DIR)/,$(PROJECT_NAME))

# Lining the object files
$(EXEC) : $(OUT_OBJS)
	$(CXX) -o $@ $(LFLAGS) $^ $(LIBPATH) $(LIBS)

# Compiling
$(OUT_DIR)/main.o: main.cpp CliCmdHandler.h Client.h
	$(CXX) $(CXXFLAGS) $(DEBUG) $(INCLUDES) -c -o $(OUT_DIR)/main.o main.cpp

$(OUT_DIR)/Cli.o: Cli.h Cli.cpp CliCmdHandler.h
	$(CXX) $(CXXFLAGS) $(DEBUG) $(INCLUDES) -c -o $(OUT_DIR)/Cli.o Cli.cpp

$(OUT_DIR)/Client.o: Client.h Client.cpp
	$(CXX) $(CXXFLAGS) $(DEBUG) $(INCLUDES) -c -o $(OUT_DIR)/Client.o Client.cpp

$(OUT_DIR)/Server.o: Server.h Server.cpp
	$(CXX) $(CXXFLAGS) $(DEBUG) $(INCLUDES) -c -o $(OUT_DIR)/Server.o Server.cpp

# clean and debug targets
.PHONY : clean debug
clean :
	rm -f $(OUT_DIR)/*.o $(EXEC)
debug : 
	$(MAKE) $(MAKEFILE) DEBUG="-g -D DEBUG"

# Crate the build directory. Sub-optimal impl, but should do it for now.
$(shell   mkdir -p $(OUT_DIR))