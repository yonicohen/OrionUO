package uk.co.jmaul.orionuo;

import org.libsdl.app.SDLActivity;

/**
 * Entry point for the Android build.
 *
 * SDLActivity loads the libraries named below and then calls SDL_main in
 * libmain.so. The client's main() in OrionMain.cpp becomes SDL_main
 * automatically: it includes SDL.h, which includes SDL_main.h, which defines
 * main to SDL_main on platforms where SDL owns the entry point.
 *
 * The UO data files are not bundled - they are copyright and about 2.6 GB - so
 * they have to be side-loaded and located at runtime. That lookup is still to
 * be wired up; see docs/ANDROID.md.
 */
public class OrionActivity extends SDLActivity
{
    @Override
    protected String[] getLibraries()
    {
        return new String[] {
            "SDL2",
            "SDL2_mixer",
            "main",
        };
    }
}
