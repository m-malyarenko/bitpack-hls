###############################################################################
# Project properties
###############################################################################

PROJECT_NAME = bitpack-hls
PROJECT_VERSION = 0.1.0
PROJECT_TYPE = BIN

###############################################################################
# Project directory structure variables and rules
###############################################################################

# Project directories
SRC_DIR = ./src
INCLUDE_DIR = ./include
BUILD_DIR = ./build
LIB_DIR = ./lib
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj

# Search directories
vpath %.o $(OBJ_DIR)
vpath %.cpp $(shell find ./src -type d -printf "%p ")
vpath %.hpp $(INCLUDE_DIR) $(shell find ./src -type d -printf "%p ")

# Rules for creating a build directory tree
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR): $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

$(OBJ_DIR): $(BUILD_DIR)
	@mkdir -p $(OBJ_DIR)

###############################################################################
# Build properties
###############################################################################

# Build type ------------------------------------------------------------------

# Build type: DEBUG, RELEASE
BUILD_TYPE = DEBUG

# Tools -----------------------------------------------------------------------

# Compiler
CXX = clang++

# Archiver
AR = ar

# Path ------------------------------------------------------------------------

# Include path
INCLUDE_PATH = $(addprefix -I ,$(shell find $(INCLUDE_DIR) -type d -printf "%p "))
INCLUDE_PATH += -I /usr/include/llvm-14 -I /usr/include/llvm-c-14

# Library path
# LIB_PATH = $(addprefix -L ,$(shell find $(LIB_DIR) -type d -printf "%p "))
# LIB_PATH +=

# Libraries -------------------------------------------------------------------

LIB = LLVM-14 lpsolve55 colamd

LIB_LINK = $(addprefix -l,$(LIB))

# Flags -----------------------------------------------------------------------

# Compiler flags
CXX_FLAGS = -std=c++17 -Wall -Wpedantic $(INCLUDE_PATH)

# Linker flags
LD_FLAGS = $(LIB_PATH) $(LIB_LINK)

ifeq ($(BUILD_TYPE), DEBUG)
CXX_FLAGS += -O1 -g
else ifeq ($(BUILD_TYPE), RELEASE)
CXX_FLAGS += -O2 -D NDEBUG
else
$(error Invalid BUILD_TYPE. Possible values: DEBUG, RELEASE)
endif

ifeq ($(PROJECT_TYPE), BIN)
TARGET_RULE = build-bin
else ifeq ($(PROJECT_TYPE), SLIB)
TARGET_RULE = build-static-lib
else
$(error Invalid PROJECT_TYPE. Possible values: BIN, SLIB)
endif

###############################################################################
# Build rules
###############################################################################

# List of source files
SOURCES := $(notdir $(shell find ./src -type f -name "*.cpp" -printf "%p "))

# List of object files
OBJECTS := $(SOURCES:%.cpp=$(OBJ_DIR)/%.o)

# Build object file from source
$(OBJ_DIR)/%.o: %.cpp
	@echo
	@echo "Building target: $(notdir $@)"
	$(CXX) -c $(CXX_FLAGS) -o $@ $<

# Build binary from object files
.PHONY: build-bin
build-bin: $(OBJECTS)
	@echo
	@echo "Building target: $(PROJECT_NAME)"
	$(CXX) -o $(BIN_DIR)/$(PROJECT_NAME) $^ $(LD_FLAGS)

# Build static library from object files
.PHONY: build-static-lib
build-static-lib: $(OBJECTS)
	@echo
	@echo "Building target: lib$(PROJECT_NAME).a"
	$(CXX) crs $(BIN_DIR)/lib$(PROJECT_NAME).a $^

# Build all
.PHONY: all
all: $(OBJ_DIR) $(BIN_DIR) $(TARGET_RULE)

ifeq ($(PROJECT_TYPE), BIN)
.PNONY: run
run: all
	@echo
	@echo "Running program $(PROJECT_NAME)"
	$(BIN_DIR)/$(PROJECT_NAME) $(ARGS)
endif

###############################################################################
# Utility rules
###############################################################################

.PHONY: clean
clean:
	@rm -f -r $(BUILD_DIR)
