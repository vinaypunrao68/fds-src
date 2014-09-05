package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.commons.codec.binary.Hex;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;
import java.security.Security;
import java.text.MessageFormat;

public class TokenEncrypter {
    private static final String TOKEN_FORMAT = "id: {0}, secret: {1}";

    static {
        Security.addProvider(new org.bouncycastle.jce.provider.BouncyCastleProvider());
    }

    public AuthenticationToken tryParse(SecretKey key, String encrypted) throws SecurityException {
        try {
            Object[] parsed = decryptAndParse(encrypted, key);
            long userId = Long.parseLong((String) parsed[0]);
            String secret = ((String) parsed[1]);
            return new AuthenticationToken(userId, secret);
        } catch (Exception e) {
            throw new SecurityException();
        }
    }

    public String signature(SecretKey secretKey, AuthenticationToken token) {
        try {
            String value = MessageFormat.format(TOKEN_FORMAT, token.getUserId(), token.getSecret());
            byte[] clearText = value.getBytes();
            Cipher cipher = Cipher.getInstance("AES/CBC/PKCS7Padding", "BC");
            IvParameterSpec initVector = new IvParameterSpec(secretKey.getEncoded());
            cipher.init(Cipher.ENCRYPT_MODE, secretKey, initVector);
            byte[] encrypted = cipher.doFinal(clearText);
            return Hex.encodeHexString(encrypted);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    private Object[] decryptAndParse(String encrypted, SecretKey key) throws Exception {
        String decrypted = decrypt(encrypted, key);
        return new MessageFormat(TOKEN_FORMAT).parse(decrypted);
    }

    private String decrypt(String encrypted, SecretKey key) throws Exception {
        byte[] bytes = Hex.decodeHex(encrypted.toCharArray());
        Cipher cipher = Cipher.getInstance("AES/CBC/PKCS7Padding", "BC");
        IvParameterSpec initVector = new IvParameterSpec(key.getEncoded());
        cipher.init(Cipher.DECRYPT_MODE, key, initVector);
        return new String(cipher.doFinal(bytes));
    }
}
