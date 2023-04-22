all:
	@echo "Write this Makefile by your self."

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	//@echo "Write this Makefile by your self."
	
TOPNAME = top
##NXDC_FILES = constr/top.nxdc
INC_PATH ?=

LLVM_CXXFLAGS += $(shell llvm-config --cxxflags) -fPIE
LLVM_LIBS += $(shell llvm-config --libs)

VERILATOR = verilator
VERILATOR_CFLAGS += -MMD --build -cc  \
				-O3 --x-assign fast --x-initial fast --noassert --trace
#VERILATOR_CFLAGS += $(LLVM_CXXFLAGS) 

BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
BIN = $(BUILD_DIR)/$(TOPNAME)

default: $(BIN)

$(shell mkdir -p $(BUILD_DIR))


# constraint file
#SRC_AUTO_BIND = $(abspath $(BUILD_DIR)/auto_bind.cpp)
#$(SRC_AUTO_BIND): $(NXDC_FILES)
#	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $^ $@

# project source
VSRCS = $(shell find $(abspath ./vsrc) -name "*.v")
CSRCS = $(shell find $(abspath ./csrc) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
#CSRCS += $(SRC_AUTO_BIND)

# rules for NVBoard
#include $(NVBOARD_HOME)/scripts/nvboard.mk

# Include all filelist.mk to merge file lists
#FILELIST_MK = $(shell find ./csrc -name "filelist.mk")
#include $(FILELIST_MK)

# rules for verilator
INCFLAGS = $(addprefix -I, $(INC_PATH))
CFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""
LDFLAGS += -lSDL2 -lSDL2_image
#LDFLAGS += $(LLVM_LIBS)

$(BIN): $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE)
	@rm -rf $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(TOPNAME) $^ \
		$(addprefix -CFLAGS , $(CFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))

all: default

# Command to execute NEMU
IMG ?=
NPC_EXEC := $(BIN) $(IMG)

run-env: $(BIN) $(DIFF_REF_SO)

run: run-env
	$(call git_commit, "run NPC")
	$(NPC_EXEC)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: default all clean run

#include $(NPC_HOME)/tools/difftest.mk
include ../Makefile
