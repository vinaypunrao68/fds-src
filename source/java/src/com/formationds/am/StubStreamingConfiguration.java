package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.streaming.StreamingConfiguration;
import com.formationds.streaming.registration;
import org.apache.thrift.TException;

import java.util.List;

public class StubStreamingConfiguration implements StreamingConfiguration.Iface {
    @Override
    public registration registerStream(String url, String http_method, List<String> volume_names, int sample_freq_seconds, int duration_seconds) throws TException {
        return null;
    }

    @Override
    public List<registration> list_registrations(int thrift_sucks) throws TException {
        return null;
    }

    @Override
    public void deregisterStream(int registration_id) throws TException {

    }
}
