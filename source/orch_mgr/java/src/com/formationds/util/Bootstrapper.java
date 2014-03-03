package com.formationds.util;

import com.formationds.web.om.Main;

import java.io.*;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.ArrayList;
import java.util.Collection;

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
        String gluePath = new File("../lib/libfds-om-glue.0.0.0.0.so").getAbsolutePath();
        System.load(gluePath);
        printf("[debug] JVM bootstrapper loaded");
        ClassLoader currentLoader = Thread.currentThread().getContextClassLoader();
        Collection<URL> urls = new ArrayList<>();
        urls.add(new File("../lib/java/classes").toURL());

        try {
            File jarDirectory = new File("../lib/java/");
            if (!jarDirectory.exists()) {
                return;
            }
            FileFilter jarFilter = (pathname) -> pathname.getName().endsWith("jar") || pathname.getName().endsWith("jar");
            File[] jarFiles = jarDirectory.listFiles(jarFilter);
            for (int i = 0; i < jarFiles.length; i++) {
                urls.add(jarFiles[i].toURL());
                printf("[debug] loading " + jarFiles[i].getName());
            }
            URLClassLoader newLoader = new URLClassLoader(urls.toArray(new URL[0]), currentLoader);
            Thread.currentThread().setContextClassLoader(newLoader);
            new Main().main();
        } catch (Exception e) {
            e.printStackTrace(new PrintWriter(new NativeWriter()));
        }
    }

    public static void main(String[] args) throws Exception {
        new Bootstrapper().start();
    }
}
