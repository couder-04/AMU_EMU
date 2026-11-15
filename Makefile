CXX ?= c++
CXXFLAGS ?= -std=c++11 -pedantic -Wall -Wextra -Wpointer-arith -Wwrite-strings

.PHONY: all
all: asm emu

asm: assembler/asm.cpp Makefile
	$(CXX) $(CXXFLAGS) assembler/asm.cpp -o asm

emu: emulator/emu.cpp Makefile
	$(CXX) $(CXXFLAGS) emulator/emu.cpp -o emu
