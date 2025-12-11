SRC_DIR			:= src/
BIN_DIR			:= bin/
OBJ_DIR			:= bin/obj/

CC				:= g++
CC_FLAGS		:= -std=c++20 -g -Wall# -O3 -D NDEBUG
CC_INCLUDE		:= -I/usr/include/ -Ilib -Ilib/imgui -Ilib/glm-1.0.2 -Ilib/imgui/backends

LD				:= g++
LD_FLAGS		:= -g
LD_INCLUDE		:= -lpthread -lglfw -lvulkan -ldl -lX11 -lXrandr -lXi

DEP_FLAGS		:= -MMD -MP

CC_FILES_IN		:= $(wildcard $(SRC_DIR)*.cpp) $(wildcard $(SRC_DIR)lib/*.cpp)
CC_FILES_OUT	:= $(patsubst $(SRC_DIR)%.cpp, $(OBJ_DIR)%.o, $(CC_FILES_IN))
CC_FILES_DEP	:= $(patsubst $(SRC_DIR)%.cpp, $(OBJ_DIR)%.d, $(CC_FILES_IN))

CC_FILES_IN_PB	:= src/package-builder/package-builder.cpp src/package.cpp src/debug.cpp src/exec.cpp
CC_FILES_OUT_PB	:= $(patsubst $(SRC_DIR)%.cpp, $(OBJ_DIR)%.o, $(CC_FILES_IN_PB))
CC_FILES_DEP_PB := $(patsubst $(SRC_DIR)%.cpp, $(OBJ_DIR)%.d, $(CC_FILES_IN_PB))

EXE_OUT			:= $(BIN_DIR)hop-engine
EXE_OUT_PB		:= $(BIN_DIR)package-builder

.PHONY: clean $(BIN_DIR) $(OBJ_DIR)

all: execute

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@mkdir -p $(dir $@)
	@echo "Compiling" $< to $@
	@$(CC) $(CC_FLAGS) $(CC_INCLUDE) $(DEP_FLAGS) -c $< -o $@

-include $(CC_FILES_DEP) $(CC_FILES_DEP_PB)

$(EXE_OUT): $(CC_FILES_OUT)
	@echo "Linking" $@
	@$(LD) $(LD_FLAGS) -o $@ $(CC_FILES_OUT) $(LD_INCLUDE)

$(EXE_OUT_PB): $(CC_FILES_OUT_PB)
	@echo "Linking" $@
	@$(LD) $(LD_FLAGS) -o $@ $(CC_FILES_OUT_PB) $(LD_INCLUDE)

build: $(EXE_OUT)

package-builder: $(EXE_OUT_PB)

execute: $(EXE_OUT) package-builder
	$(EXE_OUT_PB) res -c resources.hop
	@$(EXE_OUT)

clean:
	@rm -r $(BIN_DIR)
	