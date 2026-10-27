#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class Simulator8085 {
private:
    unordered_map<string, unsigned char> registers;
    bool zeroFlag = false;
    bool signFlag = false;
    bool carryFlag = false;
    bool auxiliaryCarryFlag = false;
    bool parityFlag = false;
    bool halted = false;

    static string trim(string text) {
        size_t first = text.find_first_not_of(" \t\r\n");
        if (first == string::npos) return "";
        size_t last = text.find_last_not_of(" \t\r\n");
        return text.substr(first, last - first + 1);
    }

    static string upper(string text) {
        transform(text.begin(), text.end(), text.begin(),
                  [](unsigned char c) { return static_cast<char>(toupper(c)); });
        return text;
    }

    static vector<string> splitOperands(const string& text) {
        vector<string> result;
        string token;
        stringstream stream(text);
        while (getline(stream, token, ',')) {
            result.push_back(upper(trim(token)));
        }
        return result;
    }

    static bool isRegister(const string& name) {
        return name == "A" || name == "B" || name == "C" ||
               name == "D" || name == "E" || name == "H" ||
               name == "L";
    }

    static bool parseByte(string text, unsigned char& value) {
        text = upper(trim(text));
        if (!text.empty() && text.back() == 'H') {
            text.pop_back();
            if (text.empty()) return false;
            unsigned int parsed = 0;
            stringstream ss;
            ss << hex << text;
            ss >> parsed;
            if (ss.fail() || !ss.eof() || parsed > 0xFF) return false;
            value = static_cast<unsigned char>(parsed);
            return true;
        }

        try {
            size_t used = 0;
            unsigned long parsed = stoul(text, &used, 0);
            if (used != text.size() || parsed > 0xFF) return false;
            value = static_cast<unsigned char>(parsed);
            return true;
        } catch (...) {
            return false;
        }
    }

    static bool parseWord(string text, unsigned short& value) {
        text = upper(trim(text));
        if (!text.empty() && text.back() == 'H') {
            text.pop_back();
            if (text.empty()) return false;
            unsigned int parsed = 0;
            stringstream ss;
            ss << hex << text;
            ss >> parsed;
            if (ss.fail() || !ss.eof() || parsed > 0xFFFF) return false;
            value = static_cast<unsigned short>(parsed);
            return true;
        }

        try {
            size_t used = 0;
            unsigned long parsed = stoul(text, &used, 0);
            if (used != text.size() || parsed > 0xFFFF) return false;
            value = static_cast<unsigned short>(parsed);
            return true;
        } catch (...) {
            return false;
        }
    }

    void updateSZP(unsigned char value) {
        zeroFlag = (value == 0);
        signFlag = ((value & 0x80) != 0);
        int ones = 0;
        for (int bit = 0; bit < 8; ++bit) {
            if (value & (1 << bit)) ++ones;
        }
        parityFlag = (ones % 2 == 0);
    }

    static string hexByte(unsigned char value) {
        stringstream ss;
        ss << uppercase << hex << setw(2) << setfill('0')
           << static_cast<unsigned int>(value) << "H";
        return ss.str();
    }

    void showFlags() const {
        cout << "Flags: Z=" << zeroFlag
             << " S=" << signFlag
             << " P=" << parityFlag
             << " CY=" << carryFlag
             << " AC=" << auxiliaryCarryFlag << '\n';
    }

    void showRegisters() const {
        cout << "\n--- Register values ---\n";
        for (const string& name : {"A", "B", "C", "D", "E", "H", "L"}) {
            cout << name << " = " << hexByte(registers.at(name)) << '\n';
        }
        showFlags();
        cout << "-----------------------\n";
    }

    bool getPair(const string& pairName, unsigned short& value) const {
        if (pairName == "BC") {
            value = (static_cast<unsigned short>(registers.at("B")) << 8) |
                    registers.at("C");
        } else if (pairName == "DE") {
            value = (static_cast<unsigned short>(registers.at("D")) << 8) |
                    registers.at("E");
        } else if (pairName == "HL") {
            value = (static_cast<unsigned short>(registers.at("H")) << 8) |
                    registers.at("L");
        } else {
            return false;
        }
        return true;
    }

    void setPair(const string& pairName, unsigned short value) {
        if (pairName == "BC") {
            registers["B"] = static_cast<unsigned char>((value >> 8) & 0xFF);
            registers["C"] = static_cast<unsigned char>(value & 0xFF);
        } else if (pairName == "DE") {
            registers["D"] = static_cast<unsigned char>((value >> 8) & 0xFF);
            registers["E"] = static_cast<unsigned char>(value & 0xFF);
        } else if (pairName == "HL") {
            registers["H"] = static_cast<unsigned char>((value >> 8) & 0xFF);
            registers["L"] = static_cast<unsigned char>(value & 0xFF);
        }
    }

    bool executeLine(string line) {
        // Remove comments beginning with ';'.
        size_t comment = line.find(';');
        if (comment != string::npos) line = line.substr(0, comment);
        line = trim(line);
        if (line.empty()) return true;

        // Accept a simple label prefix such as "START: MVI A, 05H".
        size_t colon = line.find(':');
        if (colon != string::npos) {
            line = trim(line.substr(colon + 1));
            if (line.empty()) return true;
        }

        size_t space = line.find_first_of(" \t");
        string opcode = upper(space == string::npos ? line : line.substr(0, space));
        string operandText = (space == string::npos) ? "" : trim(line.substr(space + 1));
        vector<string> operands = splitOperands(operandText);

        auto requireCount = [&](size_t count) {
            if (operands.size() != count) {
                cout << "Error: " << opcode << " expects " << count
                     << " operand(s).\n";
                return false;
            }
            return true;
        };

        if (opcode == "HLT") {
            if (!operandText.empty()) {
                cout << "Error: HLT takes no operands.\n";
                return false;
            }
            halted = true;
            cout << "Execution halted.\n";
            return true;
        }

        if (opcode == "MOV") {
            if (!requireCount(2)) return false;
            if (!isRegister(operands[0]) || !isRegister(operands[1])) {
                cout << "Error: MOV supports register-to-register operands only.\n";
                return false;
            }
            registers[operands[0]] = registers[operands[1]];
            return true;
        }

        if (opcode == "MVI") {
            if (!requireCount(2)) return false;
            if (!isRegister(operands[0])) {
                cout << "Error: MVI destination must be A, B, C, D, E, H, or L.\n";
                return false;
            }
            unsigned char value;
            if (!parseByte(operands[1], value)) {
                cout << "Error: invalid 8-bit immediate value: " << operands[1] << '\n';
                return false;
            }
            registers[operands[0]] = value;
            return true;
        }

        if (opcode == "LXI") {
            if (!requireCount(2)) return false;
            unsigned short value;
            if (!parseWord(operands[1], value) ||
                !(operands[0] == "BC" || operands[0] == "DE" || operands[0] == "HL")) {
                cout << "Error: LXI supports BC, DE, or HL with a 16-bit value.\n";
                return false;
            }
            setPair(operands[0], value);
            return true;
        }

        if (opcode == "INX" || opcode == "DCX") {
            if (!requireCount(1)) return false;
            unsigned short value;
            if (!getPair(operands[0], value)) {
                cout << "Error: " << opcode << " supports BC, DE, or HL.\n";
                return false;
            }
            value = static_cast<unsigned short>(opcode == "INX" ? value + 1 : value - 1);
            setPair(operands[0], value);
            return true;
        }

        if (opcode == "ADD" || opcode == "SUB" || opcode == "CMP" ||
            opcode == "ANA" || opcode == "ORA" || opcode == "XRA") {
            if (!requireCount(1)) return false;
            if (!isRegister(operands[0])) {
                cout << "Error: " << opcode << " expects a register operand.\n";
                return false;
            }

            unsigned char a = registers["A"];
            unsigned char value = registers[operands[0]];

            if (opcode == "ADD") {
                unsigned int result = static_cast<unsigned int>(a) + value;
                carryFlag = result > 0xFF;
                auxiliaryCarryFlag = ((a & 0x0F) + (value & 0x0F)) > 0x0F;
                registers["A"] = static_cast<unsigned char>(result & 0xFF);
                updateSZP(registers["A"]);
            } else if (opcode == "SUB" || opcode == "CMP") {
                int result = static_cast<int>(a) - static_cast<int>(value);
                carryFlag = a < value;
                auxiliaryCarryFlag = (a & 0x0F) < (value & 0x0F);
                unsigned char resultByte = static_cast<unsigned char>(result & 0xFF);
                updateSZP(resultByte);
                if (opcode == "SUB") registers["A"] = resultByte;
            } else {
                unsigned char result = 0;
                if (opcode == "ANA") result = static_cast<unsigned char>(a & value);
                if (opcode == "ORA") result = static_cast<unsigned char>(a | value);
                if (opcode == "XRA") result = static_cast<unsigned char>(a ^ value);
                registers["A"] = result;
                carryFlag = false;
                auxiliaryCarryFlag = (opcode == "ANA");
                updateSZP(result);
            }
            return true;
        }

        if (opcode == "INR" || opcode == "DCR") {
            if (!requireCount(1)) return false;
            if (!isRegister(operands[0])) {
                cout << "Error: " << opcode << " expects a register operand.\n";
                return false;
            }
            unsigned char oldValue = registers[operands[0]];
            unsigned char newValue = static_cast<unsigned char>(
                opcode == "INR" ? oldValue + 1 : oldValue - 1);
            registers[operands[0]] = newValue;
            updateSZP(newValue);
            auxiliaryCarryFlag = (opcode == "INR")
                ? ((oldValue & 0x0F) == 0x0F)
                : ((oldValue & 0x0F) == 0x00);
            // On the 8085, INR and DCR do not modify CY.
            return true;
        }

        if (opcode == "ADI" || opcode == "SUI" || opcode == "ANI" ||
            opcode == "ORI" || opcode == "XRI" || opcode == "CPI") {
            if (!requireCount(1)) return false;
            unsigned char value;
            if (!parseByte(operands[0], value)) {
                cout << "Error: invalid 8-bit immediate value: " << operands[0] << '\n';
                return false;
            }
            unsigned char a = registers["A"];
            if (opcode == "ADI") {
                unsigned int result = static_cast<unsigned int>(a) + value;
                carryFlag = result > 0xFF;
                auxiliaryCarryFlag = ((a & 0x0F) + (value & 0x0F)) > 0x0F;
                registers["A"] = static_cast<unsigned char>(result & 0xFF);
                updateSZP(registers["A"]);
            } else if (opcode == "SUI" || opcode == "CPI") {
                int result = static_cast<int>(a) - static_cast<int>(value);
                carryFlag = a < value;
                auxiliaryCarryFlag = (a & 0x0F) < (value & 0x0F);
                unsigned char resultByte = static_cast<unsigned char>(result & 0xFF);
                updateSZP(resultByte);
                if (opcode == "SUI") registers["A"] = resultByte;
            } else {
                unsigned char result = 0;
                if (opcode == "ANI") result = static_cast<unsigned char>(a & value);
                if (opcode == "ORI") result = static_cast<unsigned char>(a | value);
                if (opcode == "XRI") result = static_cast<unsigned char>(a ^ value);
                registers["A"] = result;
                carryFlag = false;
                auxiliaryCarryFlag = (opcode == "ANI");
                updateSZP(result);
            }
            return true;
        }

        cout << "Error: unsupported instruction: " << opcode << '\n';
        return false;
    }

public:
    Simulator8085() {
        for (const string& name : {"A", "B", "C", "D", "E", "H", "L"}) {
            registers[name] = 0;
        }
    }

    void run() {
        cout << "============================================\n";
        cout << "  8085 Instruction Simulator (Educational)\n";
        cout << "============================================\n";
        cout << "Enter one instruction per line. Type HLT to stop.\n";
        cout << "Hex values can be written as 05H or 0x05.\n";
        cout << "Type SHOW to display registers and flags.\n\n";

        string line;
        while (!halted) {
            cout << "8085> ";
            if (!getline(cin, line)) break;

            if (upper(trim(line)) == "SHOW") {
                showRegisters();
                continue;
            }

            executeLine(line);
            if (!halted) showRegisters();
        }
        cout << "Program finished.\n";
    }
};

int main() {
    Simulator8085 simulator;
    simulator.run();
    return 0;
}
