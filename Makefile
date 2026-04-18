# Makefile for PA2
.PHONY: all run clean
# ---------- build ----------
all: pa2_main.o pa2_blif_parser.o pa2_aig_builder.o
	@g++ -std=c++17 pa2_main.o pa2_blif_parser.o pa2_aig_builder.o -o 114521123_PA2

# ---------- run ----------
# make run input=<input.blif> output=<output.blif> k=<2~10>
run: all
	@./114521123_PA2 -input $(input) -output $(output) -k $(k)

# ---------- clean ----------
clean:
	@rm *.o
	@rm 114521123_PA2

# ---------- compile ----------
pa2_main.o: 114521123_PA2.cpp
	@g++ -std=c++17 -c 114521123_PA2.cpp

pa2_blif_parser.o: inc/blif_parser.h src/blif_parser.cpp
	@g++ -std=c++17 -c src/blif_parser.cpp

pa2_aig_builder.o: inc/aig_builder.h src/aig_builder.cpp
	@g++ -std=c++17 -c src/aig_builder.cpp



