/***********************************************************************************
**
** SoundManager.h
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H
//----------------------------------------------------------------------------------
// Not a file-format struct: it is an in-memory table of track names, so it must
// not be packed. Packing put the 8-byte pointer at unaligned offsets, which the
// arm64 linker rejects when emitting chained fixups.
struct MidiInfoStruct
{
    const char *musicName;
    bool loop;
};

#pragma pack(push, 1)
// Parsed straight out of a RIFF file, so this one really is packed. The fields
// are fixed-width: `unsigned long` is 4 bytes on Windows but 8 on any LP64
// target, which silently shifted every field after chunkSize.
struct WaveHeader
{
    char chunkId[4];
    uint32_t chunkSize;
    char format[4];
    char subChunkId[4];
    uint32_t subChunkSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t bytesPerSecond;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char dataChunkId[4];
    uint32_t dataSize;
    //data;
};
#pragma pack(pop)
static_assert(sizeof(WaveHeader) == 44, "WAV header must stay 44 bytes");
//----------------------------------------------------------------------------------
class CSoundManager
{
public:
    int CurrentMusicIndex = -1;

private:
    static const int MIDI_MUSIC_COUNT = 57;
    static const MidiInfoStruct MidiInfo[MIDI_MUSIC_COUNT];
    HSTREAM m_Music{ NULL };
    HSTREAM m_WarMusic{ NULL };

    void TraceMusicError(DWORD error);
    //std::map<HSTREAM, BYTE*> streams;
public:
    CSoundManager();
    ~CSoundManager();

    bool Init();
    void Free();
    void ResumeSound();
    void PauseSound();

    //Mix_Chunk *LoadSoundEffect(TIndexSound &is);

    bool FreeStream(HSTREAM hSteam);

    bool IsPlayingNormalMusic();

    //Метод расчета звука. При расчете учитываются: звук клиента, дистанция для эффектов.
    float GetVolumeValue(int distance = -1, bool music = false);

    HSTREAM LoadSoundEffect(CIndexSound &is);

    UCHAR_LIST CreateWaveFile(CIndexSound &is);

    //void PlaySoundEffect(Mix_Chunk *mix, int volume);
    void PlaySoundEffect(HSTREAM stream, float volume);

    void PlayMidi(int index, bool warmode);

    void PlayMP3(const os_path &fileName, int index, bool loop, bool warmode = false);

    void StopMusic();

    void StopWarMusic();

    void SetMusicVolume(float volume);
};
//----------------------------------------------------------------------------------
extern CSoundManager g_SoundManager;
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------
