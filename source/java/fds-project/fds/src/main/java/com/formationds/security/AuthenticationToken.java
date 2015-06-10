package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import javax.crypto.SecretKey;

public class AuthenticationToken {
    public static final AuthenticationToken ANONYMOUS = new AuthenticationToken(-1, "");

    private final long userId;
    private final String secret;

    public AuthenticationToken(long userId, String secret) {
        this.userId = userId;
        this.secret = secret;
    }

    public String signature(SecretKey secretKey) {
        return new TokenEncrypter().signature(secretKey, this);
    }

    public String getSecret() {
        return secret;
    }

    public long getUserId() {
        return userId;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        AuthenticationToken that = (AuthenticationToken) o;

        if (userId != that.userId) return false;
        if (!secret.equals(that.secret)) return false;

        return true;
    }

    @Override
    public int hashCode() {
        int result = (int) (userId ^ (userId >>> 32));
        result = 31 * result + secret.hashCode();
        return result;
    }
}
