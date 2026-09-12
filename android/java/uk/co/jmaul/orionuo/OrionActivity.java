package uk.co.jmaul.orionuo;

import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.WindowInsets;

import org.libsdl.app.SDLActivity;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Entry point for the Android build.
 *
 * SDLActivity loads the libraries named below and then calls SDL_main in
 * libmain.so. The client's main() in OrionMain.cpp becomes SDL_main
 * automatically: it includes SDL.h, which includes SDL_main.h, which defines
 * main to SDL_main on platforms where SDL owns the entry point.
 *
 * The UO data files are not bundled - they are copyright and about 2.6 GB - so
 * they are side-loaded and located at runtime; see docs/ANDROID.md.
 */
public class OrionActivity extends SDLActivity
{
    /** One argument per line, in the same external directory as the UO data. */
    private static final String ARGS_FILE = "orion_args.txt";

    @Override
    protected String[] getLibraries()
    {
        return new String[] {
            "SDL2",
            "SDL2_mixer",
            "main",
        };
    }

    /**
     * The client takes its shard address from the command line (-login
     * host,port), and there is no command line on Android. Two ways to supply
     * one, in order of precedence:
     *
     *   adb shell am start -n uk.co.jmaul.orionuo/.OrionActivity \
     *       --esa args "-login uo.jmaul.co.uk,2593"
     *
     * or a plain text file next to the UO data, one argument per line, which is
     * what a device without adb can use:
     *
     *   <external files>/orion_args.txt
     *
     * Without either the client falls back to login.cfg, which in a stock UO
     * install points at 127.0.0.1 and cannot connect to anything.
     */
    @Override
    protected String[] getArguments()
    {
        Bundle extras = getIntent() != null ? getIntent().getExtras() : null;
        if (extras != null)
        {
            String[] fromIntent = extras.getStringArray("args");
            if (fromIntent != null && fromIntent.length > 0)
                return fromIntent;
        }

        List<String> fromFile = readArgumentsFile();
        if (!fromFile.isEmpty())
            return fromFile.toArray(new String[0]);

        return super.getArguments();
    }

    /**
     * Lets the soft keyboard have somewhere to draw.
     *
     * SDL puts the activity in sticky immersive fullscreen. On Android 16 with
     * edge-to-edge enforced the IME then never gets a surface - the window
     * manager reports the input method window as GONE with NO_SURFACE while the
     * input method service believes it is showing, so asking for the keyboard
     * appears to work and nothing comes up. Leaving immersive mode while text
     * is being entered gives it room; going back afterwards restores the
     * fullscreen game view.
     */
    public static void setImmersiveMode(final boolean immersive)
    {
        if (mSingleton == null)
            return;

        mSingleton.runOnUiThread(new Runnable()
        {
            @Override
            public void run()
            {
                try
                {
                    View decor = mSingleton.getWindow().getDecorView();
                    int flags = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN;

                    if (immersive)
                    {
                        flags |= View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
                    }

                    decor.setSystemUiVisibility(flags);
                }
                catch (Exception e)
                {
                    android.util.Log.w("OrionUO", "could not change immersive mode: " + e);
                }
            }
        });
    }

    /**
     * Height in pixels of the soft keyboard, or 0 when it is down.
     *
     * SDL draws into a SurfaceView that keeps the full window, so the keyboard
     * simply covers the bottom of the rendered frame and nothing native ever
     * hears about it - which left the login panel, which sits at the bottom of
     * the 640x480 pre-game artwork, permanently underneath it. The renderer
     * asks for this and scales that artwork into what is left.
     */
    public static int getSoftKeyboardHeight()
    {
        try
        {
            View root = mSingleton.getWindow().getDecorView();

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R)
            {
                WindowInsets insets = root.getRootWindowInsets();
                if (insets != null)
                    return insets.getInsets(WindowInsets.Type.ime()).bottom;
            }

            // Before API 30 there is no ime() inset, so infer it from how much
            // of the window is still visible. A small difference is the status
            // or navigation bar rather than a keyboard.
            Rect visible = new Rect();
            root.getWindowVisibleDisplayFrame(visible);
            int covered = root.getHeight() - visible.bottom;
            return (covered > root.getHeight() / 5) ? covered : 0;
        }
        catch (Exception e)
        {
            return 0;
        }
    }

    private List<String> readArgumentsFile()
    {
        List<String> args = new ArrayList<String>();

        File dir = getExternalFilesDir(null);
        if (dir == null)
            return args;

        File file = new File(dir, ARGS_FILE);
        if (!file.isFile())
            return args;

        BufferedReader reader = null;
        try
        {
            reader = new BufferedReader(new FileReader(file));
            String line;
            while ((line = reader.readLine()) != null)
            {
                line = line.trim();
                // '#' and ';' are both comment markers in the UO config files
                // this sits beside, so accept either here too.
                if (line.isEmpty() || line.startsWith("#") || line.startsWith(";"))
                    continue;
                args.add(line);
            }
        }
        catch (Exception e)
        {
            android.util.Log.w("OrionUO", "could not read " + file + ": " + e);
            return Collections.emptyList();
        }
        finally
        {
            try { if (reader != null) reader.close(); } catch (Exception ignored) {}
        }

        return args;
    }
}
