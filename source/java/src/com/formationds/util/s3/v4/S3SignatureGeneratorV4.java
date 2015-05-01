package com.formationds.util.s3.v4;

import org.bouncycastle.util.encoders.Hex;
import org.joda.time.DateTime;
import org.joda.time.DateTimeZone;
import org.joda.time.format.ISODateTimeFormat;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;
import java.security.InvalidKeyException;
import java.security.NoSuchAlgorithmException;

public class S3SignatureGeneratorV4 {
    private String createStringToSign(byte[] canonicalRequestSha, String awsRegion, DateTime date) {
        StringBuilder builder = new StringBuilder();
        builder.append("AWS4-HMAC-SHA256\n");
        DateTime utcDate = date.toDateTime(DateTimeZone.UTC);
        builder.append(utcDate.toString(ISODateTimeFormat.basicDateTimeNoMillis()));
        builder.append("\n");

        builder.append(utcDate.toString(ISODateTimeFormat.basicDate()));
        builder.append("/");
        builder.append(awsRegion);
        builder.append("/s3/aws4_request\n");

        builder.append(Hex.toHexString(canonicalRequestSha));
        return builder.toString();
    }

    // TODO: this is very similar to the HMAC code in S3SignatureGenerator
    private byte[] hmacSha256(byte[] key, byte[] data) {
        try {
            SecretKeySpec keySpec = new SecretKeySpec(key, "HmacSHA256");
            Mac mac = Mac.getInstance("HmacSHA256");
            mac.init(keySpec);
            return mac.doFinal(data);
        } catch(NoSuchAlgorithmException | InvalidKeyException ex) {
            throw new RuntimeException(ex);
        }
    }

    private byte[] hmacSha256(String key, String data) {
        return hmacSha256(key.getBytes(), data.getBytes());
    }

    private byte[] hmacSha256(byte[] key, String data) {
        return hmacSha256(key, data.getBytes());
    }

    private byte[] createSigningKey(String secretKey, DateTime date, String awsRegion) {
        DateTime utcDate = date.toDateTime(DateTimeZone.UTC);
        byte[] dateKey = hmacSha256("AWS4" + secretKey, date.toString(ISODateTimeFormat.basicDate()));
        byte[] dateRegionKey = hmacSha256(dateKey, awsRegion);
        byte[] dateRegionServiceKey = hmacSha256(dateRegionKey, "s3");
        byte[] signingKey = hmacSha256(dateRegionServiceKey, "aws4_request");
        return signingKey;
    }

    public byte[] signature(String secretKey, DateTime date, String awsRegion, byte[] canonicalRequestSha) {
        byte[] signingKey = createSigningKey(secretKey, date, awsRegion);
        String stringToSign = createStringToSign(canonicalRequestSha, awsRegion, date);
        return hmacSha256(signingKey, stringToSign);
    }

    public byte[] fullContentSignature(String secretKey, SignatureRequestData requestData, String awsRegion, byte[] fullContentSha) {
        byte[] requestSha = requestData.fullCanonicalRequestHash(fullContentSha);
        return signature(secretKey, requestData.getDate(), awsRegion, requestSha);
    }
}
