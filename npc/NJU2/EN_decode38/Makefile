TOPNAME = decode38
OBJ_DIR = ./obj_dir
VTOPNAME=V$(TOPNAME)
WAVENAME=./wave.fst
BIN=./obj_dir/$(VTOPNAME)
VSRCS = $(shell find $(abspath ./) -name "*.v")
CSRCS = $(shell find $(abspath ./) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
all:
	@echo "Write this Makefile by your self."
	make clean
	make sim
	make run
	make fst
sim:$(VSRCS) $(CSRCS)
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	echo "Write this Makefile by your self."
	verilator --cc   --top-module $(TOPNAME) --trace-fst $(VSRCS) --exe --Mdir $(OBJ_DIR) --build -j 0 -Wall $(CSRCS) 
clean:
	rm -rf $(OBJ_DIR)
run: $(BIN)
	$^
fst: $(WAVENAME)
	gtkwave $^
lint:$(VSRCS)
	verilator --cc --lint-only -Wall --top-module $(TOPNAME) --lint-only  -Wall $(VSRCS) 
	
