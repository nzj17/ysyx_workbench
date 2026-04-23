objects =sim_main.cpp top.v
all:
	@echo "Write this Makefile by your self."

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@echo "Write this Makefile by your self."

include ../Makefile
sim_vcd: $(objects)
	verilator --cc --trace top.v --exe --build -j 0 -Wall sim_main.cpp 

sim_fst:$(objects)
	verilator --cc --trace-fst top.v --exe --build -j 0 -Wall sim_main.cpp 
