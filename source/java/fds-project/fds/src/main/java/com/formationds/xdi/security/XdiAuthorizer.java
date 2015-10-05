package com.formationds.xdi.security;

import com.formationds.apis.*;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.util.blob.Mode;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.BlobWithMetadata;
import com.formationds.xdi.XdiConfigurationApi;
import com.formationds.xdi.s3.S3Endpoint;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import com.google.common.collect.Maps;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import javax.security.auth.login.LoginException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

public class XdiAuthorizer {
    private static final Logger LOG = Logger.getLogger(XdiAuthorizer.class);
    private Authenticator authenticator;
    private Authorizer authorizer;
    private final Cache<String, Map<String, String>> volumeMetadataCache;
    private AsyncAm asyncAm;

    public XdiAuthorizer(Authenticator authenticator, Authorizer authorizer, AsyncAm asyncAm) {
        this.authenticator = authenticator;
        this.authorizer = authorizer;
        this.asyncAm = asyncAm;
        volumeMetadataCache = CacheBuilder.newBuilder()
                .expireAfterWrite(1, TimeUnit.MINUTES)
                .build();
    }

    public static BlobDescriptor withAcl(BlobDescriptor blobDescriptor, XdiAcl.Value acl) {
        Map<String, String> metadata = blobDescriptor.getMetadata();
        metadata.put(XdiAcl.X_AMZ_ACL, acl.getCommonName());
        return blobDescriptor;
    }

    public void updateBlobAcl(AuthenticationToken token, String bucketName, String objectName, XdiAcl.Value newAcl) throws Exception {
        BlobDescriptor blobDescriptor = asyncAm.statBlob(S3Endpoint.FDS_S3, bucketName, objectName).get();
        if (!hasBlobPermission(token, bucketName, Intent.changePermissions, blobDescriptor.getMetadata())) {
            throw new SecurityException();
        }
        blobDescriptor = withAcl(blobDescriptor, newAcl);
        TxDescriptor tx = asyncAm.startBlobTx(S3Endpoint.FDS_S3, bucketName, objectName, 1).get();
        asyncAm.updateMetadata(S3Endpoint.FDS_S3, bucketName, objectName, tx, blobDescriptor.getMetadata()).get();
        asyncAm.commitBlobTx(S3Endpoint.FDS_S3, bucketName, objectName, tx).get();
    }

    public boolean hasBlobPermission(AuthenticationToken token, String bucket, Intent intent, Map<String, String> blobMetadata) {
        if (authorizer.ownsVolume(token, bucket)) {
            return true;
        }

        if (hasVolumePermission(token, bucket, intent)) {
            return true;
        }

        XdiAcl.Value objectAcl = XdiAcl.parse(blobMetadata);
        return objectAcl.allow(intent);
    }

    public boolean hasVolumePermission(AuthenticationToken token, String volume, Intent intent) {
        if (authorizer.ownsVolume(token, volume)) {
            return true;
        }

        Map<String, String> volumeMetadata = new HashMap<>();

        try {
            volumeMetadata = volumeMetadataCache.get(volume, () -> asyncAm.getVolumeMetadata(S3Endpoint.FDS_S3, volume).get());
        } catch (ExecutionException e) {
            LOG.error("Error retrieving metadata for volume " + volume, e);
        }

        XdiAcl.Value acl = XdiAcl.parse(volumeMetadata);
        return acl.allow(intent);
    }

    public boolean hasToplevelPermission(AuthenticationToken token) {
        if (authenticator.allowAll()) {
            return true;
        } else {
            return !token.equals(AuthenticationToken.ANONYMOUS);
        }
    }

    public long tenantId(AuthenticationToken token) {
        return authorizer.tenantId(token);
    }

    public boolean allowAll() {
        return authenticator.allowAll();
    }

    public AuthenticationToken currentToken(String principal) throws LoginException {
        return authenticator.currentToken(principal);
    }

    public AuthenticationToken authenticate(String login, String password) throws LoginException {
        return authenticator.authenticate(login, password);
    }

    public AuthenticationToken parseToken(String tokenHeader) throws LoginException {
        return authenticator.parseToken(tokenHeader);
    }

    public void updateBucketAcl(AuthenticationToken token, String bucketName, XdiAcl.Value newAcl) throws Exception {
        if (!hasVolumePermission(token, bucketName, Intent.changePermissions)) {
            throw new SecurityException();
        }
        Map<String, String> metadata = asyncAm.getVolumeMetadata(S3Endpoint.FDS_S3, bucketName).get();
        metadata.put(XdiAcl.X_AMZ_ACL, newAcl.getCommonName());
        asyncAm.setVolumeMetadata(S3Endpoint.FDS_S3, bucketName, metadata).get();
        volumeMetadataCache.put(bucketName, metadata);
    }
}
