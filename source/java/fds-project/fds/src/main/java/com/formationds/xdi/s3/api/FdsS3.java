package com.formationds.xdi.s3.api;

import com.formationds.apis.MediaPolicy;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.commons.util.SupplierWithExceptions;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.security.AuthenticationToken;
import com.formationds.util.RunnableWithExceptions;
import com.formationds.util.async.CompletableFutureUtility;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.s3.S3Endpoint;

import java.util.List;
import java.util.concurrent.CompletableFuture;

public class FdsS3 {
    private final AuthenticationToken token;
    private Xdi xdi;
    private final String DOMAIN = S3Endpoint.FDS_S3;

    public FdsS3(AuthenticationToken token, Xdi xdi) {
        this.token = token;
        this.xdi = xdi;
    }

    public FdsS3Bucket bucket(String bucketName) {
        return new FdsS3Bucket(bucketName);
    }

    public long getTenantId() {
        return xdi.getAuthorizer().tenantId(token);
    }

    public CompletableFuture<List<VolumeDescriptor>> listBuckets() {
        return callSync(() -> xdi.listVolumes(token, DOMAIN));
    }

    private <T> CompletableFuture<T> callSync(SupplierWithExceptions<T> sup) {
        try {
            return CompletableFuture.completedFuture(sup.supply());
        } catch (Exception ex) {
            return CompletableFutureUtility.exceptionFuture(ex);
        }
    }

    private <Void> CompletableFuture<Void> runSync(RunnableWithExceptions run) {
        try {
            run.run();
            return CompletableFuture.completedFuture(null);
        } catch (Exception ex) {
            return CompletableFutureUtility.exceptionFuture(ex);
        }
    }

    public class FdsS3Bucket {
        private String bucketName;
        public FdsS3Bucket(String bucketName) {
            this.bucketName = bucketName;
        }

        public FdsS3Object object(String objectName) {
            return new FdsS3Object(bucketName, objectName);
        }

        public CompletableFuture<Void> create() {
            return runSync(() -> {
                VolumeSettings volumeSettings = new VolumeSettings(1024 * 1024 * 2, VolumeType.OBJECT, Long.MAX_VALUE, 24 * 60 * 60 /** one day **/, MediaPolicy.HDD_ONLY);
                xdi.createVolume(token, DOMAIN, bucketName, volumeSettings);
            });
        }

        public CompletableFuture<Void> delete() {
            return runSync(() -> xdi.deleteVolume(token, DOMAIN, bucketName));
        }

        public CompletableFuture<Boolean> exists() {
            return callSync(() -> xdi.statVolume(token, DOMAIN, bucketName) != null);
        }

        // namespace?
        public CompletableFuture<BlobDescriptor> listObjects() {
            return null;
        }
    }

    public class FdsS3Object {
        private String bucketName;
        private String objectName;

        public FdsS3Object(String bucketName, String objectName) {
            this.bucketName = bucketName;
            this.objectName = objectName;
        }
    }

    public class ObjectData {

    }
}
