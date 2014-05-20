package com.formationds.util;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.util.libconfig.ParsedConfig;
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

    public Configuration(String commandName, String[] commandLineArgs) throws Exception {
        OptionParser parser = new OptionParser();
        parser.allowsUnrecognizedOptions();
        parser.accepts("fds-root").withRequiredArg();
        OptionSet options = parser.parse(commandLineArgs);
        if (options.has("fds-root")) {
            fdsRoot = new File((String) options.valueOf("fds-root"));
        } else {
            fdsRoot = new File("/fds");
        }
        initLog4J(commandName, fdsRoot);
        initJaas(fdsRoot);
    }

    private void initJaas(File fdsRoot) throws Exception {
        Path jaasConfig = Paths.get(fdsRoot.getCanonicalPath(), "etc", "auth.conf");
        URL configUrl = jaasConfig.toFile().toURL();
        System.setProperty("java.security.auth.login.config", configUrl.toExternalForm());
    }

    private void initLog4J(String commandName, File fdsRoot) {
        Path logPath = Paths.get(fdsRoot.getAbsolutePath(), "var", "logs", commandName + ".log").toAbsolutePath();
        properties.put("log4j.rootLogger", "INFO, rolling");
        properties.put("log4j.appender.rolling", "org.apache.log4j.RollingFileAppender");
        properties.put("log4j.appender.rolling.File", logPath.toString());
        properties.put("log4j.appender.rolling.MaxFileSize", "5120KB");
        properties.put("log4j.appender.rolling.MaxBackupIndex", "10");
        properties.put("log4j.appender.rolling.layout", "org.apache.log4j.PatternLayout");
        properties.put("log4j.appender.rolling.layout.ConversionPattern", "[%t] %-5p %l - %m%n");
        properties.put("log4j.appender.rolling.layout.ConversionPattern", "%d{ISO8601} - %p %c - %m%n");
        properties.put("log4j.logger.org.hibernate", "WARN");
        properties.put("log4j.logger.com.mchange", "WARN");
        properties.put("log4j.logger.org.jetty", "INFO");
        properties.put("log4j.logger.org.apache.thrift", "WARN");
        properties.put("log4j.logger.com.formationds", "DEBUG");
        PropertyConfigurator.configure(properties);
    }

    public boolean enforceRestAuth() {
        return getOmConfig().lookup("fds.om.enforce_rest_auth").booleanValue();
    }

    public String getFdsRoot() {
        return fdsRoot.getAbsolutePath();
    }

    public ParsedConfig getPlatformConfig() {
        Path path = Paths.get(getFdsRoot(), "etc", "platform.conf");
        return getParserFacade(path);
    }

    public ParsedConfig getOmConfig() {
        Path path = Paths.get(getFdsRoot(), "etc", "orch_mgr.conf");
        return getParserFacade(path);
    }

    public ParsedConfig getDemoConfig() {
        Path path = Paths.get(getFdsRoot(), "etc", "demo.conf");
        return getParserFacade(path);
    }

    private ParsedConfig getParserFacade(Path path) {
        try {
            return new ParsedConfig(Files.newInputStream(path));
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}
