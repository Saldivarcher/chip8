#include <fstream>
#include <iostream>
#include <random>
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


    // clearing display
    for(auto &c: gfx)
        c = 0;
    
    // setting all memory locations to 0
    for(auto &i: memory)
        i = 0;
    
    // setting the first 80 bits of memory to the font
    for(int i = 0; i < 80; i++)
        memory[i] = font[i];
}

void Chip8::emulateCycle()
{
    opcode = memory[pc] << 8 | memory[pc +  1];
    opcode = opcode & 0xF000;
    switch(opcode){
        default: break;
        case 0x0000:
            switch(opcode){
                case 0x00E0:
                    for(auto &c: gfx)
                        c = 0;
                    pc += 2;
                    break;
                case 0x00EE:
                    pc = stack[sp--];
                    pc += 2;
                    break;
            }
            break;
        case 0x1000:
            pc = opcode & 0x0FFF;
            break;
        case 0x2000:
            stack[++sp] = pc;
            pc = opcode & 0x0FFF;
            break;
        case 0x3000:
            break;             
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
