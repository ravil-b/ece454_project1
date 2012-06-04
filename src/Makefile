CXX = g++
CXXFLAGS = -Wall

OBJS = main.o

PROJECT_NAME = ece454_project1

OUT_DIR = ../build
OUT_OBJS = $(addprefix $(OUT_DIR)/,$(OBJS))
EXEC = $(addprefix $(OUT_DIR)/,$(PROJECT_NAME))

$(EXEC) : $(OUT_OBJS)
	$(CXX) $^ -o $@

$(OUT_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEBUG) -c -o $@ $<


.PHONY : clean debug
clean :
	rm -f $(OUT_DIR)/*.o $(EXEC)
debug : 
	$(MAKE) $(MAKEFILE) DEBUG="-g -D DEBUG"