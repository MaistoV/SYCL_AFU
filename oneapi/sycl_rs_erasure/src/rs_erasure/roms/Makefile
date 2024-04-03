RM  := rm -rf
OUT_DIR ?= ./

# If ISA-L is not installed in the default libraries directories (e.g., in /usr/lib/), define:
# LD_ISAL_FLAG = -L/path/to/libisal.so
rom_gen: src/rs_rom_gen.c src/rs_rom_utils.c
	gcc $^ -o $@ -lisal $(LD_ISAL_FLAG)

rom_gen_multi_erasure: src/rs_rom_gen_multi_erasure.c src/rs_rom_utils.c
	gcc $^ -o $@ -lisal

survival_gen: src/survival_gen.c
	g++ $^ -o $@

erasure_gen: src/erasure_gen.c
	g++ $^ -o $@

regen_perms: survival_gen erasure_gen
	./survival_gen 3  2 > survival_patterns_3_2.txt
	./survival_gen 6  3 > survival_patterns_6_3.txt
	./survival_gen 10 4 > survival_patterns_10_4.txt
	./erasure_gen $$((3+2))  2 > erasure_patterns_3_2.txt
	./erasure_gen $$((6+3))  3 > erasure_patterns_6_3.txt
	./erasure_gen $$((10+4)) 4 > erasure_patterns_10_4.txt

regen_roms: rom_gen regen_perms
	./rom_gen -k 3 	-p 2 -l 1 -e 1 -o $(OUT_DIR)
	./rom_gen -k 6 	-p 3 -l 1 -e 1 -o $(OUT_DIR)
	./rom_gen -k 10 -p 4 -l 1 -e 1 -o $(OUT_DIR)

regen_roms_multi_erasure: rom_gen_multi_erasure regen_perms
	./rom_gen_multi_erasure -k 3 	-p 2 -l 1 -e 1 -o $(OUT_DIR)
	./rom_gen_multi_erasure -k 6 	-p 3 -l 1 -e 1 -o $(OUT_DIR)
	./rom_gen_multi_erasure -k 10	-p 4 -l 1 -e 1 -o $(OUT_DIR)

clean:
	$(RM) rom_gen regen_roms_multi_erasure erasure_gen survival_gen




