/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.xdi;

import com.formationds.apis.*;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.util.async.AsyncRequestStatistics;
import com.formationds.util.async.AsyncResourcePool;
import org.apache.thrift.TException;

import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public interface Xdi {

    String LAST_MODIFIED = "Last-Modified";

    long createVolume(AuthenticationToken token, String domainName, String volumeName, VolumeSettings volumePolicy) throws ApiException, TException;

    void deleteVolume(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException;

    VolumeStatus statVolume(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException;

    VolumeDescriptor volumeConfiguration(AuthenticationToken token, String domainName, String volumeName) throws ApiException, TException;

    List<VolumeDescriptor> listVolumes(AuthenticationToken token, String domainName) throws ApiException, TException;

    List<BlobDescriptor> volumeContents(AuthenticationToken token, String domainName, String volumeName, int count, long offset) throws ApiException, TException;

    BlobDescriptor statBlob(AuthenticationToken token, String domainName, String volumeName, String blobName) throws ApiException, TException;

    InputStream readStream(AuthenticationToken token, String domainName, String volumeName, String blobName) throws Exception;

    InputStream readStream(AuthenticationToken token, String domainName, String volumeName, String blobName, long requestOffset, long requestLength) throws Exception;

    byte[] writeStream(AuthenticationToken token, String domainName, String volumeName, String blobName, InputStream in, Map<String, String> metadata) throws Exception;

    void deleteBlob(AuthenticationToken token, String domainName, String volumeName, String blobName) throws ApiException, TException;

    Authenticator getAuthenticator();

    Authorizer getAuthorizer();

    String getSystemVolumeName(AuthenticationToken token) throws SecurityException;
}
