CC        := clang++ -Wall -g -O3 -std=c++0x -march=native -mtune=native -fno-omit-frame-pointer
LD        := -lboost_date_time -lboost_thread -lboost_program_options -lpthread -lboost_system -lrt

SRC_DIR   := ./
BUILD_DIR := ./
INC_DIR   := ./
EXE := ./gibbs

SRC       := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
OBJ       := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC))
INCLUDES  := $(addprefix -I,$(INC_DIR))

vpath %.cpp $(SRC_DIR)

$(BUILD_DIR)/%.o: %.cpp
		@echo $<
			@$(CC) $(INCLUDES) -c $< -o $@


.PHONY: all checkdirs clean

all: checkdirs $(BUILD_DIR)/$(EXE)

$(BUILD_DIR)/$(EXE): $(OBJ)
		@echo linking
			@g++ -static $^ $(LD) -o $@


checkdirs: $(BUILD_DIR)

$(BUILD_DIR):
		@mkdir -p $@

clean:
		@rm -rf $(EXE) *.o

