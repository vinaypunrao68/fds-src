package com.formationds.util;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;
import java.security.InvalidKeyException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class DigestUtil {
    private static MessageDigest getMd(String mdAlgorithm) {
        try {
            return MessageDigest.getInstance(mdAlgorithm);
        } catch(NoSuchAlgorithmException ex) {
            throw new AssertionError(mdAlgorithm + "message digest algorithm not found", ex);
        }
    }

    private static Mac getMac(String macAlgorithm) {
        try {
            return Mac.getInstance(macAlgorithm);
        } catch(NoSuchAlgorithmException ex) {
            throw new AssertionError(macAlgorithm + " mac algorithm not found", ex);
        }
    }

    public static MessageDigest newSha256() {
        return getMd("SHA-256");
    }

    public static MessageDigest newMd5() {
        return getMd("MD5");
    }

    public static Mac newHmacSha1() {
        return getMac("HmacSHA1");
    }

    public static Mac newHmacSha256() {
        return getMac("HmacSHA256");
    }

    public static byte[] hmacSha256(byte[] key, byte[] data) {
        try {
            SecretKeySpec keySpec = new SecretKeySpec(key, "HmacSHA256");
            Mac mac = DigestUtil.newHmacSha256();
            mac.init(keySpec);
            return mac.doFinal(data);
        } catch(InvalidKeyException ex) {
            throw new SecurityException("key is not valid", ex);
        }
    }
}
