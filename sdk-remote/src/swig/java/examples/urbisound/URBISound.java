/*
 * Copyright (C) 2010-2011, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */

package examples.urbisound;

import java.io.IOException;

import urbi.*;
import urbi.*;

public class    URBISound
{
    static {
        System.loadLibrary("urbijava");
    }

    static public UClient   robotC = null;

    static public SoundSampler      soundSampler = null;

    public static void main(String[] args)
    {
        if (args.length != 1)
        {
            System.err.println("Usage: urbisound robot");
            System.exit(1);
        }
        robotC = new UClient(args[0]);

        CallSound   sound = new CallSound();
        robotC.setCallback(sound, "sound");

        robotC.send("sound<< ping;");
        soundSampler = new SoundSampler();
        soundSampler.setAction(sound);

        urbi.execute ();
    }
}
