package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.streaming.Streaming;
import com.formationds.streaming.StreamingConfiguration;
import com.formationds.streaming.volumeDataPoints;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.util.List;

public class StreamingHandler implements Streaming.Iface {
    private static final Logger LOG = Logger.getLogger(StreamingHandler.class);
    private StreamingConfiguration.Iface configClient;

    public StreamingHandler(StreamingConfiguration.Iface configClient) {
        this.configClient = configClient;
    }

    @Override
    public void publishMetaStream(int registration_id, List<volumeDataPoints> data_points) throws TException {
        LOG.debug("Got " + data_points.size() + " data points for registration id " + registration_id);
    }
}
