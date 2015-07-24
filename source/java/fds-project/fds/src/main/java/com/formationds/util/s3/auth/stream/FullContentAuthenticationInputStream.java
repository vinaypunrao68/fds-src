package com.formationds.util.s3.auth.stream;

import com.amazonaws.auth.AWSCredentials;
import com.formationds.util.DigestUtil;
import com.formationds.util.s3.auth.S3SignatureGeneratorV4;
import com.formationds.util.s3.auth.ScopeInfo;
import com.formationds.util.s3.auth.SignatureRequestData;

import java.io.IOException;
import java.io.InputStream;
import java.security.DigestInputStream;
import java.util.Arrays;

public class FullContentAuthenticationInputStream extends InputStream {
    DigestInputStream inputStream;
    private final AWSCredentials credentials;
    private final SignatureRequestData requestData;
    private final ScopeInfo scopeInfo;
    private byte[] contentSha;
    private byte[] expectedSignature;

    public FullContentAuthenticationInputStream(InputStream inputStream, AWSCredentials credentials, SignatureRequestData requestData, ScopeInfo scopeInfo, byte[] expectedSignature) {
        this.credentials = credentials;
        this.requestData = requestData;
        this.scopeInfo = scopeInfo;
        this.inputStream = new DigestInputStream(inputStream, DigestUtil.newSha256());
        this.expectedSignature = expectedSignature;
    }

    @Override
    public int read(byte[] b) throws IOException {
        int result = inputStream.read(b);
        if(result == -1)
            validate();
        return result;
    }

    @Override
    public int read(byte[] b, int off, int len) throws IOException {
        int result = inputStream.read(b, off, len);
        if(result == -1)
            validate();
        return result;
    }

    @Override
    public int read() throws IOException {
        int result = inputStream.read();
        if(result == -1)
            validate();
        return result;
    }

    @Override
    public int available() throws IOException {
        return inputStream.available();
    }

    @Override
    public void close() throws IOException {
        inputStream.close();
    }

    private void validate() {
        if(contentSha == null) {
            contentSha = inputStream.getMessageDigest().digest();
        }
        S3SignatureGeneratorV4 gen = new S3SignatureGeneratorV4();
        byte[] actualSignature = gen.fullContentSignature(credentials.getAWSSecretKey(), this.requestData, this.scopeInfo, contentSha);

        if(!Arrays.equals(actualSignature, expectedSignature))
            throw new SecurityException("signature does not match expected signature in request");
    }
}
