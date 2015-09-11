package com.formationds.spike.iscsi;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.RealAsyncAm;
import com.formationds.xdi.XdiClientFactory;
import org.jscsi.target.Configuration;
import org.jscsi.target.Target;
import org.jscsi.target.TargetServer;

import java.util.ArrayList;
import java.util.List;


public class JScsiServer {

    private final AsyncAm am;
    private final List<VolumeDescriptor> volumeDescriptors;
    private final Object init_lock = new Object();
    private Thread serverThread;
    private TargetServer server;

    public static final String ISCSI_DOMAIN = "iscsi";

    public JScsiServer(AsyncAm am, List<VolumeDescriptor> volumeDescriptors) {
        this.am = am;
        this.volumeDescriptors = volumeDescriptors;
    }

    public void start() {
        synchronized (init_lock) {
            if(serverThread != null)
                return; // log?

            JScsiConfigurationBuilder builder = new JScsiConfigurationBuilder();
            builder.withAddress("127.0.0.1").withSloppyNegotiation(true);
            for (VolumeDescriptor vd : volumeDescriptors)
                builder.withTarget(new Target(vd.name, vd.name, new FdsStorageModule(am, ISCSI_DOMAIN, vd)));

            Configuration config = builder.build();

            server = new TargetServer(config);

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
        int platformPort = 7000;
        int amOffset = 1899;

        XdiClientFactory cf = new XdiClientFactory();
        ConfigurationService.Iface om = cf.remoteOmService("localhost", 9090);

        RealAsyncAm am = new RealAsyncAm("localhost", platformPort + amOffset, 10383);
        am.start();

        List<VolumeDescriptor> descriptors = new ArrayList<>();
        for(VolumeDescriptor descriptor : om.listVolumes(ISCSI_DOMAIN)) {
            if(descriptor.getPolicy().isSetBlockDeviceSizeInBytes() && descriptor.getPolicy().blockDeviceSizeInBytes > 0) {
                System.out.println("Found volume: " + descriptor.name);
                descriptors.add(descriptor);
            }
        }

        JScsiServer server = new JScsiServer(am, descriptors);

        server.start();
        System.out.println("Server started - press any key to exit");
        System.in.read();
        server.stop();
    }
}