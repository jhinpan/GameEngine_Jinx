# Makefile

# Compiler
CXX = emcc

# Current directory where Makefile resides
CUR_DIR = $(shell pwd)

# Include directories with dynamic path for LuaBridge
INCLUDES = -I$(CUR_DIR)/glm -I$(CUR_DIR)/rapidjson
INCLUDES += -I$(CUR_DIR)/Lua -I$(CUR_DIR)/LuaBridge
INCLUDES += -I$(CUR_DIR)/box2d/include -I$(CUR_DIR)/box2d/include/box2d -I$(CUR_DIR)/box2d -I$(CUR_DIR)/box2d/dynamics
INCLUDES += -I$(CUR_DIR)/emsdk/upstream/emscripten/cache/sysroot/include

# Compiler flags
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 $(INCLUDES)

# Verbose mode
ifdef VERBOSE
CXXFLAGS += -v
LDFLAGS += -v
endif

# Emscripten Flags
EM_FLAGS = -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -s USE_SDL_TTF=2 -s USE_SDL_MIXER=2
EM_FLAGS += -s WASM=1 --preload-file resources
EM_FLAGS += -s ALLOW_MEMORY_GROWTH=1
EM_FLAGS += --shell-file $(SHELL_FILE) # Use a variable for the HTML shell file

# Now in the compiler flags or linker flags, just add $(EM_FLAGS)
CXXFLAGS += $(EM_FLAGS)

# Assuming the Lua library is in $(CUR_DIR)/Lua
LDFLAGS += -L$(CUR_DIR)/Lua -llua $(EM_FLAGS)

# Name of the final executable
EXECUTABLE = game_engine_web.html

# Source files
SOURCES = main.cpp $(wildcard box2d/*.cpp) Actor.cpp

# Object files (ensure they are generated in the current directory or a specific build directory)
OBJECTS = $(SOURCES:%.cpp=%.o)

# Rule for making the final executable
$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

# Rule for making object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Default HTML shell file
SHELL_FILE ?= $(CUR_DIR)/retro_shell.html

# Clean
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
