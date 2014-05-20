package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.*;
import com.formationds.security.Authenticator;
import com.formationds.security.AuthorizationToken;
import com.sun.security.auth.UserPrincipal;
import org.apache.thrift.TException;
import org.joda.time.DateTime;

import javax.security.auth.login.LoginException;
import java.io.InputStream;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class Xdi {
    private final AmService.Iface am;
    private ConfigurationService.Iface config;

    public static final String LAST_MODIFIED = "Last-Modified";
    private Authenticator authenticator;

    public Xdi(AmService.Iface am, ConfigurationService.Iface config, Authenticator authenticator) {
        this.am = am;
        this.config = config;
        this.authenticator = authenticator;
    }

    public XdiCredentials authenticate(String login, String password) throws LoginException {
        authenticator.login(login, password);
        UserPrincipal principal = new UserPrincipal(login);
        AuthorizationToken token = new AuthorizationToken(Authenticator.KEY, principal);
        return new XdiCredentials(principal, token.getKey());
    }

    public void createVolume(String domainName, String volumeName, VolumePolicy volumePolicy) throws ApiException, TException {
        config.createVolume(domainName, volumeName, volumePolicy);
    }

    public void deleteVolume(String domainName, String volumeName) throws ApiException, TException {
        config.deleteVolume(domainName, volumeName);
    }

    public VolumeStatus statVolume(String domainName, String volumeName) throws ApiException, TException {
        return am.volumeStatus(domainName, volumeName);
    }

    public VolumeDescriptor volumeConfiguration(String domainName, String volumeName) throws ApiException, TException {
        return config.statVolume(domainName, volumeName);
    }

    public List<VolumeDescriptor> listVolumes(String domainName) throws ApiException, TException {
        return config.listVolumes(domainName);
    }

    public List<BlobDescriptor> volumeContents(String domainName, String volumeName, int count, long offset) throws ApiException, TException {
        return am.volumeContents(domainName, volumeName, count, offset);
    }

    public BlobDescriptor statBlob(String domainName, String volumeName, String blobName) throws ApiException, TException {
        return am.statBlob(domainName, volumeName, blobName);
    }

    public InputStream readStream(String domainName, String volumeName, String blobName) throws Exception {
        Iterator<byte[]> iterator = new FdsObjectIterator(am, config).read(domainName, volumeName, blobName);
        return new FdsObjectStreamer(iterator);
    }

    public InputStream readStream(String domainName, String volumeName, String blobName, long requestOffset, long requestLength) throws Exception {
        Iterator<byte[]> iterator = new FdsObjectIterator(am, config).read(domainName, volumeName, blobName, requestOffset, requestLength);
        return new FdsObjectStreamer(iterator);
    }


    public byte[] writeStream(String domainName, String volumeName, String blobName, InputStream in, Map<String, String> metadata) throws Exception {
        VolumeDescriptor volume = config.statVolume(domainName, volumeName);
        int bufSize = volume.getPolicy().getMaxObjectSizeInBytes();
        metadata.putIfAbsent(LAST_MODIFIED, Long.toString(DateTime.now().getMillis()));
        return new StreamWriter(bufSize, am).write(domainName, volumeName, blobName, in, metadata);
    }

    public void deleteBlob(String domainName, String volumeName, String blobName) throws ApiException, TException {
        am.deleteBlob(domainName, volumeName, blobName);
    }
}
