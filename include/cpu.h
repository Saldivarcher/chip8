#ifndef CPU_H
#define CPU_H

#include <fstream>
#include <iostream>
#include <random>
#include <unordered_map>
#include <map>
#include <stdexcept>
#include <boost/range/irange.hpp>
#include <SDL2/SDL.h>


#include <chrono>
#include <utility>


class NullPointerException: public std::runtime_error 
{
public:
    NullPointerException(): std::runtime_error("Invalid Opcode!") { }
};

static const int W = 64, H = 32;

class Chip8
{
private:
    unsigned short nnn, kk, VF, Vx, Vy, x, y;
public:
    unsigned short opcode;
    unsigned char memory[4096];
    unsigned char V[16], sp, waiting_key;
    unsigned short I, pc, stack[12];
    std::random_device rd;
    unsigned char gfx[(W * H)], font[16 * 5];
    unsigned char key[16];
    unsigned char delay_timer, sound_timer;

    // using functions pointers and a map
    // bc switch cases are ugly.
    typedef void (Chip8::*FP)();
    std::map<int, FP> instructions;

    Chip8();
    void emulateCycle();
    void read_file(const char *);
    void render(Uint32*);

    /******instructions*******/

    // 0x0*** -> instructions
    void cls();
    void ret();

    // 0x1*** - 0x7*** -> instructions
    void jp_addr();
    void call();
    void se_vx_byte();
    void sne_vx_byte();
    void se_vx_vy();
    void ld_vx_byte();
    void add_vx_byte();

    // 0x8*** -> instructions
    void ld_vx_vy();
    void or_();
    void and_();
    void xor_();
    void add_vx_vy();
    void sub();
    void shr();
    void subn();
    void shl();

    // 0x9*** - 0xD*** -> instructions
    void sne_vx_vy();
    void ld_i_addr();
    void jp_v0_addr();
    void rnd();
    void drw();

    // 0xE*** -> instructions
    void skp();
    void sknp();

    // 0xF*** -> instructions
    void ld_vx_dt();
    void ld_vx_k();
    void ld_dt_vx();
    void ld_st_vx();
    void add_i_vx();
    void ld_f_vx();
    void ld_b_vx();
    void ld_i_vx();
    void ld_vx_i();
};

#endif