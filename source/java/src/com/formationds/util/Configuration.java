package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import joptsimple.OptionParser;
import joptsimple.OptionSet;
import org.apache.log4j.PropertyConfigurator;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.Properties;

public class Configuration {
    Properties properties = new Properties();
    private File fdsRoot;

    public Configuration(String[] commandLineArgs) throws Exception {
        OptionParser parser = new OptionParser();
        parser.accepts("fds-root").withRequiredArg();
        OptionSet options = parser.parse(commandLineArgs);
        if (options.has("fds-root")) {
            fdsRoot = new File((String) options.valueOf("fds-root"));
        } else {
            fdsRoot = new File("/fds");
        }

        if (fdsRoot.exists()) {
            InputStream stream = new FileInputStream(new File(fdsRoot, "etc/app.properties"));
            properties.load(stream);
            PropertyConfigurator.configure(properties);
        }
    }

    public boolean enforceRestAuth() {
        return Boolean.parseBoolean(properties.getProperty("com.formationds.authorization.rest", "false"));
    }

    public String getFdsRoot() {
        return fdsRoot.getAbsolutePath();
    }
}
