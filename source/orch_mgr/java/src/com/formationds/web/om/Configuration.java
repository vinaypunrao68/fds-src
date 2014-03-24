package com.formationds.web.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.log4j.PropertyConfigurator;

import java.io.InputStream;
import java.net.URL;
import java.util.Properties;

class Configuration {
    Properties properties = new Properties();

    public Configuration() throws Exception {
        String configPath = System.getProperty("fds.config");
        InputStream stream = new URL(configPath).openConnection().getInputStream();
        properties.load(stream);
        PropertyConfigurator.configure(properties);
    }

    public boolean enforceRestAuth() {
        return Boolean.parseBoolean(properties.getProperty("com.formationds.authorization.rest", "false"));
    }
}
