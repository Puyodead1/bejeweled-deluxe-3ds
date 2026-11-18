// Phase 1 smoke test: SDL2 window + renderer, SDL2 audio driven by libmodplug playing an S3M from romfs, controller
// and touch input. Writes a log to sdmc:/switch/Bejeweled/hello.log so a run in an emulator can be checked.
// Press + (or touch the top-left corner) to quit.

#include <switch.h>
#include <SDL.h>
#include <libmodplug/modplug.h>

#include <dirent.h>
#include <sys/stat.h>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static FILE* gLog = nullptr;

static void Log(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (gLog)
    {
        fputs(buf, gLog);
        fputc('\n', gLog);
        fflush(gLog);
    }
    printf("%s\n", buf);
}

static int CountFiles(const std::string& dir, long long& bytes)
{
    DIR* d = opendir(dir.c_str());
    if (!d)
        return 0;
    int count = 0;
    while (dirent* e = readdir(d))
    {
        std::string name = e->d_name;
        if (name == "." || name == "..")
            continue;
        std::string path = dir + (dir.back() == '/' ? "" : "/") + name;
        struct stat st;
        if (stat(path.c_str(), &st) != 0)
            continue;
        if (S_ISDIR(st.st_mode))
            count += CountFiles(path, bytes);
        else
        {
            count++;
            bytes += st.st_size;
        }
    }
    closedir(d);
    return count;
}

static std::vector<char> ReadFile(const char* path)
{
    std::vector<char> data;
    FILE* f = fopen(path, "rb");
    if (!f)
        return data;
    fseek(f, 0, SEEK_END);
    data.resize(ftell(f));
    fseek(f, 0, SEEK_SET);
    data.resize(fread(data.data(), 1, data.size(), f));
    fclose(f);
    return data;
}

static SDL_atomic_t gAudioCallbacks;

static void AudioCallback(void* userdata, Uint8* stream, int len)
{
    SDL_AtomicIncRef(&gAudioCallbacks);
    ModPlugFile* mod = static_cast<ModPlugFile*>(userdata);
    int got = ModPlug_Read(mod, stream, len);
    if (got < len)
        SDL_memset(stream + got, 0, len - got);
}

int main(int argc, char** argv)
{
    romfsInit();
    mkdir("sdmc:/switch", 0777);
    mkdir("sdmc:/switch/Bejeweled", 0777);
    gLog = fopen("sdmc:/switch/Bejeweled/hello.log", "w");
    Log("hello: start, argc=%d", argc);

    // "--quit-after N": exit by itself after N seconds (for scripted emulator runs: eden-cli -p "--quit-after 20").
    Uint32 quitAfterMs = 0;
    for (int i = 1; i + 1 < argc; i++)
        if (std::string(argv[i]) == "--quit-after")
            quitAfterMs = (Uint32)atoi(argv[i + 1]) * 1000;
    if (quitAfterMs)
        Log("will quit after %u ms", quitAfterMs);

    long long bytes = 0;
    int files = CountFiles("romfs:/", bytes);
    Log("romfs: %d files, %lld bytes", files, bytes);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0)
    {
        Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    SDL_version v;
    SDL_GetVersion(&v);
    Log("SDL %d.%d.%d, video driver %s, audio driver %s", v.major, v.minor, v.patch, SDL_GetCurrentVideoDriver(),
        SDL_GetCurrentAudioDriver());

    SDL_Window* window = SDL_CreateWindow("Bejeweled", 0, 0, 1280, 720, 0);
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) : nullptr;
    if (!renderer)
    {
        Log("window/renderer failed: %s", SDL_GetError());
        return 1;
    }
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer, &info);
    Log("renderer: %s", info.name);

    for (int i = 0; i < SDL_NumJoysticks(); i++)
        SDL_JoystickOpen(i);
    Log("joysticks: %d", SDL_NumJoysticks());

    // Music: the title-screen song, through libmodplug.
    std::vector<char> song = ReadFile("romfs:/music/1980-autonomus.s3m");
    Log("song: %zu bytes", song.size());
    ModPlug_Settings mps;
    ModPlug_GetSettings(&mps);
    mps.mChannels = 2;
    mps.mBits = 16;
    mps.mFrequency = 44100;
    mps.mResamplingMode = MODPLUG_RESAMPLE_LINEAR;
    mps.mLoopCount = -1;
    ModPlug_SetSettings(&mps);
    ModPlugFile* mod = song.empty() ? nullptr : ModPlug_Load(song.data(), (int)song.size());
    Log("modplug: %s, name '%s'", mod ? "loaded" : "FAILED", mod ? ModPlug_GetName(mod) : "");

    SDL_AudioDeviceID audio = 0;
    if (mod)
    {
        SDL_AudioSpec want = {}, have = {};
        want.freq = 44100;
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = 1024;
        want.callback = AudioCallback;
        want.userdata = mod;
        audio = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
        Log("audio device: %u (%d Hz, %d ch)", audio, have.freq, have.channels);
        if (audio)
            SDL_PauseAudioDevice(audio, 0);
    }

    bool running = true;
    Uint32 start = SDL_GetTicks();
    int frames = 0;
    int touches = 0, buttons = 0;
    while (running && appletMainLoop())
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_JOYBUTTONDOWN:
                buttons++;
                Log("button %d", e.jbutton.button);
                if (e.jbutton.button == 10) // + on the Switch SDL joystick mapping
                    running = false;
                break;
            case SDL_FINGERDOWN:
                touches++;
                Log("touch %.3f %.3f", e.tfinger.x, e.tfinger.y);
                if (e.tfinger.x < 0.1f && e.tfinger.y < 0.1f)
                    running = false;
                break;
            }
        }

        // Seven "gems" bouncing in the game's palette, so a screenshot shows the renderer works.
        float t = (SDL_GetTicks() - start) / 1000.0f;
        SDL_SetRenderDrawColor(renderer, 16, 16, 40, 255);
        SDL_RenderClear(renderer);
        static const SDL_Color kColors[7] = {{230, 30, 30, 255}, {240, 240, 240, 255}, {40, 200, 40, 255}, {240, 220, 30, 255},
                                             {200, 40, 220, 255}, {240, 130, 20, 255}, {40, 120, 240, 255}};
        for (int i = 0; i < 7; i++)
        {
            SDL_Rect r = {160 + i * 140, 320 + (int)(std::sin(t * 3 + i) * 120), 96, 96};
            SDL_SetRenderDrawColor(renderer, kColors[i].r, kColors[i].g, kColors[i].b, 255);
            SDL_RenderFillRect(renderer, &r);
        }
        SDL_RenderPresent(renderer);

        frames++;
        Uint32 elapsed = SDL_GetTicks() - start;
        static Uint32 nextReport = 5000;
        if (elapsed >= nextReport)
        {
            Log("t=%us: %d frames (%.1f fps avg), %d audio callbacks", elapsed / 1000, frames, frames * 1000.0f / elapsed,
                SDL_AtomicGet(&gAudioCallbacks));
            nextReport += 5000;
        }
        if (quitAfterMs && elapsed >= quitAfterMs)
            running = false;
    }

    Log("exit: %d frames, %d buttons, %d touches", frames, buttons, touches);
    if (audio)
        SDL_CloseAudioDevice(audio);
    if (mod)
        ModPlug_Unload(mod);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    romfsExit();
    if (gLog)
        fclose(gLog);
    return 0;
}
