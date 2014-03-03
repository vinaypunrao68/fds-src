package com.formationds.util;

import com.formationds.web.om.Main;

import java.io.File;
import java.io.FileFilter;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.ArrayList;
import java.util.Collection;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class Bootstrapper {
    public void start() throws Exception {
        ClassLoader currentLoader = Thread.currentThread().getContextClassLoader();
        Collection<URL> urls = new ArrayList<>();
        File jarDirectory = new File("../lib/java/");
        if (!jarDirectory.exists()) {
            return;
        }
        FileFilter jarFilter = (pathname) -> pathname.getName().endsWith("jar") || pathname.getName().endsWith("jar");
        File[] jarFiles = jarDirectory.listFiles(jarFilter);
        for (int i = 0; i < jarFiles.length; i++) {
            urls.add(jarFiles[i].toURL());
        }
        URLClassLoader newLoader = new URLClassLoader(urls.toArray(new URL[0]), currentLoader);
        Thread.currentThread().setContextClassLoader(newLoader);
        new Main().main();
    }

    public static void main(String[] args) throws Exception {
        new Bootstrapper().start();
    }
}
