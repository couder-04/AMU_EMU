/*
 * Name    : Parth Tomar
 * Roll No : 2501CS09
 *
 * emu.cpp - Emulator for IITPSx processor.
 *
 * Usage: emu [options] prog.o|prog.lst
 *
 * Options:
 * -trace   : Show instructions and register values during execution.
 * -before  : Display memory before execution.
 * -dump    : Display memory after execution.
 * -mem N   : Set memory size.
 * -limit N : Set maximum instruction count.
 * -disasm  : Display program instructions and exit.
 * -isa     : Display instruction set and exit.
 * -version : Display emulator version.
 *
 * Runtime errors:
 * Invalid PC/memory access, division by zero,
 * unknown opcode, or instruction limit exceeded.
 *
 * Exit codes:
 * 0 = Successful execution (HALT)
 * 1 = Usage or loading error
 * 2 = Runtime error
 *
 * Important:
 * - PC increments before executing an instruction.
 * - Binary operations update A; B remains unchanged.
 * - Shift counts outside 0..31: SHL gives 0; SHR gives 0, or -1 if B is negative
 *   (SHR is an arithmetic shift, so it sign-fills).
 * - Arithmetic wraps around 32 bits.
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace {

const char *kNames[] = {"ldc", "adc", "ldl", "stl", "ldnl", "stnl", "add", "sub", "shl", "shr", "adj",
                        "a2sp", "sp2a", "call", "return", "brz", "brlz", "br", "halt", "mul", "div"};
const int kHasOperand[] = {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0};
const int kOpcodeCount = 21;

class Jarvis {
public:
    Jarvis(size_t memWords, unsigned long long limit)
        : memory(memWords, 0), A(0), B(0), PC(0), SP((int32_t)memWords), loadedWordCount(0),
          executedInstructionCount(0), instructionLimit(limit) {}
    bool load(const char *path);
    int run(bool trace);
    void dump(bool all) const;
    void disassemble() const;

private:
    std::vector<int32_t> memory;
    int32_t A, B, PC, SP;
    size_t loadedWordCount;
    unsigned long long executedInstructionCount, instructionLimit;

    int fail(const char *msg) const;
    bool addrOk(long long address) const { return address >= 0 && address < (long long)memory.size(); }
    void printRegs() const;
};

bool Jarvis::load(const char *path) {
    const char *extension = strrchr(path, '.');
    if (extension && strcmp(extension, ".lst") == 0) {
        std::ifstream listingFile(path);
        if (!listingFile) { fprintf(stderr, "emu: cannot open '%s'\n", path); return false; }

        std::string line;
        size_t wordCount = 0;
        size_t lineNumber = 0;
        while (std::getline(listingFile, line)) {
            ++lineNumber;
            size_t first = line.find_first_not_of(" \t\r");
            if (first == std::string::npos) continue;
            size_t last = line.find_last_not_of(" \t\r");
            if (last - first + 1 != 32) {
                fprintf(stderr, "emu: expected a 32-bit binary word at %s:%lu\n", path,
                        (unsigned long)lineNumber);
                return false;
            }
            if (wordCount >= memory.size()) {
                fprintf(stderr, "emu: program too large for memory\n");
                return false;
            }

            uint32_t word = 0;
            for (size_t i = first; i <= last; ++i) {
                if (line[i] != '0' && line[i] != '1') {
                    fprintf(stderr, "emu: invalid binary digit at %s:%lu\n", path,
                            (unsigned long)lineNumber);
                    return false;
                }
                word = (word << 1) | (uint32_t)(line[i] - '0');
            }
            memory[wordCount++] = (int32_t)word;
        }
        if (listingFile.bad()) {
            fprintf(stderr, "emu: error reading '%s'\n", path);
            return false;
        }
        if (wordCount == 0) {
            fprintf(stderr, "emu: listing file contains no machine code\n");
            return false;
        }
        loadedWordCount = wordCount;
        return true;
    }

    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "emu: cannot open '%s'\n", path); return false; }
    unsigned char b[4];
    size_t n = 0;
    size_t got;
    while ((got = fread(b, 1, 4, f)) == 4) {
        if (n >= memory.size()) { fprintf(stderr, "emu: program too large for memory\n"); fclose(f); return false; }
        memory[n++] = (int32_t)((uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24));
    }
    fclose(f);
    if (got != 0) { fprintf(stderr, "emu: object file size is not a multiple of 4 bytes\n"); return false; }
    loadedWordCount = n;
    return true;
}

void Jarvis::printRegs() const {
    printf("Registers: A=%d (0x%08X)  B=%d (0x%08X)  PC=%d  SP=%d\n", A, (unsigned)A, B, (unsigned)B, PC, SP);
}

int Jarvis::fail(const char *msg) const {
    printf("RUNTIME ERROR: %s\n", msg);
    printf("Instructions executed: %llu\n", executedInstructionCount);
    printRegs();
    return 2;
}

int Jarvis::run(bool trace) {
    for (;;) {
        if (PC < 0 || (size_t)PC >= memory.size()) return fail("PC out of range (errant program)");
        uint32_t w = (uint32_t)memory[(size_t)PC];
        int op = (int)(w & 0xFF);
        int32_t arg = (int32_t)(w & 0xFFFFFF00u) / 256;
        if (op >= kOpcodeCount) {
            char m[64];
            snprintf(m, sizeof m, "unknown opcode %d at PC=%d", op, PC);
            return fail(m);
        }
        if (executedInstructionCount >= instructionLimit) return fail("instruction limit exceeded (probable infinite loop)");
        if (trace) {
            if (kHasOperand[op]) printf("%08X: %-6s %d", (unsigned)PC, kNames[op], arg);
            else printf("%08X: %-6s", (unsigned)PC, kNames[op]);
            printf("    [A=%d B=%d SP=%d]\n", A, B, SP);
        }
        ++executedInstructionCount;
        ++PC;
        long long ea;
        switch (op) {
        case 0: B = A; A = arg; break;
        case 1: A = (int32_t)((uint32_t)A + (uint32_t)arg); break;
        case 2: ea = (long long)SP + arg;
            if (!addrOk(ea)) return fail("ldl address out of range");
            B = A; A = memory[(size_t)ea]; break;
        case 3: ea = (long long)SP + arg;
            if (!addrOk(ea)) return fail("stl address out of range");
            memory[(size_t)ea] = A; A = B; break;
        case 4: ea = (long long)A + arg;
            if (!addrOk(ea)) return fail("ldnl address out of range");
            A = memory[(size_t)ea]; break;
        case 5: ea = (long long)A + arg;
            if (!addrOk(ea)) return fail("stnl address out of range");
            memory[(size_t)ea] = B; break;
        case 6: A = (int32_t)((uint32_t)B + (uint32_t)A); break;
        case 7: A = (int32_t)((uint32_t)B - (uint32_t)A); break;
        case 8:
            A = (A < 0 || A > 31) ? 0 : (int32_t)((uint32_t)B << A); break;
        case 9:
            if (A < 0 || A > 31) A = (B < 0) ? -1 : 0;
            else A = (B >= 0) ? (int32_t)((uint32_t)B >> A) : (int32_t)~(~(uint32_t)B >> A);
            break;
        case 10: SP = (int32_t)((uint32_t)SP + (uint32_t)arg); break;
        case 11: SP = A; A = B; break;
        case 12: B = A; A = SP; break;
        case 13: B = A; A = PC; PC = PC + arg; break;
        case 14: PC = A; A = B; break;
        case 15: if (A == 0) PC = PC + arg; break;
        case 16: if (A < 0) PC = PC + arg; break;
        case 17: PC = PC + arg; break;
        case 18:
            printf("HALT reached after %llu instruction(s)\n", executedInstructionCount);
            printRegs();
            return 0;
        case 19: A = (int32_t)((uint32_t)B * (uint32_t)A); break;
        case 20:
            if (A == 0) return fail("division by zero");
            if (A == -1) A = (int32_t)(0u - (uint32_t)B);
            else A = B / A;
            break;
        }
    }
}

void Jarvis::dump(bool all) const {
    size_t i, shown = 0;
    printf("Memory dump (address: words)\n");
    for (i = 0; i < loadedWordCount; i += 4) {
        printf("%08X:", (unsigned)i);
        for (size_t k = i; k < i + 4 && k < loadedWordCount; ++k) printf(" %08X", (unsigned)memory[k]);
        printf("\n");
    }
    if (all) {
        for (i = loadedWordCount; i < memory.size(); ++i)
            if (memory[i] != 0) {
                if (!shown) printf("Non-zero words outside the loaded image:\n");
                printf("%08X: %08X (%d)\n", (unsigned)i, (unsigned)memory[i], memory[i]);
                ++shown;
            }
    }
}

void Jarvis::disassemble() const {
    for (size_t i = 0; i < loadedWordCount; ++i) {
        uint32_t w = (uint32_t)memory[i];
        int op = (int)(w & 0xFF);
        int32_t arg = (int32_t)(w & 0xFFFFFF00u) / 256;
        printf("%08X  %08X  ", (unsigned)i, (unsigned)w);
        if (op >= kOpcodeCount) printf("data 0x%X\n", (unsigned)w);
        else if (kHasOperand[op]) printf("%s %d\n", kNames[op], arg);
        else printf("%s\n", kNames[op]);
    }
}

void printIsa() {
    printf("IITPSx instruction set (opcode: mnemonic, operand)\n");
    for (int i = 0; i < kOpcodeCount; ++i)
        printf("  %2d: %-6s %s\n", i, kNames[i], kHasOperand[i] ? "value/offset" : "-");
    printf("Pseudo ops (assembler only): data <value>, SET <value>\n");
}

}

int main(int argc, char **argv) {
    bool trace = false, before = false, after = false, disasm = false;
    size_t memWords = 65536;
    unsigned long long limit = 10000000ULL;
    const char *file = 0;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-trace")) trace = true;
        else if (!strcmp(argv[i], "-before")) before = true;
        else if (!strcmp(argv[i], "-dump")) after = true;
        else if (!strcmp(argv[i], "-disasm")) disasm = true;
        else if (!strcmp(argv[i], "-isa")) { printIsa(); return 0; }
        else if (!strcmp(argv[i], "-version")) { printf("IITPSx emulator 1.0.0\n"); return 0; }
        else if (!strcmp(argv[i], "-mem") && i + 1 < argc) memWords = (size_t)strtoul(argv[++i], 0, 0);
        else if (!strcmp(argv[i], "-limit") && i + 1 < argc) limit = strtoull(argv[++i], 0, 0);
        else if (argv[i][0] != '-') file = argv[i];
        else { fprintf(stderr, "emu: unknown option %s\n", argv[i]); return 1; }
    }
    if (!file || memWords == 0) {
        fprintf(stderr, "usage: %s [-trace] [-before] [-dump] [-disasm] [-isa] [-mem N] [-limit N] prog.o|prog.lst\n",
                argv[0]);
        return 1;
    }
    Jarvis emulator(memWords, limit);
    if (!emulator.load(file)) return 1;
    if (disasm) { emulator.disassemble(); return 0; }
    if (before) emulator.dump(false);
    int rc = emulator.run(trace);
    if (after) emulator.dump(true);
    return rc;
}
