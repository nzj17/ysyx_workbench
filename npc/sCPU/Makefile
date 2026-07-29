TOPNAME = sum
NXDC_FILES = constr/top.nxdc
INC_PATH ?=

VERILATOR = verilator
VERILATOR_CFLAGS += -MMD --build -cc  \
				-O3 --x-assign fast --x-initial fast --noassert

BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
BIN = $(BUILD_DIR)/$(TOPNAME)

default: $(BIN)

$(shell mkdir -p $(BUILD_DIR))

# constraint file
SRC_AUTO_BIND = $(abspath $(BUILD_DIR)/auto_bind.cpp)
$(SRC_AUTO_BIND): $(NXDC_FILES)
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $^ $@

# project source
VSRCS = ./vsrc/sum.v
CSRCS = $(shell find $(abspath ./csrc) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
CSRCS += $(SRC_AUTO_BIND)

# rules for NVBoard
include $(NVBOARD_HOME)/scripts/nvboard.mk

# rules for verilator
INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""

$(BIN): $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE)
	@rm -rf $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(TOPNAME) $^ \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))
# verilator  -MMD --build -cc -O3 --x-assign fast --x-initial fast --noassert  --top-module top $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE) $(addprefix -CFLAGS , $(addprefix-I, $(INC_PATH))) $(addprefix -LDFLAGS , $(LDFLAGS)) --Mdir ./build/obj_dir --exe -o $(abspath ./build/top)
all: default
     
run: $(BIN)
	@$^
lint:$(VSRCS)
	verilator --cc --lint-only -Wall --top-module $(TOPNAME) --lint-only  -Wall $(VSRCS) 
clean:
	rm -rf $(BUILD_DIR)
.PHONY: default all clean run
