package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.streaming.Streaming;
import com.formationds.streaming.StreamingConfiguration;
import com.formationds.streaming.StreamingRegistrationMsg;
import com.formationds.streaming.volumeDataPoints;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.util.List;

public class StatisticsPublisher implements Streaming.Iface {
    private static final Logger LOG = Logger.getLogger(StatisticsPublisher.class);
    private StreamingConfiguration.Iface configClient;

    public StatisticsPublisher(StreamingConfiguration.Iface configClient) {
        this.configClient = configClient;
    }

    @Override
    public void publishMetaStream(int registrationId, List<volumeDataPoints> dataPoints) throws TException {
        configClient.list_registrations(0).stream()
                .filter(r -> r.getId() == registrationId)
                .forEach(r -> {
                    publish(r, dataPoints);
                });
    }

    private void publish(StreamingRegistrationMsg registrationMsg, List<volumeDataPoints> dataPoints) {

    }

}


