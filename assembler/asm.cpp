/*
 * Name    : Parth Tomar
 * Roll No : 2501CS09
 *
 * Declaration:
 * I, Parth Tomar (2501CS09), declare that this is my own work
 * for the CS2102 IITPSx mini project.
 *
 * asm.cpp - Two-pass assembler for the IITPSx processor.
 *
 * Usage: asm prog.asm  ->  generates prog.o and prog.lst
 *
 * Design:
 * - Pass 1: Finds labels and assigns their values.
 * - Pass 2: Generates machine code and reports errors.
 * - Instructions are stored in a table with their opcodes.
 * - Object code is stored as 32-bit little-endian words.
 * - Bits 31-8 store the operand; bits 7-0 store the opcode.
 * - Data words store their values directly.
 * - Supports SET, mul, div, and advanced listing.
 */
#include <cerrno>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace {

/* How the operand of a mnemonic is treated. */
enum OperandKind {
    OP_NONE,   /* no operand                                   */
    OP_VALUE,  /* operand used directly (label value = address) */
    OP_OFFSET, /* PC relative: label -> label - (PC + 1)        */
    OP_DATA,   /* 'data' pseudo op: a whole 32-bit word         */
    OP_SET     /* 'SET' pseudo op: gives the label a value      */
};

struct Def {
    const char *mnemonic;
    int operationCode;
    OperandKind operandKind;
};

//  STACK : A ----> B

/* Table of instruction names and expected operands (names are lower case). */
const Def kInstrTable[] = {
    {"data", -1, OP_DATA},  {"ldc", 0, OP_VALUE},   {"adc", 1, OP_VALUE},
    {"ldl", 2, OP_VALUE},   {"stl", 3, OP_VALUE},   {"ldnl", 4, OP_VALUE},
    {"stnl", 5, OP_VALUE},  {"add", 6, OP_NONE},    {"sub", 7, OP_NONE},
    {"shl", 8, OP_NONE},    {"shr", 9, OP_NONE},    {"adj", 10, OP_VALUE},
    {"a2sp", 11, OP_NONE},  {"sp2a", 12, OP_NONE},  {"call", 13, OP_OFFSET},
    {"return", 14, OP_NONE},{"brz", 15, OP_OFFSET}, {"brlz", 16, OP_OFFSET},
    {"br", 17, OP_OFFSET},  {"halt", 18, OP_NONE},  {"mul", 19, OP_NONE},
    {"div", 20, OP_NONE},   {"set", -1, OP_SET}};

const int kOpcodeHalt = 18;
const long long kOperandMin = -0x800000LL; /* -2^23 = -8388608 */
const long long kOperandMax = 0x7FFFFFLL;  /* 2^23 - 1 = 8388607 (signed 24-bit) */

struct Ref {      // helps to refer by name
    std::string labelName;
    long long labelValue;
    bool isSetDirective;
    bool referenced;
    int sourceLine;
};

struct Ref_List {// record for the listing file
    uint32_t address;
    int entryKind; /* 0 = label, 1 = instruction word, 2 = SET line */
    uint32_t machineWord;
    std::string listingText;
};

std::string trim(const std::string &s) {
    size_t b = 0, e = s.size();
    while (b < e && isspace((unsigned char)s[b])) ++b;
    while (e > b && isspace((unsigned char)s[e - 1])) --e;
    return s.substr(b, e - b);
}

std::string toLower(std::string s) {
    for (size_t i = 0; i < s.size(); ++i) s[i] = (char)tolower((unsigned char)s[i]);
    return s;
}

std::vector<std::string> splitWords(const std::string &s) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && isspace((unsigned char)s[i])) ++i;
        size_t j = i;
        while (j < s.size() && !isspace((unsigned char)s[j])) ++j;
        if (j > i) out.push_back(s.substr(i, j - i));
        i = j;
    }
    return out;
}

/* A valid label: alphanumeric string beginning with a letter. */
bool isValidLabel(const std::string &s) {
    if (s.empty() || !isalpha((unsigned char)s[0])) return false;
    for (size_t i = 1; i < s.size(); ++i)
        if (!isalnum((unsigned char)s[i])) return false;
    return true;
}

const Def *findInstr(const std::string &mnemonic) {
    size_t n = sizeof(kInstrTable) / sizeof(kInstrTable[0]);
    for (size_t i = 0; i < n; ++i)
        if (mnemonic == kInstrTable[i].mnemonic) return &kInstrTable[i];
    return 0;
}

std::string numToString(long long v) {
    char buf[32];
    snprintf(buf, sizeof buf, "%lld", v);
    return buf;
}

class Builder {
public:
    explicit Builder(const std::string &src) : sourcePath(src), programCounter(0), errorCount(0), warningCount(0) {}
    bool assemble();
    bool writeFiles(const std::string &objPath, const std::string &lstPath) const;
    int errors() const { return errorCount; }
    int warnings() const { return warningCount; }
    size_t words() const { return machineCode.size(); }

private:
    std::string sourcePath;
    long long programCounter;
    int errorCount, warningCount;
    std::vector<Ref> labels;
    std::map<std::string, size_t> labelLookup;
    std::vector<uint32_t> machineCode;
    std::vector<Ref_List> listingEntries;
    bool hasHaltInstruction = false;

    void pass(int passNo);
    void processLine(int passNo, int lineNo, const std::string &line);
    bool evalOperand(int passNo, int lineNo, const Def &def, const std::string &tok,
                     long long &value, std::string &shown);
    void report(int passNo, int lineNo, bool pass2Only, const std::string &msg);
    const Ref *labelAtAddress(long long address) const;
};

/* Errors are reported once: syntax errors in pass 1, unresolved labels in pass 2. */
void Builder::report(int passNo, int lineNo, bool pass2Only, const std::string &msg) {
    if ((pass2Only && passNo == 2) || (!pass2Only && passNo == 1)) {
        ++errorCount;
        fprintf(stderr, "ERROR: %s:%d: %s\n", sourcePath.c_str(), lineNo, msg.c_str());
    }
}

const Ref *Builder::labelAtAddress(long long address) const {
    for (size_t i = 0; i < labels.size(); ++i)
        if (!labels[i].isSetDirective && labels[i].labelValue == address) return &labels[i];
    return 0;
}

/* Convert an operand token to a value. Returns false if an error was reported. */
bool Builder::evalOperand(int passNo, int lineNo, const Def &def, const std::string &tok,
                            long long &value, std::string &shown) {
    value = 0;
    shown = tok;
    char c = tok[0];
    if (isdigit((unsigned char)c) || c == '+' || c == '-') {
        char *end = 0;
        errno = 0;
        long long v = strtoll(tok.c_str(), &end, 0); /* decimal, 0x hex, 0 octal */
        if (*end != '\0' || end == tok.c_str() || errno == ERANGE) {
            report(passNo, lineNo, false, "invalid number '" + tok + "'");
            return false;
        }
        value = v;
        shown = numToString(v);
    } else if (isValidLabel(tok)) {
        if (def.operandKind == OP_SET) {
            report(passNo, lineNo, false, "SET needs a numeric value, not a label");
            return false;
        }
        if (passNo == 1) return true; /* value unknown yet */
        std::map<std::string, size_t>::iterator it = labelLookup.find(tok);
        if (it == labelLookup.end()) {
            report(passNo, lineNo, true, "no such label '" + tok + "'");
            return false;
        }
        labels[it->second].referenced = true;
        value = labels[it->second].labelValue;
        if (def.operandKind == OP_OFFSET) value = value - (programCounter + 1);
        shown = tok;
    } else {
        report(passNo, lineNo, false, "invalid operand '" + tok + "'");
        return false;
    }
    /* range checks (labels are only checked in pass 2) */
    bool known = (passNo == 2) || isdigit((unsigned char)c) || c == '+' || c == '-';
    if (known) {
        long long lo = kOperandMin, hi = kOperandMax;
        if (def.operandKind == OP_DATA) { lo = -2147483648LL; hi = 4294967295LL; }
        if (def.operandKind == OP_SET) { lo = -2147483648LL; hi = 4294967295LL; }
        if (value < lo || value > hi) {
            report(passNo, lineNo, passNo == 2 && !(isdigit((unsigned char)c) || c == '+' || c == '-'),
                   "operand out of range: " + numToString(value));
            return false;
        }
    }
    return true;
}

void Builder::processLine(int passNo, int lineNo, const std::string &line) {
    std::string text = line;
    size_t sc = text.find(';');
    if (sc != std::string::npos) text.erase(sc); /* strip comment */
    text = trim(text);
    if (text.empty()) return;

    /* optional "label:" prefix */
    std::string labelName;
    bool haveLabel = false, labelOk = false;
    size_t colon = text.find(':');
    if (colon != std::string::npos) {
        labelName = trim(text.substr(0, colon));
        text = trim(text.substr(colon + 1));
        haveLabel = true;
        labelOk = isValidLabel(labelName);
        if (!labelOk) report(passNo, lineNo, false, "bogus label name '" + labelName + "'");
    }

    std::vector<std::string> tok = splitWords(text);
    long long labelValue = programCounter;
    bool labelIsSet = false;
    bool emitWord = false;
    uint32_t word = 0;
    std::string mnemonicText, operandShown;
    const Def *def = 0;

    if (!tok.empty()) {
        def = findInstr(toLower(tok[0]));
        if (!def) {
            if (haveLabel && tok[0].find(':') != std::string::npos)
                report(passNo, lineNo, false, "only one label is allowed per line");
            else
                report(passNo, lineNo, false, "unknown mnemonic '" + tok[0] + "'");
        } else {
            size_t nargs = tok.size() - 1;
            bool ok = true;
            long long value = 0;
            std::string shown;
            mnemonicText = def->mnemonic;
            if (nargs > 1) {
                report(passNo, lineNo, false, "extra text at end of line");
                ok = false;
            } else if (def->operandKind == OP_NONE) {
                if (nargs == 1) {
                    report(passNo, lineNo, false,
                           std::string("unexpected operand for '") + def->mnemonic + "'");
                    ok = false;
                }
            } else if (nargs == 0) {
                report(passNo, lineNo, false, std::string("missing operand for '") + def->mnemonic + "'");
                ok = false;
            } else {
                ok = evalOperand(passNo, lineNo, *def, tok[1], value, shown);
            }

            if (def->operandKind == OP_SET) {
                if (!haveLabel) report(passNo, lineNo, false, "SET requires a label on the same line");
                if (ok) { labelValue = value; labelIsSet = true; }
                else labelIsSet = true; /* keep label out of the PC address space */
                if (passNo == 2) {
                    Ref_List entry = {0, 2, 0, labelName + ": SET " + (ok ? shown : tok.size() > 1 ? tok[1] : "?")};
                    listingEntries.push_back(entry);
                }
                haveLabel = haveLabel && labelOk; /* handled below */
            } else {
                emitWord = true;
                if (def->operandKind == OP_DATA) {
                    word = (uint32_t)(value & 0xFFFFFFFFLL);
                } else {
                    word = (((uint32_t)value & 0xFFFFFFu) << 8) | (uint32_t)def->operationCode;
                    if (def->operationCode == kOpcodeHalt) hasHaltInstruction = true;
                }
                /* numeric branch offset: show the label at the target, if there is one */
                if (def->operandKind == OP_OFFSET && ok && passNo == 2 && !isValidLabel(tok[1])) {
                    const Ref *targetLabel = labelAtAddress(programCounter + 1 + value);
                    shown = targetLabel ? targetLabel->labelName : numToString(value);
                }
                operandShown = ok ? shown : "";
            }
        }
    }

    /* label definition (pass 1) */
    if (haveLabel && labelOk) {
        if (passNo == 1) {
            if (labelLookup.count(labelName)) {
                report(passNo, lineNo, false, "duplicate label '" + labelName + "'");
            } else {
                Ref label = {labelName, labelValue, labelIsSet, false, lineNo};
                labelLookup[labelName] = labels.size();
                labels.push_back(label);
            }
        } else if (!labelIsSet) {
            Ref_List entry = {(uint32_t)programCounter, 0, 0, labelName + ":"};
            listingEntries.push_back(entry);
        }
    }

    if (emitWord) {
        if (passNo == 2) {
            machineCode.push_back(word);
            Ref_List entry = {(uint32_t)programCounter, 1, word, mnemonicText + (operandShown.empty() ? "" : " " + operandShown)};
            listingEntries.push_back(entry);
        }
        ++programCounter;
    }
}

void Builder::pass(int passNo) {
    std::ifstream in(sourcePath.c_str());
    std::string line;
    int lineNo = 0;
    programCounter = 0;
    if (passNo == 2) { machineCode.clear(); listingEntries.clear(); }
    while (std::getline(in, line)) processLine(passNo, ++lineNo, line);
}

bool Builder::assemble() {
    std::ifstream probe(sourcePath.c_str());
    if (!probe) {
        fprintf(stderr, "ERROR: cannot open '%s'\n", sourcePath.c_str());
        return false;
    }
    probe.close();
    pass(1);
    pass(2);
    for (size_t i = 0; i < labels.size(); ++i)
        if (!labels[i].referenced) {
            ++warningCount;
            fprintf(stderr, "WARNING: %s:%d: label '%s' defined but never used\n", sourcePath.c_str(),
                    labels[i].sourceLine, labels[i].labelName.c_str());
        }
    if (errorCount == 0 && !hasHaltInstruction) {
        ++warningCount;
        fprintf(stderr, "WARNING: %s: program contains no HALT instruction\n", sourcePath.c_str());
    }
    return errorCount == 0;
}

bool Builder::writeFiles(const std::string &objPath, const std::string &lstPath) const {
    FILE *o = fopen(objPath.c_str(), "wb");
    if (!o) { fprintf(stderr, "ERROR: cannot write '%s'\n", objPath.c_str()); return false; }
    for (size_t i = 0; i < machineCode.size(); ++i) {
        unsigned char b[4] = {(unsigned char)(machineCode[i] & 0xFF), (unsigned char)((machineCode[i] >> 8) & 0xFF),
                              (unsigned char)((machineCode[i] >> 16) & 0xFF), (unsigned char)((machineCode[i] >> 24) & 0xFF)};
        fwrite(b, 1, 4, o);
    }
    fclose(o);

    FILE *l = fopen(lstPath.c_str(), "w");
    if (!l) { fprintf(stderr, "ERROR: cannot write '%s'\n", lstPath.c_str()); return false; }
    for (size_t i = 0; i < listingEntries.size(); ++i) {
        const Ref_List &entry = listingEntries[i];
        if (entry.entryKind == 1) {
            for (int bit = 31; bit >= 0; --bit)
                fputc((entry.machineWord & (1u << bit)) ? '1' : '0', l);
            fputc('\n', l);
        }
    }
    fclose(l);
    return true;
}

} /* namespace */

int main(int argc, char **argv) {
    if (argc == 2 && std::string(argv[1]) == "-version") {
        printf("IITPSx assembler 1.0.0\n");
        return 0;
    }
    if (argc != 2) {
        fprintf(stderr, "usage: %s program.asm   (or -version)\n", argv[0]);
        return 1;
    }
    std::string src = argv[1];
    std::string base = src;
    if (base.size() > 4 && base.compare(base.size() - 4, 4, ".asm") == 0) base.erase(base.size() - 4);

    Builder as(src);
    bool ok = as.assemble();
    if (!ok) {
        fprintf(stderr, "Assembly FAILED: %d error(s), %d warning(s). No output files written.\n",
                as.errors(), as.warnings());
        return 1;
    }
    if (!as.writeFiles(base + ".o", base + ".lst")) return 1;
    printf("Assembly OK: %lu word(s), %d warning(s). Wrote %s.o and %s.lst\n", (unsigned long)as.words(),
           as.warnings(), base.c_str(), base.c_str());
    return 0;
}
