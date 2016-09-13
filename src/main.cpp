#include "../include/cpu.h"


static deque<pair<unsigned, bool>> AudioQueue;

int main(int argc, char** argv)
{
    Chip8 cpu;
    cpu.read_file(argv[1]);
    
    for(auto &i: cpu.memory)
        printf("memory: %04X\n", i);

    
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
    
    SDL_AudioSpec spec, obtained;
    spec.freq = 44100;
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;
    spec.samples = spec.freq / 20;
    spec.callback = [](void *, Uint8 *stream, int len) {
        short *target = (short *)stream;
        while(len > 0 && !AudioQueue.empty()){
            auto &data = AudioQueue.front();
            for (; len && data.first; target += 2, len -= 4, --data.first)
                target[0] = target[1] = data.second * 300 * ((len & 128) - 64);
            if(!data.first)
                AudioQueue.pop_front();
        }
    };

    SDL_OpenAudio(&spec, &obtained);
    SDL_PauseAudio(0);

    unsigned instr_per_frame = 50000;
    unsigned max_consecutive_insns = 0;
    int frames_done = 0;
    bool loop = false;
    

    auto start = chrono::system_clock::now();
    while (!loop)
    {
        for (int i = 0; i < max_consecutive_insns and !(cpu.waiting_key & 0x80); ++i){
            cpu.emulateCycle();
            printf("opcode: %04X\n", cpu.opcode);
        }

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
                    if(event.type == SDL_KEYDOWN and (cpu.waiting_key & 0x80)){
                        cpu.waiting_key &= 0x7F;
                        cpu.V[cpu.waiting_key] = i->second;
                    }
                }
        }
        auto cur = chrono::system_clock::now();
        chrono::duration<double> elapsed_seconds = cur - start;
        int frames = int(elapsed_seconds.count() * 60) - frames_done;
        if(frames > 0){
            frames_done += frames;

            int st = std::min(frames, cpu.sound_timer+0); cpu.sound_timer -= st;
            int dt = std::min(frames, cpu.delay_timer+0); cpu.delay_timer -= dt;

            SDL_LockAudio();
            AudioQueue.emplace_back(obtained.freq * (st) / 60, true);
            AudioQueue.emplace_back(obtained.freq * (frames - st) / 60, false);
            SDL_UnlockAudio();

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