/***********************************************************************************
**
** SoundBackend.cpp
**
** SDL_mixer implementation of the handful of BASS entry points CSoundManager
** uses. BASS is Windows-only, so without this a non-Windows build is silent:
** every BASS_* name used to be a no-op macro in Stubs.h.
**
** Two things do not map one-to-one and are handled here:
**
**   - BASS streams are uniform; SDL_mixer splits them into Mix_Chunk (a fully
**     decoded sample played on one of many channels) and Mix_Music (a single
**     streamed track). Sound effects come from memory and become chunks, music
**     comes from a file and becomes a Mix_Music.
**   - SDL_mixer plays only one Mix_Music at a time. CSoundManager holds both a
**     normal and a war track, but it always stops one before starting the other,
**     so a single music slot is sufficient. g_CurrentMusic tracks which handle
**     owns it so BASS_ChannelIsActive can answer per-handle.
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "stdafx.h"

#if !defined(ORION_WINDOWS)

#include <SDL2/SDL_mixer.h>

#include <string>
#include <vector>

namespace
{
struct SoundStream
{
    // Exactly one of these is set.
    Mix_Chunk *chunk = nullptr;
    Mix_Music *music = nullptr;

    // Backing bytes for a memory chunk; Mix_LoadWAV_RW copies, but keeping the
    // buffer costs little and keeps ownership obvious.
    std::vector<unsigned char> data;

    int channel = -1;
    bool loop = false;
    float volume = 1.0f;
};

bool g_Initialized = false;

// The handle currently occupying SDL_mixer's single music slot, if any.
SoundStream *g_CurrentMusic = nullptr;

int ToMixVolume(float volume)
{
    if (volume <= 0.0f)
        return 0;
    if (volume >= 1.0f)
        return MIX_MAX_VOLUME;
    return (int)(volume * MIX_MAX_VOLUME);
}

SoundStream *AsStream(HSTREAM handle)
{
    return reinterpret_cast<SoundStream *>(handle);
}
} // namespace

//----------------------------------------------------------------------------------
bool BASS_Init(int, int frequency, int, void *, void *)
{
    if (g_Initialized)
        return true;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
    {
        LOG("SDL_InitSubSystem(SDL_INIT_AUDIO) failed: %s\n", SDL_GetError());
        return false;
    }

    // Ask for the decoders UO needs. Mix_Init reports what it actually got, and
    // a missing one is not fatal: it only means that format will fail to load.
    const int wanted = MIX_INIT_MP3 | MIX_INIT_MID | MIX_INIT_OGG;
    const int got = Mix_Init(wanted);
    if ((got & wanted) != wanted)
        LOG("Mix_Init: some decoders unavailable (wanted 0x%x, got 0x%x): %s\n",
            wanted,
            got,
            Mix_GetError());

    if (frequency <= 0)
        frequency = 44100;

    if (Mix_OpenAudio(frequency, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        LOG("Mix_OpenAudio failed: %s\n", Mix_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }

    // UO can overlap a fair number of effects; the default of 8 is too few.
    Mix_AllocateChannels(64);

    g_Initialized = true;
    return true;
}
//----------------------------------------------------------------------------------
void BASS_Free()
{
    if (!g_Initialized)
        return;

    Mix_HaltMusic();
    Mix_HaltChannel(-1);
    g_CurrentMusic = nullptr;

    Mix_CloseAudio();
    Mix_Quit();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);

    g_Initialized = false;
}
//----------------------------------------------------------------------------------
void BASS_Start()
{
    if (!g_Initialized)
        return;

    Mix_Resume(-1);
    Mix_ResumeMusic();
}
//----------------------------------------------------------------------------------
void BASS_Pause()
{
    if (!g_Initialized)
        return;

    Mix_Pause(-1);
    Mix_PauseMusic();
}
//----------------------------------------------------------------------------------
float BASS_GetVolume()
{
    // BASS reports the device volume in 0..1 and CSoundManager scales the
    // configured client volume against it. SDL_mixer has no device volume, so
    // report unity and let the client setting be the only attenuation.
    return 1.0f;
}
//----------------------------------------------------------------------------------
bool BASS_SetConfig(int, int)
{
    // Interpolation and 3D-algorithm hints have no SDL_mixer equivalent.
    return true;
}
//----------------------------------------------------------------------------------
bool BASS_SetConfigPtr(int option, const char *value)
{
    if (option != BASS_CONFIG_MIDI_DEFFONT || value == nullptr || *value == 0)
        return false;

    // fluidsynth needs a soundfont to render MIDI at all.
    if (!fs_path_exists(os_path(value)))
    {
        LOG("MIDI soundfont not found: %s\n", value);
        return false;
    }

    if (Mix_SetSoundFonts(value) == 0)
    {
        LOG("Mix_SetSoundFonts failed: %s\n", Mix_GetError());
        return false;
    }

    return true;
}
//----------------------------------------------------------------------------------
HSTREAM BASS_StreamCreateFile(
    bool fromMemory, const void *file, uint64_t offset, uint64_t length, uint32_t flags)
{
    if (!g_Initialized || file == nullptr)
        return nullptr;

    auto *stream = new SoundStream();
    stream->loop = (flags & BASS_SAMPLE_LOOP) != 0;

    if (fromMemory)
    {
        const auto *bytes = reinterpret_cast<const unsigned char *>(file) + offset;
        stream->data.assign(bytes, bytes + (size_t)length);

        SDL_RWops *rw = SDL_RWFromConstMem(stream->data.data(), (int)stream->data.size());
        if (rw == nullptr)
        {
            LOG("SDL_RWFromConstMem failed: %s\n", SDL_GetError());
            delete stream;
            return nullptr;
        }

        // freesrc = 1: Mix_LoadWAV_RW closes the RWops for us, on success or not.
        stream->chunk = Mix_LoadWAV_RW(rw, 1);
        if (stream->chunk == nullptr)
        {
            LOG("Mix_LoadWAV_RW failed: %s\n", Mix_GetError());
            delete stream;
            return nullptr;
        }
    }
    else
    {
        const char *path = reinterpret_cast<const char *>(file);
        stream->music = Mix_LoadMUS(path);
        if (stream->music == nullptr)
        {
            LOG("Mix_LoadMUS('%s') failed: %s\n", path, Mix_GetError());
            delete stream;
            return nullptr;
        }
    }

    return reinterpret_cast<HSTREAM>(stream);
}
//----------------------------------------------------------------------------------
HSTREAM BASS_MIDI_StreamCreateFile(
    bool fromMemory, const void *file, uint64_t offset, uint64_t length, uint32_t flags, uint32_t)
{
    // SDL_mixer routes .mid through the same loader as every other music format.
    return BASS_StreamCreateFile(fromMemory, file, offset, length, flags);
}
//----------------------------------------------------------------------------------
bool BASS_ChannelPlay(HSTREAM handle, bool loop)
{
    SoundStream *stream = AsStream(handle);
    if (!g_Initialized || stream == nullptr)
        return false;

    // CSoundManager passes the loop flag here for MIDI, and sets it through
    // BASS_SAMPLE_LOOP at creation time for MP3, so honour either.
    const bool shouldLoop = loop || stream->loop;

    if (stream->music != nullptr)
    {
        if (Mix_PlayMusic(stream->music, shouldLoop ? -1 : 1) < 0)
        {
            LOG("Mix_PlayMusic failed: %s\n", Mix_GetError());
            return false;
        }

        Mix_VolumeMusic(ToMixVolume(stream->volume));
        g_CurrentMusic = stream;
        return true;
    }

    if (stream->chunk != nullptr)
    {
        const int channel = Mix_PlayChannel(-1, stream->chunk, shouldLoop ? -1 : 0);
        if (channel < 0)
        {
            LOG("Mix_PlayChannel failed: %s\n", Mix_GetError());
            return false;
        }

        stream->channel = channel;
        Mix_Volume(channel, ToMixVolume(stream->volume));
        return true;
    }

    return false;
}
//----------------------------------------------------------------------------------
void BASS_ChannelStop(HSTREAM handle)
{
    SoundStream *stream = AsStream(handle);
    if (!g_Initialized || stream == nullptr)
        return;

    if (stream->music != nullptr)
    {
        if (g_CurrentMusic == stream)
        {
            Mix_HaltMusic();
            g_CurrentMusic = nullptr;
        }
        return;
    }

    if (stream->chunk != nullptr && stream->channel >= 0)
    {
        // Only halt the channel if it is still ours; it may have been recycled.
        if (Mix_GetChunk(stream->channel) == stream->chunk)
            Mix_HaltChannel(stream->channel);

        stream->channel = -1;
    }
}
//----------------------------------------------------------------------------------
bool BASS_ChannelIsActive(HSTREAM handle)
{
    SoundStream *stream = AsStream(handle);
    if (!g_Initialized || stream == nullptr)
        return false;

    if (stream->music != nullptr)
        return g_CurrentMusic == stream && Mix_PlayingMusic() != 0;

    if (stream->chunk != nullptr && stream->channel >= 0)
        return Mix_Playing(stream->channel) != 0 &&
               Mix_GetChunk(stream->channel) == stream->chunk;

    return false;
}
//----------------------------------------------------------------------------------
void BASS_ChannelSetAttribute(HSTREAM handle, int attribute, float value)
{
    SoundStream *stream = AsStream(handle);
    if (!g_Initialized || stream == nullptr || attribute != BASS_ATTRIB_VOL)
        return;

    stream->volume = value;

    if (stream->music != nullptr)
    {
        if (g_CurrentMusic == stream)
            Mix_VolumeMusic(ToMixVolume(value));
        return;
    }

    if (stream->chunk != nullptr)
    {
        Mix_VolumeChunk(stream->chunk, ToMixVolume(value));
        if (stream->channel >= 0 && Mix_GetChunk(stream->channel) == stream->chunk)
            Mix_Volume(stream->channel, ToMixVolume(value));
    }
}
//----------------------------------------------------------------------------------
bool BASS_StreamFree(HSTREAM handle)
{
    SoundStream *stream = AsStream(handle);
    if (stream == nullptr)
        return false;

    BASS_ChannelStop(handle);

    if (stream->music != nullptr)
    {
        if (g_CurrentMusic == stream)
            g_CurrentMusic = nullptr;
        Mix_FreeMusic(stream->music);
    }

    if (stream->chunk != nullptr)
        Mix_FreeChunk(stream->chunk);

    delete stream;
    return true;
}
//----------------------------------------------------------------------------------

#endif // !ORION_WINDOWS
