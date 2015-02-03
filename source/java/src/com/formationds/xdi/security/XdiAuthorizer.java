package com.formationds.xdi.security;

import com.formationds.apis.*;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authenticator;
import com.formationds.security.Authorizer;
import com.formationds.util.blob.Mode;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.xdi.XdiConfigurationApi;
import com.formationds.xdi.s3.PutObjectAcl;
import com.formationds.xdi.s3.S3Endpoint;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import javax.security.auth.login.LoginException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.stream.Collectors;

public class XdiAuthorizer {
    private static final Logger LOG = Logger.getLogger(XdiAuthorizer.class);
    private Authenticator authenticator;
    private Authorizer authorizer;
    private AmService.Iface am;
    private ConfigurationApi config;
    private final BucketMetadataCache cache;

    public XdiAuthorizer(Authenticator authenticator, Authorizer authorizer, AmService.Iface am, ConfigurationApi config) {
        this.authenticator = authenticator;
        this.authorizer = authorizer;
        this.am = am;
        this.config = config;
        cache = new BucketMetadataCache(config, am);
        cache.start();
    }

    public boolean hasVolumePermission(AuthenticationToken token, String volume, Intent intent) {
        if (authorizer.ownsVolume(token, volume)) {
            return true;
        }

        Map<String, String> map = cache.getMetadata(volume);
        String acl = map.getOrDefault(PutObjectAcl.X_AMZ_ACL, PutObjectAcl.PRIVATE);

        if (intent.equals(Intent.read)) {
            return !PutObjectAcl.PRIVATE.equals(acl);
        } else if (intent.equals(Intent.readWrite)) {
            return PutObjectAcl.PUBLIC_READ_WRITE.equals(acl);
        }

        return false;
    }

    public boolean hasToplevelPermission(AuthenticationToken token, Intent intent) {
        return !token.equals(AuthenticationToken.ANONYMOUS);
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

    public void updateBucketAcl(String bucketName, String aclName) throws Exception {
        VolumeDescriptor volumeDescriptor = config.statVolume(S3Endpoint.FDS_S3, bucketName);
        String systemVolume = XdiConfigurationApi.systemFolderName(volumeDescriptor.getTenantId());
        createVolumeIfNeeded(systemVolume, volumeDescriptor.getTenantId());
        Map<String, String> metadata = new HashMap<String, String>();
        metadata.put(PutObjectAcl.X_AMZ_ACL, aclName);
        String blobName = makeBucketAclBlobName(volumeDescriptor.getVolId());
        am.updateBlobOnce(S3Endpoint.FDS_S3, systemVolume, blobName, Mode.TRUNCATE.getValue(), ByteBuffer.allocate(0), 0, new ObjectOffset(0), metadata);
        cache.refresh();
    }

    private void createVolumeIfNeeded(String volume, long tenantId) {
        try {
            config.createVolume(S3Endpoint.FDS_S3, volume, new VolumeSettings(1024 * 1024 * 2, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY), tenantId);
        } catch (TException e) {

        }
    }

    private String makeBucketAclBlobName(long volumeId) {
        return "fds_bucket_acl_" + volumeId;
    }


    private class BucketMetadataCache {
        private volatile Map<String, Map<String, String>> metadataCache;
        private final ConfigurationApi config;
        private final AmService.Iface am;

        BucketMetadataCache(ConfigurationApi config, AmService.Iface am) {
            this.config = config;
            this.am = am;
            this.metadataCache = new HashMap<>();
        }

        void start() {
            refresh();
            Thread thread = new Thread(() -> {
                while (true) {
                    try {
                        Thread.sleep(2000);
                        refresh();
                    } catch (Exception e) {
                        LOG.error("Error refreshing bucket metadata cache", e);
                    }
                }
            });
            thread.setName("Volume metadata cache updater");
            thread.start();
        }

        Map<String, String> getMetadata(String bucketName) {
            return metadataCache.getOrDefault(bucketName, new HashMap<>());
        }

        void refresh() {
            try {
                metadataCache = config.listVolumes(S3Endpoint.FDS_S3)
                        .stream()
                        .collect(Collectors.<VolumeDescriptor, String, Map<String, String>>toMap(
                                vd -> vd.getName(),
                                vd -> {
                                    String systemVolume = XdiConfigurationApi.systemFolderName(vd.getTenantId());
                                    String blobName = makeBucketAclBlobName(vd.getVolId());
                                    try {
                                        BlobDescriptor blobDescriptor = am.statBlob(S3Endpoint.FDS_S3, systemVolume, blobName);
                                        return blobDescriptor.getMetadata();
                                    } catch (ApiException e) {
                                        return new HashMap<>();
                                    } catch (Exception e) {
                                        throw new RuntimeException(e);
                                    }
                                }));
            } catch (TException e) {
                throw new RuntimeException(e);
            }
        }
    }

}
