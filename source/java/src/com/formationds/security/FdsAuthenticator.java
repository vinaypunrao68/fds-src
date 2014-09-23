package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.User;
import com.formationds.xdi.CachedConfiguration;
import com.formationds.xdi.ConfigurationApi;
import org.apache.commons.lang.StringUtils;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import javax.crypto.SecretKey;
import javax.security.auth.login.LoginException;
import java.util.Map;
import java.util.UUID;

public class FdsAuthenticator implements Authenticator {
    private final static Logger LOG = Logger.getLogger(FdsAuthenticator.class);
    private ConfigurationApi cache;
    private SecretKey secretKey;

    public FdsAuthenticator(ConfigurationApi cache, SecretKey secretKey) {
        this.cache = cache;
        this.secretKey = secretKey;
    }

    @Override
    public boolean allowAll() {
        return false;
    }


    @Override
    public AuthenticationToken authenticate(String login, String password) throws LoginException {
        CachedConfiguration cachedConfig = cache.get();
        Map<String, User> map = cachedConfig.usersByName();

        if (!map.containsKey(login)) {
            throw new LoginException();
        }

        HashedPassword hasher = new HashedPassword();

        if (StringUtils.isBlank(password)) {
            throw new LoginException();
        }

        boolean valid = hasher.verify(map.get(login).getPasswordHash(), password);

        if (!valid) {
            throw new LoginException();
        }

        User user = map.get(login);
        return new AuthenticationToken(user.getId(), user.getSecret());
    }

    @Override
    public AuthenticationToken currentToken(String login) throws LoginException {
        CachedConfiguration config = cache.get();
        Map<String, User> map = config.usersByName();
        if (!map.containsKey(login)) {
            throw new LoginException();
        }

        User user = map.get(login);
        return new AuthenticationToken(user.getId(), user.getSecret());
    }


    @Override
    public AuthenticationToken reissueToken(long userId) throws LoginException {
        User user = null;
        try {
            user = cache.allUsers(0).stream()
                    .filter(u -> u.getId() == userId)
                    .findFirst()
                    .orElseThrow(() -> new LoginException());
        } catch (TException e) {
            LOG.error("Error loading configuration", e);
            throw new LoginException();
        }

        String newSecret = UUID.randomUUID().toString();
        try {
            cache.updateUser(user.getId(), user.getIdentifier(), user.getPasswordHash(), newSecret, user.isFdsAdmin);
        } catch (TException e) {
            LOG.error("Error updating config", e);
            throw new RuntimeException(e);
        }
        return new AuthenticationToken(user.getId(), newSecret);
    }

    @Override
    public AuthenticationToken resolveToken(String signature) throws LoginException {
        AuthenticationToken token = null;
        try {
            token = new TokenEncrypter().tryParse(secretKey, signature);
        } catch (SecurityException e) {
            throw new LoginException();
        }

        long userId = token.getUserId();
        String secret = token.getSecret();
        long count = 0;
        try {
            count = cache.allUsers(0).stream()
                    .filter(u -> u.getId() == userId)
                    .filter(u -> u.getSecret().equals(secret))
                    .count();
        } catch (TException e) {
            LOG.error("Error reading configuration", e);
            throw new LoginException();
        }

        if (count == 0) {
            throw new LoginException();
        }

        return token;
    }
}
