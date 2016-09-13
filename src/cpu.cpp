#include "cpu.h"

using namespace std;

unsigned char font[80] = 
{
    0xF0 ,0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};


Chip8::Chip8()
{
    instructions[0x00E0] = &Chip8::cls;
    instructions[0x00EE] = &Chip8::ret;

    instructions[0x1000] = &Chip8::jp_addr;     instructions[0x2000] = &Chip8::call;
    instructions[0x3000] = &Chip8::se_vx_byte;  instructions[0x4000] = &Chip8::sne_vx_byte;
    instructions[0x5000] = &Chip8::se_vx_vy;    instructions[0x6000] = &Chip8::ld_vx_byte;
    instructions[0x7000] = &Chip8::add_vx_byte;

    instructions[0x8000] = &Chip8::ld_vx_vy;    instructions[0x8001] = &Chip8::or_;
    instructions[0x8002] = &Chip8::and_;        instructions[0x8003] = &Chip8::xor_;
    instructions[0x8004] = &Chip8::add_vx_vy;   instructions[0x8005] = &Chip8::sub;
    instructions[0x8006] = &Chip8::shr;         instructions[0x8007] = &Chip8::subn;
    instructions[0x800E] = &Chip8::shl;

    instructions[0x9000] = &Chip8::sne_vx_vy;   instructions[0xA000] = &Chip8::ld_i_addr;
    instructions[0xB000] = &Chip8::jp_v0_addr;  instructions[0xC000] = &Chip8::rnd;
    instructions[0xD000] = &Chip8::drw;

    instructions[0xE09E] = &Chip8::skp;
    instructions[0xE0A1] = &Chip8::sknp;
    
    instructions[0xF007] = &Chip8::ld_vx_dt;    instructions[0xF015] = &Chip8::ld_vx_k;
    instructions[0xF018] = &Chip8::ld_st_vx;    instructions[0xF01E] = &Chip8::add_i_vx;
    instructions[0xF029] = &Chip8::ld_f_vx;     instructions[0xF033] = &Chip8::ld_b_vx;
    instructions[0xF055] = &Chip8::ld_i_vx;     instructions[0xF065] = &Chip8::ld_vx_i;

    pc = 0x200;
    sp = 0;
    I = 0;

    memset(memory, 0, sizeof(memory));
    memset(gfx, 0, sizeof(gfx));
    memset(stack, 0, sizeof(stack));
    memset(V, 0, sizeof(V));
    for (auto i : boost::irange(0, 80))
        memory[i] = font[i];
    

}

void Chip8::render(int* pixels)
{
    for (int pos = 0; pos < W * H; ++pos)
        pixels[pos] = 0xFFFFFF * ((gfx[pos] >> (7 - pos % 8)) & 1);
}

void Chip8::read_file(const char* filename)
{
    auto pos = 0x200;
    ifstream f(filename, ios::in | ios::binary);
    while(f.good())
        memory[pos++] = f.get();
    
}

void Chip8::emulateCycle()
{
    try{
        opcode = memory[pc] << 8 | memory[pc + 1];

        nnn = opcode & 0x0FFF;
        kk = opcode & 0x00FF;

        x = (opcode >> 8) & 0x0F00;
        Vx = V[x];

        y = (opcode >> 4) & 0x00F0;
        Vy = V[y];
        VF = V[0xF];

        auto generic_opcode = opcode;
        auto opcode_test = opcode & 0xF000;

        if(opcode_test == 0x0000)
            generic_opcode = opcode & 0x00FF;

        else if(opcode_test == 0x8000)
            generic_opcode = opcode & 0xF00F;
        
        else if(opcode_test == 0xE000)
            generic_opcode = opcode & 0xF0FF;

        else if(opcode_test == 0xF000)
            generic_opcode = opcode & 0xF0FF;
        
        else
            generic_opcode = opcode & 0xF000;

        if(not instructions[generic_opcode])
            throw NullPointerException();

        else
            (this->*instructions[generic_opcode])();

        if(delay_timer > 0)
            --delay_timer;
    }
    catch (NullPointerException e){
        cerr << e.what() << endl;
        printf("opcode: %04X\n", opcode);
        exit(1);
    }
}

// 0x00E0
void Chip8::cls()
{
    for(auto& i: gfx)
        i = 0;
    pc += 2;
}

// 0x00EE
void Chip8::ret()
{
    pc = stack[--sp];
    pc += 2;
}

// 0x1nnn
void Chip8::jp_addr()
{
    pc = nnn;
}

// 0x2nnn
void Chip8::call()
{
    stack[sp++] = pc;
    pc = nnn;
}

// 0x3xkk
void Chip8::se_vx_byte()
{
    if (Vx == kk)
        pc += 4;
    else
        pc += 2;
}

// 0x4xkk
void Chip8::sne_vx_byte()
{
    if(Vx != kk)
        pc += 4;
    else
        pc += 2;
}

//0x5xy0
void Chip8::se_vx_vy()
{
    if(Vx == Vy)
        pc += 4;
    else
        pc += 2;
}

// 0x6xkk
void Chip8::ld_vx_byte()
{
    Vx = kk;
    pc += 2;
}

// 0x7xkk
void Chip8::add_vx_byte()
{
    Vx = Vx + kk;
    pc += 2;
}

// 0x8xy0
void Chip8::ld_vx_vy()
{
    Vx = Vy;
    pc += 2;
}

// 0x8xy1
void Chip8::or_()
{
    Vx = Vx | Vy;
    pc += 2;
}

// 0x8xy2
void Chip8::and_()
{
    Vx = Vx & Vy;
    pc += 2;
}

// 0x8xy3
void Chip8::xor_()
{
    Vx = Vx ^ Vy;
    pc += 2;
}

// 0x8xy4
void Chip8::add_vx_vy()
{
    if(Vy > (0xFF - Vx))
        VF = 1;
    else
        VF = 0;
    Vx = Vx + Vy;
    pc += 2;
}

// 0x8xy5
void Chip8::sub()
{
    if(Vy > Vx)
        VF = 0;
    else
        VF = 1;
    Vx = Vx - Vy;
    pc += 2;
}

// 0x8xy6
void Chip8::shr()
{
    VF = Vx & 0x1;
    Vx = Vx >> 1;
    pc += 2;
}

// 0x8xy7
void Chip8::subn()
{
    if(Vx > Vy)
        VF = 0;
    else
        VF = 1;
    Vx = Vy - Vx;
}

// 0x8xyE
void Chip8::shl()
{
    VF = Vx >> 7;
    Vx = Vx << 1;
    pc += 2;
}

// 0x9xy0
void Chip8::sne_vx_vy()
{
    if(Vx != Vy)
        pc += 4;
    else
        pc += 2;
}

// 0xAnnn
void Chip8::ld_i_addr()
{
    I = nnn;
    pc += 2;
}

// 0xBnnn
void Chip8::jp_v0_addr()
{
    pc = nnn + V[0];
}

// 0xCxkk
void Chip8::rnd()
{
    mt19937 re(rd());
    uniform_int_distribution<int> ui(0, 255);
    Vx = ui(re) & kk;
    pc += 2;
}

// 0xDxyn
void Chip8::drw()
{
    unsigned short height = opcode & 0x000F;
    unsigned short pixel;

    V[0xF] = 0;
    for (int yline = 0; yline < height; yline++){
        pixel = memory[I + yline];
        for (int xline = 0; xline < 8; xline++){
            if((pixel & (0x80 >> xline)) != 0){
                if(gfx[(Vx + xline + ((Vy + yline) * 64))] == 1){
                    VF = 1;
                }
                gfx[Vx + xline + ((Vy + yline) * 64)] ^= 1;
            }
        }
    }
    pc += 2;
}

// 0xEx9E
void Chip8::skp()
{
    if(key[Vx] != 0)
        pc += 4;
    else
        pc += 2;
}

// 0xExA1
void Chip8::sknp()
{
    if(key[Vx] == 0)
        pc += 4;
    else
        pc += 2;
}

// 0xFx07
void Chip8::ld_vx_dt()
{
    Vx = delay_timer;
    pc += 2;
}

// 0xFx0A
void Chip8::ld_vx_k()
{
    bool keyPress = false;
    for (int i = 0; i < 16; ++i){
        if(key[i] != 0){
            Vx = i;
            keyPress = true;
        }
    }
    waiting_key = 0x80 | x;

    if(!keyPress)
        return;

    pc += 2;
}

// 0xFx15
void Chip8::ld_dt_vx()
{
    delay_timer = Vx;
    pc += 2;
}

// 0xFx18
void Chip8::ld_st_vx()
{
    sound_timer = Vx;
    pc += 2;
}

// 0xFx1E
void Chip8::add_i_vx()
{
    I = I + Vx;
    pc += 2;
}

// 0xFx29
void Chip8::ld_f_vx()
{
    I = Vx * 0x5;
    pc += 2;
}

// 0xFx33
void Chip8::ld_b_vx()
{
    memory[I] = Vx / 100;
    memory[I + 1] = (Vx / 10) % 10;
    memory[I + 2] = (Vx % 100) % 10;
    pc += 2;
}

// 0xFx55
void Chip8::ld_i_vx()
{
    for (int i = 0; i <= x; ++i)
        memory[I + i] = V[i];
    I = I + x + 1;
    pc += 2;
}

// 0xFx65
void Chip8::ld_vx_i()
{
    for (int i = 0; i <= x; ++i)
        V[i] = memory[I + i];
    I = I + x + 1;
    pc += 2;
}



int main(int argc, char** argv)
{
    Chip8 cpu;
    cpu.read_file(argv[1]);

    



}