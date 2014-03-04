package com.formationds.util;

import com.formationds.web.om.Main;

import java.io.*;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class Bootstrapper {
    public static native void printf(final String s);

    public class NativeWriter extends Writer {
        @Override
        public void write(char[] cbuf, int off, int len) throws IOException {
            Bootstrapper.printf(new String(cbuf, off, len));
        }

        @Override
        public void flush() throws IOException {
        }

        @Override
        public void close() throws IOException {
        }
    }

    public void start() throws Exception {
        try {
            String gluePath = new File("../lib/libfds-om-glue.0.0.0.0.so").getAbsolutePath();
            System.load(gluePath);

            if (! loadAllJars()) return;

            new Main().main();
        } catch (Exception e) {
            e.printStackTrace(new PrintWriter(new NativeWriter()));
        }
    }

    private boolean loadAllJars() throws IOException {
        printf("[debug] JVM bootstrapper loaded");
        HackClassLoader loader = new HackClassLoader();

        loader.addFile("../lib/java/classes");

        File jarDirectory = new File("../lib/java/");
        if (!jarDirectory.exists()) {
            printf("[error] Cannot find jar libraries, exiting");
            return false;
        }
        FileFilter jarFilter = (pathname) -> pathname.getName().endsWith("jar") || pathname.getName().endsWith("jar");
        File[] jarFiles = jarDirectory.listFiles(jarFilter);
        for (int i = 0; i < jarFiles.length; i++) {
            loader.addFile(jarFiles[i]);
            printf("[debug] loading " + jarFiles[i].getName());
        }
        return true;
    }

    public static void main(String[] args) throws Exception {
        new Bootstrapper().start();
    }
}
