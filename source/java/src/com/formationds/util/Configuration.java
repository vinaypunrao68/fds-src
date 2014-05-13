package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.util.libconfig.ParserFacade;
import joptsimple.OptionParser;
import joptsimple.OptionSet;
import org.apache.log4j.PropertyConfigurator;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Properties;

public class Configuration {
    Properties properties = new Properties();
    private File fdsRoot;

    public Configuration(String[] commandLineArgs) throws Exception {
        OptionParser parser = new OptionParser();
        parser.allowsUnrecognizedOptions();
        parser.accepts("fds-root").withRequiredArg();
        OptionSet options = parser.parse(commandLineArgs);
        if (options.has("fds-root")) {
            fdsRoot = new File((String) options.valueOf("fds-root"));
        } else {
            fdsRoot = new File("/fds");
        }
        initLog4J();
        initJaas(fdsRoot);
    }

    private void initJaas(File fdsRoot) throws Exception {
        Path jaasConfig = Paths.get(fdsRoot.getCanonicalPath(), "etc", "auth.conf");
        URL configUrl = jaasConfig.toFile().toURL();
        System.setProperty("java.security.auth.login.config", configUrl.toExternalForm());
    }

    private void initLog4J() {
        properties.put("log4j.rootCategory", "DEBUG, console");
        properties.put("log4j.appender.console", "org.apache.log4j.ConsoleAppender");
        properties.put("log4j.appender.console.layout", "org.apache.log4j.PatternLayout");
        properties.put("log4j.appender.console.layout.ConversionPattern", "%-4r [%t] %-5p %c %x - %m%n");
        properties.put("log4j.logger.org.hibernate", "WARN");
        properties.put("log4j.logger.com.mchange", "WARN");
        properties.put("log4j.logger.com.formationds", "DEBUG");
        properties.put("log4j.logger.org.jetty", "INFO");
        properties.put("log4j.logger.org.apache.thrift", "DEBUG");
        PropertyConfigurator.configure(properties);
    }

    public boolean enforceRestAuth() {
        return getOmConfig().lookup("fds.om.enforce_rest_auth").booleanValue();
    }

    public String getFdsRoot() {
        return fdsRoot.getAbsolutePath();
    }

    public ParserFacade getPlatformConfig() {
        Path path = Paths.get(getFdsRoot(), "etc", "platform.conf");
        return getParserFacade(path);
    }

    public ParserFacade getOmConfig() {
        Path path = Paths.get(getFdsRoot(), "etc", "orch_mgr.conf");
        return getParserFacade(path);
    }

    private ParserFacade getParserFacade(Path path) {
        try {
            return new ParserFacade(Files.newInputStream(path));
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}
