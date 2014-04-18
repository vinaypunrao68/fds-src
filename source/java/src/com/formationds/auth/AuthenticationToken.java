package com.formationds.auth;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.commons.codec.binary.Base64;

import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import java.security.Principal;
import java.text.MessageFormat;
import java.util.UUID;

public class AuthenticationToken {
    private static final String TOKEN_FORMAT = "UUID: {0}, name: {1}";
    private String name;
    private boolean isValid;
    private SecretKey secretKey;

    public AuthenticationToken(SecretKey secretKey, Principal principal) {
        this.secretKey = secretKey;
        name = principal.getName();
        isValid = true;
    }

    public AuthenticationToken(SecretKey secretKey, String encrypted) {
        this.secretKey = secretKey;
        try {
            name = decrypt(encrypted);
            isValid = true;
        } catch (Exception e) {
            isValid = false;
        }
    }

    private String decrypt(String encrypted) throws Exception {
        byte[] bytes = Base64.decodeBase64(encrypted);
        Cipher cipher = Cipher.getInstance("AES");
        cipher.init(Cipher.DECRYPT_MODE, secretKey);
        return new String(cipher.doFinal(bytes));
    }

    public boolean isValid() {
        return isValid;
    }

    @Override
    public String toString() {
        try {
            String value = MessageFormat.format(TOKEN_FORMAT, UUID.randomUUID().toString(), name);
            byte[] clearText = value.getBytes();
            Cipher cipher = Cipher.getInstance("AES");
            cipher.init(Cipher.ENCRYPT_MODE, secretKey);
            byte[] encrypted = cipher.doFinal(clearText);
            return Base64.encodeBase64String(encrypted);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
