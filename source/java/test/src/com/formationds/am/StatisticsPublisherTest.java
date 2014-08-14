package com.formationds.am;

import com.formationds.apis.ConfigurationService;
import com.formationds.streaming.StreamingRegistrationMsg;
import com.formationds.xdi.CachingConfigurationService;
import com.formationds.xdi.XdiClientFactory;
import com.google.common.collect.Lists;
import org.junit.Test;

import java.util.List;

public class StatisticsPublisherTest {

    @Test
    public void integrationTest() throws Exception {
        XdiClientFactory clientFactory = new XdiClientFactory();
        ConfigurationService.Iface config = new CachingConfigurationService(clientFactory.remoteOmService("localhost", 9090));
        config.registerStream("http://www.google.com", "POST", Lists.newArrayList("foo"), 10, 10);
        List<StreamingRegistrationMsg> streamRegistrations = config.getStreamRegistrations(0);
        System.out.println(streamRegistrations.size());
        System.out.println(streamRegistrations.get(0));
    }

}