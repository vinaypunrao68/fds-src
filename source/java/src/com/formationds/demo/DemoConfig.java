package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.auth.BasicAWSCredentials;

public class DemoConfig {
    private String amHost;
    private int swiftPort;
    private int s3Port;
    private BasicAWSCredentials creds;
    private String[] volumeNames;

    public DemoConfig(String amHost, int swiftPort, int s3Port, BasicAWSCredentials creds, String[] volumeNames) {
        this.amHost = amHost;
        this.swiftPort = swiftPort;
        this.s3Port = s3Port;
        this.creds = creds;
        this.volumeNames = volumeNames;
    }

    public String[] getVolumeNames() {
        return volumeNames;
    }

    public String getAmHost() {
        return amHost;
    }

    public int getSwiftPort() {
        return swiftPort;
    }

    public int getS3Port() {
        return s3Port;
    }

    public BasicAWSCredentials getCreds() {
        return creds;
    }
}
