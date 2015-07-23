package com.formationds.util.s3.auth;

import com.formationds.util.DigestUtil;
import org.apache.commons.codec.binary.Hex;
import org.apache.commons.codec.digest.DigestUtils;
import org.apache.commons.io.Charsets;
import org.joda.time.DateTime;
import org.joda.time.DateTimeZone;
import org.joda.time.format.ISODateTimeFormat;

public class S3SignatureGeneratorV4 {
    String createStringToSign(byte[] canonicalRequestSha, ScopeInfo scope, DateTime date) {
        StringBuilder builder = new StringBuilder();
        builder.append("AWS4-HMAC-SHA256\n");
        DateTime utcDate = date.toDateTime(DateTimeZone.UTC);
        builder.append(utcDate.toString(ISODateTimeFormat.basicDateTimeNoMillis()));
        builder.append("\n");

        builder.append(scope.toScopeString());
        builder.append("\n");

        builder.append(Hex.encodeHexString(canonicalRequestSha));
        return builder.toString();
    }

    String createChunkStringToSign(byte[] priorSignature, byte[] contentSha, ScopeInfo scope, DateTime date) {
        StringBuilder builder = new StringBuilder();
        builder.append("AWS4-HMAC-SHA256-PAYLOAD\n");

        DateTime utcDate = date.toDateTime(DateTimeZone.UTC);
        builder.append(utcDate.toString(ISODateTimeFormat.basicDateTimeNoMillis()));
        builder.append("\n");

        builder.append(scope.toScopeString());
        builder.append("\n");

        builder.append(Hex.encodeHexString(priorSignature));
        builder.append("\n");

        String emptyHash = DigestUtils.sha256Hex("");
        builder.append(emptyHash);
        builder.append("\n");

        builder.append(Hex.encodeHexString(contentSha));
        return builder.toString();
    }

    // TODO: this is very similar to the HMAC code in S3SignatureGenerator


    byte[] hmacSha256(byte[] key, String data) {
        return DigestUtil.hmacSha256(key, data.getBytes(Charsets.UTF_8));
    }

    byte[] createSigningKey(String secretKey, ScopeInfo scopeInfo) {
        byte[] kSecret = ("AWS4" + secretKey).getBytes(Charsets.UTF_8);
        //byte[] dateKey = hmacSha256(kSecret, date.toString(ISODateTimeFormat.basicDate()));
        byte[] dateKey = hmacSha256(kSecret, scopeInfo.getDateStamp());
        byte[] dateRegionKey = hmacSha256(dateKey, scopeInfo.getRegion());
        byte[] dateRegionServiceKey = hmacSha256(dateRegionKey, scopeInfo.getService());
        byte[] signingKey = hmacSha256(dateRegionServiceKey, "aws4_request");
        return signingKey;
    }

    public byte[] signature(String secretKey, DateTime date, ScopeInfo scopeInfo, byte[] canonicalRequestSha) {
        byte[] signingKey = createSigningKey(secretKey, scopeInfo);
        String stringToSign = createStringToSign(canonicalRequestSha, scopeInfo, date);
        return hmacSha256(signingKey, stringToSign);
    }

    public byte[] fullContentSignature(String secretKey, SignatureRequestData requestData, ScopeInfo scopeInfo, byte[] fullContentSha) {
        byte[] requestSha = requestData.fullCanonicalRequestHash(fullContentSha);
        return signature(secretKey, requestData.getDate(), scopeInfo, requestSha);
    }

    public byte[] seedSignature(String secretKey, SignatureRequestData requestData, ScopeInfo scopeInfo) {
        byte[] requestSha = requestData.chunkedCanonicalRequestHash();
        return signature(secretKey, requestData.getDate(), scopeInfo, requestSha);
    }

    public byte[] chunkSignature(String secretKey, SignatureRequestData requestData, ScopeInfo scopeInfo, byte[] priorChunkSignature, byte[] contentHash) {
        String stringToSign = createChunkStringToSign(priorChunkSignature, contentHash, scopeInfo, requestData.getDate());
        byte[] signingKey = createSigningKey(secretKey, scopeInfo);
        return hmacSha256(signingKey, stringToSign);
    }
}
