package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.commons.codec.binary.Base64;
import org.bouncycastle.jcajce.provider.digest.SHA3;

import java.nio.charset.Charset;
import java.text.MessageFormat;
import java.util.UUID;

public class HashedPassword {
    private static final Charset CHARSET = Charset.forName("UTF-8");
    private static final String FORMAT = "{0}:{1}";

    public String hash(String password) {
        String salt = UUID.randomUUID().toString();
        SHA3.DigestSHA3 md = new SHA3.DigestSHA3(256); //same as DigestSHA3 md = new SHA3.Digest256();
        md.update(salt.getBytes(CHARSET));
        md.update(password.getBytes(CHARSET));
        byte[] digest = md.digest();
        return salt + ":" + Base64.encodeBase64String(digest);
    }

    public boolean verify(String hashed, String candidatePassword) {
        try {
            Object[] parts = new MessageFormat(FORMAT).parse(hashed);
            String salt = (String) parts[0];
            String hash = (String) parts[1];
            SHA3.DigestSHA3 md = new SHA3.DigestSHA3(256); //same as DigestSHA3 md = new SHA3.Digest256();
            md.update(salt.getBytes(CHARSET));
            md.update(candidatePassword.getBytes(CHARSET));
            byte[] digest = md.digest();
            String candidateHash = Base64.encodeBase64String(digest);
            return hash.equals(candidateHash);
        } catch (Exception e) {
            return false;
        }
    }
}
