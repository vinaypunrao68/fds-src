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
     *
     * @param tenantId Tenant Identifier
     * @param ipAddress IP Address of the machine to send data
     * @param port Port number to send data
     * @param samplingFrequency Frequency in minutes for sampling data
     * @param duration Samples will be sent every specified duration
     * @param buckets List of buckets for which data will be collected and sent
     * @param dataPointNames List of data-points
     */
    void register(int tenantId, String ipAddress, int port, int samplingFrequency, int duration,
                  Vector<String> buckets, Vector<String> dataPointNames);

    /**
     * Deregister streaming of data for listed buckets.
     * @param tenantId Tenant Identifier
     * @param buckets List of buckets
     */
    void deregister(int tenantId, Vector<String> buckets);
}
