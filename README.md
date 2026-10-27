# 8085 Microprocessor Instruction Simulator

A command-line educational simulator written in C++ that models a selected subset of Intel 8085 assembly instructions. It is designed to help beginners observe register and flag changes while experimenting with instructions.

## Features

- Models the 8-bit registers A, B, C, D, E, H, and L.
- Models the Zero (Z), Sign (S), Parity (P), Carry (CY), and Auxiliary Carry (AC) flags.
- Supports register-pair operations for BC, DE, and HL.
- Parses immediate values in forms such as `05H`, `0FFH`, and `0x05`.
- Accepts one instruction per line and prints the current register/flag state after each instruction.
- Supports comments beginning with `;` and simple label prefixes.

## Supported instruction subset

- Data transfer: `MOV`, `MVI`, `LXI`
- Arithmetic: `ADD`, `ADI`, `SUB`, `SUI`, `INR`, `DCR`, `INX`, `DCX`
- Logical/compare: `ANA`, `ANI`, `ORA`, `ORI`, `XRA`, `XRI`, `CMP`, `CPI`
- Control: `HLT`
- Simulator command: `SHOW` (not an 8085 machine instruction)

**Limitations:** This is not a complete 8085 emulator. It does not currently model full 64 KB memory, program counter, stack pointer, interrupts, timing/cycles, ports, or all instructions. `MOV` is register-to-register only. Flag behavior is educational and should be tested against a trusted 8085 reference before use for hardware-level work.

## Requirements

- A C++ compiler that supports C++11 or later (for example, GCC, Clang, or MinGW on Windows).

## Compile

### Windows (MinGW g++)

Open Command Prompt or PowerShell in this folder:

```bash
g++ -std=c++11 -Wall -Wextra -pedantic main.cpp -o simulator.exe
```

Run:

```bash
.\simulator.exe
```

### Linux / macOS

```bash
g++ -std=c++11 -Wall -Wextra -pedantic main.cpp -o simulator
./simulator
```

## Example session

```text
8085> MVI A, 05H
A = 05H
8085> MVI B, 03H
B = 03H
8085> ADD B
A = 08H
8085> SHOW
8085> HLT
```

The actual program prints all register and flag values after each instruction; the example above is abbreviated.

## Design overview

- `Simulator8085` stores register values and flags.
- The input parser separates an opcode from its operands and normalizes input to uppercase.
- Each supported opcode is validated before execution.
- Arithmetic/logical operations update the modeled registers and flags.
- The CLI prints the state after each instruction.

## Future improvements

- Add a 64 KB memory model and memory instructions such as `LDA`, `STA`, `LHLD`, and `SHLD`.
- Add a program counter, stack pointer, jumps, calls, and returns.
- Load and run a multi-line assembly program from a file.
- Add automated unit tests for instruction results and flag behavior.
- Add an instruction-step mode and execution trace.

## License

This project is provided for learning. Add a license file if you want others to reuse or modify it under explicit terms.
