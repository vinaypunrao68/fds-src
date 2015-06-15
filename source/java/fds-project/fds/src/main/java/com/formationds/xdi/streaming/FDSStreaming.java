package com.formationds.xdi.streaming;

import java.util.Vector;

/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

/**
 * Streaming APIs for FDS data.
 */
public interface FDSStreaming {
    /**
     * Subscribes for streaming of data listed by names. The data is sent per bucket.
     * TODO(xxx) later add tenantId and dataPointNames to parameters
     * @param ipAddress IP Address of the machine to send data
     * @param port Port number to send data
     * @param samplingFrequency Frequency in seconds for sampling data. Currently valid
     * values are 60 (1 min) and 3600 (1 hour)
     * @param duration Samples will be sent every specified duration
     * @param buckets List of buckets for which data will be collected and sent
     */
    void register(String ipAddress, int port, int samplingFrequency, int duration,
                  Vector<String> buckets);

    /**
     * Deregister streaming of data for listed buckets.
     * @param tenantId Tenant Identifier
     * @param buckets List of buckets
     */
    void deregister(Vector<String> buckets);
}
