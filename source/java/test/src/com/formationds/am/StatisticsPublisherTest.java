package com.formationds.am;

import com.formationds.streaming.DataPointPair;
import com.formationds.streaming.Streaming;
import com.formationds.streaming.volumeDataPoints;
import com.google.common.collect.Lists;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TSocket;
import org.junit.Test;

public class StatisticsPublisherTest {

    @Test
    public void makeJunitHappy() {

    }

    public void integrationTest() throws Exception {
//        XdiClientFactory clientFactory = new XdiClientFactory();
//        ConfigurationService.Iface config = new CachingConfigurationService(clientFactory.remoteOmService("localhost", 9090));
//        config.registerStream("http://www.google.com", "POST", Lists.newArrayList("foo"), 10, 10);
//        List<StreamingRegistrationMsg> streamRegistrations = config.getStreamRegistrations(0);
//        System.out.println(streamRegistrations.size());
//        System.out.println(streamRegistrations.get(0));
//
        int pmPort = 7000;
        int streamingPortOffset = 1999;    // can get from platform.conf: fds.am.streaming_port_offset
        TSocket transport = new TSocket("localhost", pmPort + streamingPortOffset);
        transport.open();
        Streaming.Client client = new Streaming.Client(new TBinaryProtocol(new TFramedTransport(transport)));
        client.publishMetaStream(1, Lists.newArrayList(new volumeDataPoints("foo", 42, Lists.newArrayList(new DataPointPair("foo", 42)))));
    }

}
