package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.commons.codec.binary.Base64;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import java.text.MessageFormat;

public class AuthenticationToken {
    private static final String TOKEN_FORMAT = "id: {0}, secret: {1}";
    private final SecretKey secretKey;
    private final long userId;
    private final String secret;
    private final String signature;

    public AuthenticationToken(SecretKey secretKey, long userId, String secret) {
        this.secretKey = secretKey;
        this.userId = userId;
        this.secret = secret;
        this.signature = generateSignature();
    }

    public AuthenticationToken(SecretKey secretKey, String encrypted) throws SecurityException {
        this.secretKey = secretKey;
        try {
            Object[] parsed = decryptAndParse(encrypted, secretKey);
            this.userId = Long.parseLong((String) parsed[0]);
            this.secret = ((String) parsed[1]);
            this.signature = encrypted;
        } catch (Exception e) {
            throw new SecurityException();
        }
    }

    private static Object[] decryptAndParse(String encrypted, SecretKey key) throws Exception {
        String decrypted = decrypt(encrypted, key);
        return new MessageFormat(TOKEN_FORMAT).parse(decrypted);
    }

    public String signature() {
        return signature;
    }

    private String generateSignature() {
        try {
            String value = MessageFormat.format(TOKEN_FORMAT, userId, secret);
            byte[] clearText = value.getBytes();
            Cipher cipher = Cipher.getInstance("AES");
            cipher.init(Cipher.ENCRYPT_MODE, secretKey);
            byte[] encrypted = cipher.doFinal(clearText);
            return Base64.encodeBase64String(encrypted);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    private static String decrypt(String encrypted, SecretKey key) throws Exception {
        byte[] bytes = Base64.decodeBase64(encrypted);
        Cipher cipher = Cipher.getInstance("AES");
        cipher.init(Cipher.DECRYPT_MODE, key);
        return new String(cipher.doFinal(bytes));
    }

    public String getSecret() {
        return secret;
    }

    public long getUserId() {
        return userId;
    }

    public static boolean isValid(SecretKey secretKey, String tokenSignature) {
        try {
            decryptAndParse(tokenSignature, secretKey);
            return true;
        } catch (Exception e) {
            return false;
        }
    }
}
