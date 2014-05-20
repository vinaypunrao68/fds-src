package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.Request;
import com.amazonaws.auth.AWSCredentials;
import com.amazonaws.auth.AWSSessionCredentials;
import com.amazonaws.auth.AbstractAWSSigner;
import com.amazonaws.auth.SigningAlgorithm;
import com.amazonaws.services.s3.internal.RestUtils;
import com.amazonaws.util.HttpUtils;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

// Modified S3 signer
// Original source: https://github.com/aws/aws-sdk-java/blob/master/src/main/java/com/amazonaws/services/s3/internal/S3Signer.java
public class CustomS3Signer extends AbstractAWSSigner {

    private static final Log log = LogFactory.getLog(CustomS3Signer.class);
    private final String httpVerb;
    private final String resourcePath;

    public CustomS3Signer(String httpVerb, String resourcePath) {
        this.httpVerb = httpVerb;
        this.resourcePath = resourcePath;

        if (resourcePath == null)
            throw new IllegalArgumentException("Parameter resourcePath is empty");
    }

    @Override
    public void sign(Request<?> request, AWSCredentials credentials) {

        if (resourcePath == null) {
            throw new UnsupportedOperationException(
                    "Cannot sign a request using a dummy S3Signer instance with "
                            + "no resource path"
            );
        }

        if (credentials == null || credentials.getAWSSecretKey() == null) {
            log.debug("Canonical string will not be signed, as no AWS Secret Key was provided");
            return;
        }

        AWSCredentials sanitizedCredentials = sanitizeCredentials(credentials);
        if ( sanitizedCredentials instanceof AWSSessionCredentials) {
            addSessionCredentials(request, (AWSSessionCredentials) sanitizedCredentials);
        }

        /*
         * In s3 sigv2, the way slash characters are encoded should be
         * consistent in both the request url and the encoded resource path.
         * Since we have to encode "//" to "/%2F" in the request url to make
         * httpclient works, we need to do the same encoding here for the
         * resource path.
         */
        String encodedResourcePath = HttpUtils.appendUri(request.getEndpoint().getPath(), resourcePath, true);

        String canonicalString = RestUtils.makeS3CanonicalString(
                httpVerb, encodedResourcePath, request, null);
        log.debug("Calculated string to sign:\n\"" + canonicalString + "\"");

        String signature = super.signAndBase64Encode(canonicalString, sanitizedCredentials.getAWSSecretKey(), SigningAlgorithm.HmacSHA1);
        request.addHeader("Authorization", "AWS " + sanitizedCredentials.getAWSAccessKeyId() + ":" + signature);
    }


    @Override
    protected void addSessionCredentials(Request<?> request, AWSSessionCredentials credentials) {
        request.addHeader("x-amz-security-token", credentials.getSessionToken());
    }
}
