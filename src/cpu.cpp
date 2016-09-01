#include <fstream>
#include <unordered_map>
#include <iostream>
#include <random>
#include <deque>
#include <chrono>


#include <boost/range/irange.hpp>
#include <SDL2/SDL.h>

using namespace std;

static const int W = 64, H = 32;

class Chip8
{
public:
    unsigned short opcode;
    unsigned char memory[4096];
    unsigned char V[16], sp;
    unsigned short I, pc;
    unsigned short stack[16];
    random_device rd;
    unsigned char gfx[W * H], waiting_key;
    unsigned char key[16];
    unsigned char delay_timer, sound_timer;

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

    Chip8();
    void emulateCycle();
    void read_file(const char *);
    void render(Uint32 *pixels);


};

Chip8::Chip8()
{
    pc = 0x200; // Program counter starts at 0x200
    opcode = 0; // Reset current opcode
    I = 0;      // Reset index register
    sp = 0;     // Reset stack pointer


    delay_timer = 0; sound_timer = 0;
    
    for(auto i: boost::irange(0,15)){
        key[i] = 0; 
        stack[i] = 0; 
        V[i] = 0; 
    }

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
    auto Vx = V[(opcode >> 8) & 0x0F00];
    auto x = opcode >> 8 & 0x0F00;

    // y - A 4-bit value, the upper 4 bits of the low byte of the instruction
    auto Vy = V[(opcode >> 4) & 0x00F0];
    auto y = opcode >> 4 & 0x00F0;

    // kk or byte - An 8-bit value, the lowest 8 bits of the instruction
    auto kk = opcode & 0x00FF;

    // nnn or addr - A 12-bit value, the lowest 12 bits of the instruction
    auto nnn = opcode & 0x0FFF;

    switch(opcode & 0xF000){
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
        {
            mt19937 re(rd());
            uniform_int_distribution<int> ui(0, 255);
            Vx = ui(re) & kk;
            pc += 2;
        }
            break;

        // Set Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
        case 0xD000:
        {
            unsigned short height = opcode & 0x000F;
            unsigned short pixel;

            V[0xF] = 0;
            for (int yline = 0; yline < height; yline++){
                pixel = memory[I + yline];
                for (int xline = 0; xline < 8; xline++){
                    if((pixel & (0x80 >> xline)) != 0){
                        if(gfx[(x + xline + (y + yline) * 64)] == 1){
                            V[0xF] = 1;
                        }
                        gfx[x + xline + ((y + yline) * 64)] ^= 1;
                    }
                }
            }
        }
            break;
        
        case 0xE000:
            switch(opcode & 0x00FF){

                // Skip next instruction if key with the value of Vx is pressed.
                case 0x009E:
                    if(key[Vx] != 0)
                        pc += 4;
                    else
                        pc += 2;
                    break;

                //Skip next instruction if key with the value of Vx is not pressed.
                case 0x00A1:
                    if(key[Vx] == 0)
                        pc += 4;
                    else
                        pc += 2;
                    break;
                }

        case 0xF000:
            switch(opcode & 0x00FF){
                // Set Vx = delay timer
                case 0x0007:
                    Vx = delay_timer;
                    pc += 2;
                    break;

                // Wait for key press, store the value of the key in Vx
                case 0x000A:
                {
                    bool key_press = false;
                    for(auto i: boost::irange(0, 16)){
                        if(key[i] != 0){
                            Vx = i;
                            key_press = true;
                        }
                    }
                    if(!key_press)
                        return;
                    pc += 2;
                }
                    break;
                
                // Set delay time = Vx
                case 0x0015:
                    delay_timer = Vx;
                    pc += 2;
                    break;
                
                // Set sound timer = Vx
                case 0x0018:
                    sound_timer = Vx;
                    pc += 2;
                    break;
                
                // Set I = I + Vx
                case 0x001E:
                    I = I + Vx;
                    pc += 2;
                    break;
                
                // Set I = location of sprite for digit Vx
                case 0x0029:
                    I = Vx * 0x5;
                    pc += 2;
                    break;

                // Store BCD representation of Vx in memory locations I, I+1, and I+2
                case 0x0033:
                    memory[I] = Vx / 100;
                    memory[I + 1] = (Vx / 10) % 10;
                    memory[I + 2] = (Vx % 100) % 10;
                    pc += 2;
                    break;
                
                // Store V0 to Vx in memory starting at address I
                case 0x0055:
                    for(auto i: boost::irange(0, x))
                        memory[I + i] = V[i];
                    I = I + x + 1;
                    pc += 2;
                    break;
                
                // Stores V0 - Vx with values from memory
                case 0x0065:
                    for(auto i: boost::irange(0,x))
                        V[i] = memory[I + i];
                    I = I + x + 1;
                    pc += 2;
                    break;
                
                default:
                    break;
                }
        break;
        default:
            break;

    }
    if(delay_timer > 0)
        delay_timer--;
    if(sound_timer > 0){
        if(sound_timer == 1)
            printf("Beep boop? \n");
        sound_timer--;
    }
}

void Chip8::render(Uint32* pixels)
{
    for (int i: boost::irange(0, W*H))
        pixels[i] = 0xFFFFFF * ((gfx[i] >> (7 - i)) & 1);
}

void Chip8::read_file(const char* filename)
{
    auto pos = pc;
    printf("this is pos: %X", pos);
    ifstream file(filename, ios::binary);
    while(file.good())
        memory[pos++ & 0xFFF] = file.get();
}


int main(int argc, char** argv)
{
    Chip8 cpu;
    cpu.read_file(argv[1]);
    
    // creating the screen
    SDL_Window *window = SDL_CreateWindow(argv[1], SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, W * 4, H * 6, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *ren = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture *texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, W, H);

    unordered_map<int,int> keymap{
        {SDLK_1, 0x1}, {SDLK_2, 0x2}, {SDLK_3, 0x3}, {SDLK_4, 0xC},
        {SDLK_q, 0x4}, {SDLK_w, 0x5}, {SDLK_e, 0x6}, {SDLK_r, 0xD},
        {SDLK_a, 0x7}, {SDLK_s, 0x8}, {SDLK_d, 0x9}, {SDLK_f, 0xE},
        {SDLK_z, 0xA}, {SDLK_x, 0x0}, {SDLK_c, 0xB}, {SDLK_v, 0xF},
        {SDLK_5, 0x5}, {SDLK_6, 0x6}, {SDLK_7, 0x7},
        {SDLK_8, 0x8}, {SDLK_9, 0x9}, {SDLK_0, 0x0}, {SDLK_ESCAPE,-1}
    };

    unsigned instr_per_frame = 50000;
    unsigned max_consecutive_insns = 0;
    int frames_done = 0;
    bool loop = false;

    auto start = chrono::system_clock::now();
    while (!loop)
    {
        for (int i = 0; i < max_consecutive_insns and !(cpu.waiting_key & 0x80); ++i)
            cpu.emulateCycle();

        for (SDL_Event event; SDL_PollEvent(&event);){
            switch(event.type){
                case SDL_QUIT:
                    loop = true;
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    auto i = keymap.find(event.key.keysym.sym);
                    if(i == keymap.end())
                        break;
                    if(i->second == -1){
                        loop = true;
                        break;
                    }
                    cpu.key[i->second] = event.type == SDL_KEYDOWN;
                    if(event.type == SDL_KEYDOWN and (cpu.waiting_key & 0x80))
                        cpu.waiting_key &= 0x7F;
                }
        }
        auto cur = chrono::system_clock::now();
        chrono::duration<double> elapsed_seconds = cur - start;
        int frames = int(elapsed_seconds.count() * 60) - frames_done;
        if(frames > 0){
            frames_done += frames;

            Uint32 pixels[W * H];
            cpu.render(pixels);
            SDL_UpdateTexture(texture, nullptr, pixels, 4 * W);
            SDL_RenderCopy(ren, texture, nullptr, nullptr);
            SDL_RenderPresent(ren);
        }
        max_consecutive_insns = max(frames, 1) * instr_per_frame;

        if((cpu.waiting_key & 0x80) or !frames)
            SDL_Delay(1000 / 60);
    }

    SDL_Quit();
}
