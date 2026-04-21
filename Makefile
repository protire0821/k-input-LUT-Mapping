# Makefile for PA2
.PHONY: all run clean

# ---------- build ----------
all: pa2_main.o pa2_blif_parser.o pa2_aig_builder.o pa2_blif_writer.o pa2_cut_selector.o pa2_lut_builder.o
	@g++ -std=c++17 pa2_main.o pa2_blif_parser.o pa2_aig_builder.o pa2_blif_writer.o pa2_cut_selector.o pa2_lut_builder.o -o 114521123_PA2

# ---------- run ----------
# make run input=<input.blif> output=<output.blif> k=<2~10>
run: 
	@./114521123_PA2 -input $(input) -output $(output) -k $(k)

# ---------- clean ----------
clean:
	@rm -f *.o 114521123_PA2 114521123_PA2.exe

# ---------- compile ----------
pa2_main.o: 114521123_PA2.cpp
	@g++ -std=c++17 -c 114521123_PA2.cpp -o pa2_main.o

pa2_blif_parser.o: inc/blif_parser.h src/blif_parser.cpp
	@g++ -std=c++17 -c src/blif_parser.cpp -o pa2_blif_parser.o

pa2_aig_builder.o: inc/aig_builder.h src/aig_builder.cpp
	@g++ -std=c++17 -c src/aig_builder.cpp -o pa2_aig_builder.o

pa2_blif_writer.o: inc/blif_writer.h src/blif_writer.cpp
	@g++ -std=c++17 -c src/blif_writer.cpp -o pa2_blif_writer.o

pa2_cut_selector.o: inc/cut_selector.h src/cut_selector.cpp
	@g++ -std=c++17 -c src/cut_selector.cpp -o pa2_cut_selector.o

pa2_lut_builder.o: inc/lut_builder.h src/lut_builder.cpp
	@g++ -std=c++17 -c src/lut_builder.cpp -o pa2_lut_builder.o
