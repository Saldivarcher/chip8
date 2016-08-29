#include <fstream>
#include <iostream>
#include <random>
#include <boost/range/irange.hpp>
#include <SDL2/SDL.h>

using namespace std;

class Chip8
{
private:
    unsigned short opcode;
    unsigned char memory[4096];
    unsigned char V[16], sp, waiting_key;
    unsigned short I, pc;
    unsigned short stack[16];
    random_device rd;

    unsigned char gfx[64 * 32];
    unsigned char font[16 * 5] = 
    {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x10, 0x10, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
    unsigned char delay_timer, sound_timer;

public:
    void initialize();
    void emulateCycle();
    void read_file(const char*);
};


void Chip8::initialize()
{
    pc = 0x200; // Program counter starts at 0x200
    opcode = 0; // Reset current opcode
    I = 0;      // Reset index register
    sp = 0;     // Reset stack pointer


    delay_timer = 0; sound_timer = 0;

    // clearing display
    for(auto &c: gfx)
        c = 0;
    
    // setting all memory locations to 0
    for(auto &i: memory)
        i = 0;
    
    // setting the first 80 bits of memory to the font
    // using the boost library for this
    for(auto i: boost::irange(0,80))
        memory[i] = font[i];
}

void Chip8::emulateCycle()
{
    opcode = memory[pc] << 8 | memory[pc +  1];
    opcode = opcode & 0xF000;

    // try to keep things in 16 bits to help visualize

    // x - A 4-bit value, the lower 4 bits of the high byte of the instruction
    auto Vx = (opcode >> 8) & 0x0F00;

    // y - A 4-bit value, the upper 4 bits of the low byte of the instruction
    auto Vy = (opcode >> 4) & 0x00F0;

    // kk or byte - An 8-bit value, the lowest 8 bits of the instruction
    auto kk = opcode & 0x00FF;

    // nnn or addr - A 12-bit value, the lowest 12 bits of the instruction
    auto nnn = opcode & 0x0FFF;

    switch(opcode & 0xF000){
        default: break;
        case 0x0000:
            switch(opcode & 0x000F){
                // clear display command
                case 0x00E0:
                    for(auto &c: gfx)
                        c = 0;
                    pc += 2;
                    break;

                // return from subroutine 
                case 0x00EE:
                    pc = stack[sp--];
                    pc += 2;
                    break;
            }
            break;

        // jump to location 'nnn'
        case 0x1000:
            pc = nnn;
            break;

        // call subroutine 'nnn'
        case 0x2000:
            stack[++sp] = pc;
            pc = nnn;
            break;

        // skip next instruction if Vx == kk
        case 0x3000:
            if(Vx == kk)
                pc += 4; 
            else
                pc += 2;
            break;             
        
        // skip next instruction if Vx != kk
        case 0x4000: 
            if(Vx == kk)
                pc += 2;
            else 
                pc += 4;
            break;
        
        // skip next instruction if Vx == Vy
        case 0x5000:
            if(Vx == Vy) 
                pc += 4;
            else
                pc += 2;
            break;
        
        // set Vx = kk
        case 0x6000:
            Vx = kk;
           pc += 2;
            break;
        
        // set Vx = Vx + kk
        case 0x7000:
           Vx += kk; 
           pc += 2;
           break;
        
        case 0x8000:
            switch(opcode & 0x000F){
                // set Vx = Vy
                case 0x0000:
                    Vx = Vy;
                    pc += 2;
                    break;
                
                // set Vx = Vx OR Vy
                case 0x0001:
                    Vx = Vx | Vy;
                    pc += 2;
                    break;
                
                // set Vx = Vx AND Vy
                case 0x0002:
                    Vx = Vx & Vy;
                    pc += 2;
                    break;

                // set Vx = Vx XOR Vy
                case 0x0003:
                    Vx = Vx ^ Vy;
                    pc += 2;
                    break;

                // set Vx = Vx + Vy then set VF = carry
                case 0x0004:
                    if(Vx += Vy > 0x00FF)
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;
                    Vx = Vx + Vy;
                    pc += 2;
                    break; 
                
                // set Vx = Vx - Vy then set VF = 1 if Vx > Vy else 0
                case 0x0005:
                    if(Vx > Vy)
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;
                    Vx = Vx - Vy;
                    pc += 2;
                    break;

                // set Vx = Vx SHR 1
                case 0x0006:
                    V[0xF] = Vx & 0x0001;
                    Vx = Vx >> 1;
                    pc += 2;
                    break;

                // set Vx = Vx - Vy, set VF = NOT borrow
                case 0x0007:
                    if(Vy > Vx)
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;
                    Vx = Vy - Vx;
                    pc += 2;
                    break;
                
                // set Vx = Vx << 1
                case 0x000E:
                    V[0xF] = Vx >> 7;
                    Vx = Vx << 1;
                    pc += 2;
                    break;
            }
            break;

        // Skip next instr if Vx != Vy
        case 0x9000:
            if(Vx != Vy)
                pc += 4;
            else
                pc += 2;
            break;
        
        // Set I to the address of nnn
        case 0xA000:
            I = nnn;
            pc += 2;
            break;
        
        // Jump to location nnn + V0
        case 0xB000:
            pc = nnn + V[0x0000];
            break;
        
        // Set Vx = random byte AND kk -> random byte being between 0-255
        case 0xC000:
            mt19937 re(rd());
            uniform_int_distribution<int> ui(0, 255);
            Vx = ui(re) & kk;
            pc += 2;
            break;

        case 0xD000:
             

    }
    
}


void Chip8::read_file(const char* filename)
{
    ifstream file(filename, ios::binary);
    while(file.good())
        memory[pc++ & 0xFFF] = file.get();
}


int main(int argc, char** argv)
{
    Chip8 cpu;
    cpu.initialize();
    cpu.read_file(argv[1]);
}
