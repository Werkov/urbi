/*
 * Copyright (C) 2010-2011, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */

package examples.test;

import urbi.*;
import urbi.*;

public class test {
    static {
        System.loadLibrary("urbijava");
    }

    public static void main(String argv[]) {
        System.out.println("Connexion to the URBI server...");
        //UClient a = new UClient("localhost");
        //UClient a = new UClient("localhost");
        UClient a = new UClient("localhost");

        if (a.error() != 0)
        {
            System.out.println("Couldn't connect to the URBI server.");
            return;
        }

        testcall c = new testcall ();
        a.setCallback (c, "tag");

        a.send("+connection(\"all\")<< 12;");

        a.send("tag<<\"hello\";");
        a.send("tag<<42;");
        a.send("tag<<[10, 24, \"biz\"];");
        a.send("tag+connection(\"all\")<<bizz;");
        a.send("var toto = bin 2;ab");
        a.send("tag<<toto;");
        a.send("camera.format = 1;loop tag<<camera.val;");

        try
        {
            Thread.sleep(200000);
        }
        catch (InterruptedException e)
        {
            System.out.println ("Done error");
            System.exit(0);
        }
        System.out.println ("Done");
        System.exit(0);
    }
}

