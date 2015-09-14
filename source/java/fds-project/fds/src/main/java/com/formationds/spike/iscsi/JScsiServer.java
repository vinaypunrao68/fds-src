package com.formationds.spike.iscsi;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.util.HostAndPort;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.RealAsyncAm;
import com.formationds.xdi.XdiClientFactory;
import joptsimple.ArgumentAcceptingOptionSpec;
import joptsimple.OptionParser;
import joptsimple.OptionSet;
import org.jscsi.target.Configuration;
import org.jscsi.target.Target;
import org.jscsi.target.TargetServer;
import org.jscsi.target.storage.IStorageModule;

import java.util.*;


public class JScsiServer {

    private final Object init_lock = new Object();
    private Thread serverThread;
    private TargetServer server;
    private Configuration configuration;

    public static final String ISCSI_DOMAIN = "iscsi";

    public JScsiServer(List<Target> targets) {
        JScsiConfigurationBuilder builder = new JScsiConfigurationBuilder();
        builder.withAddress("127.0.0.1").withSloppyNegotiation(true);
        for(Target target : targets)
            builder.withTarget(target);

        configuration = builder.build();
    }

    public void start() {
        synchronized (init_lock) {
            if(serverThread != null)
                return; // log?

            server = new TargetServer(configuration);

            serverThread = new Thread(() -> {
                try {
                    server.call();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            });

            serverThread.start();
        }
    }

    public void stop() {
        synchronized (init_lock) {
            if(serverThread == null)
                return;

            server.stop();
            try {
                serverThread.join();
            } catch (InterruptedException e) {
                // do nothing
            }
            serverThread = null;
        }
    }

    public static void main(String[] args) throws Exception {

        HostAndPort amAddr = null;
        HostAndPort omAddr = null;
        boolean useExisting = false;
        Map<String, Integer> memoryTargets = new HashMap<>();

        try {
            OptionParser parser = new OptionParser();
            parser.accepts("useexisting");
            ArgumentAcceptingOptionSpec<String> amArg = parser.accepts("am").requiredIf("useexisting").withRequiredArg().ofType(String.class);
            ArgumentAcceptingOptionSpec<String> omArg = parser.accepts("om").requiredIf("useexisting").withRequiredArg().ofType(String.class);
            ArgumentAcceptingOptionSpec<String> mTargetArg = parser.accepts("mtarget").withRequiredArg().ofType(String.class);

            OptionSet parse;
            parse = parser.parse(args);

            try {
                if(parse.has(amArg))
                    amAddr = HostAndPort.parseWithDefaultPort(parse.valueOf(amArg), 8899);
            } catch(Exception ex) {
                throw new Exception("Could not parse am argument - expecting HOST:PORT");
            }

            try {
                if(parse.has(omArg))
                    omAddr = HostAndPort.parseWithDefaultPort(parse.valueOf(omArg), 9090);
            } catch(Exception ex) {
                throw new Exception("Could not parse om argument - expecting HOST:PORT");
            }

            useExisting = parse.has("useexisting");

            for(String mtarget : parse.valuesOf(mTargetArg)) {
                try {
                    String[] parts = mtarget.split(",");
                    memoryTargets.put(parts[0], Integer.parseInt(parts[1]));
                } catch(Exception ex) {
                    throw new Exception("could not parse mtarget argument '" + mtarget + "' - expecting NAME,SIZE");
                }
            }
        } catch(Exception ex) {
            printUsageAndExit(ex.getMessage());
            return;
        }

        XdiClientFactory cf = new XdiClientFactory();
        List<Target> targets = new ArrayList<>();

        if(useExisting && omAddr != null && amAddr != null) {
            RealAsyncAm am = new RealAsyncAm(amAddr.getHost(), amAddr.getPort(), 10383);
            am.start();
            ConfigurationService.Iface om = cf.remoteOmService(omAddr.getHost(), omAddr.getPort());
            for(VolumeDescriptor descriptor : om.listVolumes(ISCSI_DOMAIN)) {
                if(descriptor.getPolicy().isSetBlockDeviceSizeInBytes() && descriptor.getPolicy().blockDeviceSizeInBytes > 0) {
                    System.out.println("Found volume: " + descriptor.name);
                    targets.add(fdsTarget(am, descriptor));
                }
            }
        }

        for(Map.Entry<String, Integer> memoryTarget : memoryTargets.entrySet()) {
            System.out.println("Memory target: " + memoryTarget.getKey());
            targets.add(memoryTarget(memoryTarget.getKey(), memoryTarget.getValue()));
        }

        if(targets.isEmpty()) {
            System.err.println("No valid targets found");
            System.exit(2);
        }

        JScsiServer server = new JScsiServer(targets);

        server.start();
        System.out.println("Server started - press any key to exit");
        System.in.read();
        server.stop();
    }

    private static Target fdsTarget(AsyncAm am, VolumeDescriptor descriptor) {
        return new Target(descriptor.name, descriptor.name, new FdsStorageModule(am, ISCSI_DOMAIN, descriptor));
    }

    private static Target memoryTarget(String name, int size) {
        return new Target(name, name, new MemoryStorageModule(1 + (size / IStorageModule.VIRTUAL_BLOCK_SIZE)));
    }

    private static void printUsageAndExit(String msg) {
        if(msg != null) {
            System.err.println(msg);
            System.err.println();
        }
        System.err.println("Arguments:");
        System.err.println("  --am=HOST:PORT       am address to use");
        System.err.println("  --om=HOST:PORT       om address to use");
        System.err.println("  --useexisting        create targets for existing block volumes, requires --om and --am arguments");
        System.err.println("  --mtarget=NAME,SIZE  create a memory backed target with the name NAME with size SIZE");
        System.err.println();
        System.exit(1);
    }
}