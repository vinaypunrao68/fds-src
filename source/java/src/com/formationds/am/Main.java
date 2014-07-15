package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.fdsp.LegacyClientFactory;
import com.formationds.security.Authenticator;
import com.formationds.security.JaasAuthenticator;
import com.formationds.spike.nbd.*;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParsedConfig;
import com.formationds.xdi.CachingConfigurationService;
import com.formationds.xdi.FakeAmService;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.XdiClientFactory;
import com.formationds.xdi.s3.S3Endpoint;
import com.formationds.xdi.swift.SwiftEndpoint;
import org.apache.log4j.Logger;
import org.apache.thrift.server.TServer;
import org.apache.thrift.server.TThreadPoolServer;
import org.apache.thrift.transport.TServerSocket;

import javax.xml.parsers.ParserConfigurationException;
import java.util.concurrent.ForkJoinPool;

class FakeAmServer {
    public static void main(String[] args) throws Exception {
        AmService.Processor<AmService.Iface> processor = new AmService.Processor<>(new FakeAmService());
        TServerSocket transport = new TServerSocket(4242);
        TServer server = new TThreadPoolServer(new TThreadPoolServer.Args(transport).processor(processor));
        server.serve();
    }
}

public class Main {
    private static Logger LOG = Logger.getLogger(Main.class);

    private static boolean defaultBooleanSetting(ParsedConfig config, String key, boolean def) {
        try {
            return config.lookup(key).booleanValue();
        } catch(Exception e) {
            return def;
        }
    }

    public static void main(String[] args) throws Exception {
        Configuration configuration = new Configuration("xdi", args);
        try {
            ParsedConfig amParsedConfig = configuration.getPlatformConfig();
            NativeAm.startAm(args);
            Thread.sleep(200);

            XdiClientFactory clientFactory = new XdiClientFactory();
            LegacyClientFactory legacyClientFactory = new LegacyClientFactory();

            boolean useFakeAm = amParsedConfig.lookup("fds.am.memory_backend").booleanValue();
            String omHost = amParsedConfig.lookup("fds.am.om_ip").stringValue();
            int omConfigPort = 9090;
            int omLegacyConfigPort = amParsedConfig.lookup("fds.am.om_config_port").intValue();

            AmService.Iface am = useFakeAm ? new FakeAmService() :
                    clientFactory.remoteAmService("localhost", 9988);
                    //clientFactory.remoteAmService("localhost", 4242);

            ConfigurationService.Iface config = new CachingConfigurationService(clientFactory.remoteOmService(omHost, omConfigPort));

            int nbdPort = amParsedConfig.lookup("fds.am.nbd_server_port").intValue();
            boolean nbdLoggingEnabled =  defaultBooleanSetting(amParsedConfig, "fds.am.enable_nbd_log", false);
            boolean nbdBlockExclusionEnabled = defaultBooleanSetting(amParsedConfig, "fds.am.enable_nbd_block_exclusion", true);
            ForkJoinPool fjp = new ForkJoinPool(50);
            NbdServerOperations ops = new FdsServerOperations(am, config, fjp);
            if(nbdLoggingEnabled)
                ops = new LoggingOperationsWrapper(ops, "/fds/var/logs/nbd");
            if(nbdBlockExclusionEnabled)
                ops = new BlockExclusionWrapper(ops, 4096);
            NbdHost nbdHost = new NbdHost(nbdPort, ops);
            
            new Thread(() -> nbdHost.run()).start();

            Authenticator authenticator = new JaasAuthenticator();
            Xdi xdi = new Xdi(am, config, authenticator, legacyClientFactory.configPathClient(omHost, omLegacyConfigPort));

            boolean enforceAuthorization = amParsedConfig.lookup("fds.am.enforce_authorization").booleanValue();
            int s3Port = amParsedConfig.lookup("fds.am.s3_port").intValue();
            new Thread(() -> new S3Endpoint(xdi, enforceAuthorization).start(s3Port)).start();

            int swiftPort = amParsedConfig.lookup("fds.am.swift_port").intValue();
            new SwiftEndpoint(xdi, enforceAuthorization).start(swiftPort);
        } catch (Throwable throwable) {
            LOG.fatal("Error starting AM", throwable);
            System.out.println(throwable.getMessage());
            System.out.flush();
            System.exit(-1);
        }
    }

}

