#include <bits/stdc++.h>
#include <fstream>
using namespace std;

int ICache[256], DCache[256], RF[16];
bool ready[16];
int pc = 0;
int num_stall = 0, num_dat_st = 0, num_con_st = 0, num_inst = 0, num_cycles = 0;
int num_arith = 0, num_log = 0, num_con = 0, num_mem = 0, num_shift = 0,
    num_halt = 0, num_imm = 0;
int halt = 0, can_fetch = 1;

class Buffer {
public:
  int ins_part1, ins_part2;
  int a, b, imm, rd_to_exec, rd_to_mem, rd_to_writeback, ALUOutput, ALUOutput2,
      LMD;
  int opcode_to_decode, opcode_to_exec, opcode_to_mem, halt_data,
      opcode_to_writeback;
  Buffer()
      : ins_part1(-1), ins_part2(-1), a(-1), b(-1), imm(-1), rd_to_exec(-1),
        rd_to_mem(-1), rd_to_writeback(-1), // Initialize different registers
        ALUOutput(-1), ALUOutput2(-1), LMD(-1), opcode_to_decode(-1),
        opcode_to_exec(-1), opcode_to_mem(-1), halt_data(-1),
        opcode_to_writeback(-1) {}
};

void fetch(Buffer &B) {
  // cout << pc << "\n";
  B.ins_part1 = ICache[pc];
  B.ins_part2 = ICache[pc + 1];
  B.opcode_to_decode = B.ins_part1 >> 4;
  // cout << 1 << " " << B.opcode_to_decode << "%\n";
  if (B.opcode_to_decode >= 0 && B.opcode_to_decode <= 3) {
    num_arith++;
  } else if (B.opcode_to_decode >= 4 && B.opcode_to_decode <= 7) {
    num_log++;
  } else if (B.opcode_to_decode >= 8 && B.opcode_to_decode <= 9) {
    num_shift++;
  } else if (B.opcode_to_decode == 10) {
    num_imm++;
  } else if (B.opcode_to_decode >= 11 && B.opcode_to_decode <= 12) {
    num_mem++;
  } else if (B.opcode_to_decode >= 13 && B.opcode_to_decode <= 14) {
    num_con++;
  } else
    num_halt++;
  num_inst++;
  pc += 2;
}

void decode(Buffer &B) { // Decode Stage
  if (B.opcode_to_decode == -1)
    return;
  // cout << "2"
  //      << " " << B.opcode_to_decode << "%\n";
  int rs1, rs2;
  B.rd_to_exec = B.ins_part1 & 0x0F;
  // cout << B.rd_to_exec << "    %\n";
  rs1 = B.ins_part2 >> 4;
  rs2 = B.ins_part2 & 0x0F;
  if ((B.opcode_to_decode >= 0 && B.opcode_to_decode <= 2) ||
      (B.opcode_to_decode >= 4 && B.opcode_to_decode <= 6)) {
    if (ready[rs1] == false || ready[rs2] == false) {
      num_stall++;
      num_dat_st++;
      B.halt_data = 1;
      return;
    }
    B.halt_data = 0;
    B.a = RF[rs1];
    B.b = RF[rs2];
    ready[B.rd_to_exec] = false;
  } else if (B.opcode_to_decode == 3) {
    if (ready[B.rd_to_exec] == false) {
      num_stall++;
      num_dat_st++;
      B.halt_data = 1;
      return;
    }
    B.halt_data = 0;
    B.a = RF[B.rd_to_exec];
    ready[B.rd_to_exec] = false;
  } else if (B.opcode_to_decode == 7) {
    if (ready[rs1] == false) {
      num_stall++;
      num_dat_st++;
      B.halt_data = 1;
      return;
    }
    B.halt_data = 0;
    B.a = RF[rs1];
    ready[B.rd_to_exec] = false;
  } else if (B.opcode_to_decode == 8 || B.opcode_to_decode == 9 ||
             B.opcode_to_decode == 11 || B.opcode_to_decode == 12) {
    if (ready[rs1] == false) {
      num_stall++;
      num_dat_st++;
      B.halt_data = 1;
      return;
    }
    B.halt_data = 0;
    B.a = RF[rs1];
    B.imm = B.ins_part2 & 0xF;
    if ((B.imm & 0x8) == 0x8)
      B.imm = B.imm | 0xF0;
    ready[B.rd_to_exec] = false;

  } else if (B.opcode_to_decode == 10) {
    B.imm = B.ins_part2;
    if (B.imm > 127)
      B.imm = B.imm - 256;
    ready[B.rd_to_exec] = false;
  } else if (B.opcode_to_decode == 14) {
    if (ready[B.rd_to_exec] == false) {
      num_stall++;
      num_dat_st++;
      B.halt_data = 1;
      return;
    }
    B.halt_data = 0;
    B.a = RF[B.rd_to_exec];
    B.imm = B.ins_part2;
    if (B.imm > 127)
      B.imm = B.imm - 256;
  } else if (B.opcode_to_decode == 13) {
    B.imm = B.ins_part1 & 0xF;
    B.imm = B.imm << 4;
    int x = B.ins_part2 >> 4;
    B.imm = B.imm | x;
    if (B.imm > 127)
      B.imm = B.imm - 256;
  } else if (B.opcode_to_decode == 15) {
    halt = 1;
  }
  B.opcode_to_exec = B.opcode_to_decode;
  if (B.opcode_to_decode == 13 || B.opcode_to_decode == 14)
    B.opcode_to_decode = -1;
  // cout << B.halt_data << "%%\n";
}

// execute
void execute(Buffer &B) {
  if (B.opcode_to_exec == -1)
    return;

  B.rd_to_mem = B.rd_to_exec;
  // cout << "3"
  //      << " " << B.opcode_to_exec << "%\n";
  if (B.opcode_to_exec == 0) {
    B.ALUOutput = B.a + B.b;
  } else if (B.opcode_to_exec == 1) {
    B.ALUOutput = B.a - B.b;
  } else if (B.opcode_to_exec == 2) {
    B.ALUOutput = B.a * B.b;
  } else if (B.opcode_to_exec == 3) {
    B.ALUOutput = B.a + 1;
  } else if (B.opcode_to_exec == 4) {
    B.ALUOutput = B.a & B.b;
  } else if (B.opcode_to_exec == 5) {
    B.ALUOutput = B.a || B.b;
  } else if (B.opcode_to_exec == 6) {
    B.ALUOutput = B.a ^ B.b;
  } else if (B.opcode_to_exec == 7) {
    B.ALUOutput = !B.a;
  } else if (B.opcode_to_exec == 8) {
    B.ALUOutput = B.a << B.imm;
  } else if (B.opcode_to_exec == 9) {
    B.ALUOutput = B.a >> B.imm;
  } else if (B.opcode_to_exec == 10) {
    B.ALUOutput = B.imm;
  } else if (B.opcode_to_exec == 11 || B.opcode_to_exec == 12) {
    B.ALUOutput = B.a + B.imm;
  } else if (B.opcode_to_exec == 13) {
    pc = pc + 2 * B.imm;
  } else if (B.opcode_to_exec == 14) {
    if (B.a == 0)
      pc = pc + 2 * B.imm;
  }
  B.opcode_to_mem = B.opcode_to_exec;
}
void memaccess(Buffer &B) {
  if (B.opcode_to_mem == -1)
    return;
  if (B.halt_data == 1) {
    // cout << "here" << endl;
    B.opcode_to_exec = -1;
  }
  if (B.opcode_to_mem == 13 || B.opcode_to_mem == 14) {
    B.opcode_to_exec = -1;
    B.opcode_to_mem = -1;
    B.opcode_to_decode = -1;
  }
  // cout << "4"
  //      << " " << B.opcode_to_mem << "%\n";
  if (B.opcode_to_mem == 11) {
    B.LMD = DCache[B.ALUOutput];
    RF[B.rd_to_mem] = B.LMD;
  } else if (B.opcode_to_mem == 12) {
    B.LMD = RF[B.rd_to_mem];
    DCache[B.ALUOutput] = B.LMD;
  }
  B.opcode_to_writeback = B.opcode_to_mem;
  if (B.halt_data == 1) {
    // cout << "here" << endl;
    B.opcode_to_mem = -1;
  }
  B.rd_to_writeback = B.rd_to_mem;
  B.ALUOutput2 = B.ALUOutput;
}

// writeback
void writeback(Buffer &B) {
  if (B.opcode_to_writeback == -1)
    return;
  // cout << "5"
  //      << " " << B.opcode_to_writeback << "%\n";
  if (B.opcode_to_writeback >= 0 && B.opcode_to_writeback <= 11) {
    // cout<<opcode<<" "<<rd<<"  :\n";
    RF[B.rd_to_writeback] = B.ALUOutput2;
    ready[B.rd_to_writeback] = true;
  }

  if (B.opcode_to_writeback == 15)
    exit(0);
}

void print_nos(string filename) {
  ofstream output;
  output.open(filename);
  output << "Total number of instructions executed:" << num_inst << endl;
  output << "Number of instructions in each class" << endl;
  output << "Arithmetic instructions              :" << num_arith << endl;
  output << "Logical instructions                 :" << num_log << endl;
  output << "Shift instructions                 :" << num_shift << endl;
  output << "Memory instructions                 :" << num_mem << endl;
  output << "Load immediate instructions         :" << num_imm << endl;
  output << "Control instructions                 :" << num_con << endl;
  output << "Halt instructions                    :" << num_halt << endl;
  output << "Cycles Per Instruction               :"
         << 1.0 * num_cycles / num_inst << endl;
  output << "Total number of stalls               :" << num_stall << endl;
  output << "Data stalls (RAW)                    :" << num_dat_st << endl;
  output << "Control stalls                       :" << num_con_st << endl;
  output.close();
}
void read_input(int arr[], int size, string filename) { // Read Input
  ifstream input;
  input.open(filename);
  string ins;
  for (int i = 0; i < size; i++) {
    if (!input)
      break;
    getline(input, ins);
    arr[i] = stoi(ins, 0, 16);
  }
  input.close();
}
void print_output(int arr[], int size, string filename) { // Print Output
  ofstream output;
  output.open(filename);
  for (int i = 0; i < size; i++) {
    int temp = arr[i];
    temp = temp & 0xff;
    output << hex << (temp >> 4);
    temp = temp & 0xf;
    output << hex << temp << endl;
  }
  output.close();
}
void print_reg() {
  for (int i = 0; i < 16; i++) {
    cout << RF[i] << " ";
  }
  cout << endl;
}
void reg_ini() {
  for (int i = 0; i < 16; i++) {
    if (RF[i] > 127) {
      RF[i] = RF[i] - 256;
    }
  }
}
int main() {                                     // Main
  read_input(ICache, 256, "./input/ICache.txt"); // Read ICache
  read_input(DCache, 256, "./input/DCache.txt"); // Read DCache
  read_input(RF, 16, "./input/RF.txt");          // Read RF
  reg_ini();
  // print_reg();
  // cout << ICache[1] << endl;
  // cout << "start";
  num_cycles = 0;
  for (int i = 0; i < 16; i++) {
    ready[i] = true;
  }
  Buffer B;
  while (pc != 256 && halt != 1) {
    // print_reg();
    num_cycles++;
    writeback(B);
    memaccess(B);
    execute(B);
    decode(B);
    if (B.opcode_to_exec == 13 || B.opcode_to_exec == 14) {
      num_stall++;
      num_con_st++;
      continue;
    }
    if (B.halt_data == 1)
      continue;
    fetch(B);
  }
  num_arith--;
  print_output(DCache, 256, "./output/DCache.txt"); // Print DCache
  print_nos("./output/Output.txt");
}
