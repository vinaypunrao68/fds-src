/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.security;

import com.formationds.apis.User;
import com.formationds.util.thrift.ConfigurationApi;
import org.apache.commons.lang.StringUtils;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import javax.crypto.SecretKey;
import javax.security.auth.login.LoginException;
import java.util.UUID;

public class FdsAuthenticator implements Authenticator {
    private final static Logger LOG = Logger.getLogger(FdsAuthenticator.class);
    private ConfigurationApi cache;
    private SecretKey        secretKey;

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
        User user = cache.getUser(login);
        if (user == null) {
            throw new LoginException();
        }

        HashedPassword hasher = new HashedPassword();

        if (StringUtils.isBlank(password)) {
            throw new LoginException();
        }

        boolean valid = hasher.verify(user.getPasswordHash(), password);

        if (!valid) {
            throw new LoginException();
        }

        return new AuthenticationToken(user.getId(), user.getSecret());
    }

    @Override
    public AuthenticationToken currentToken(String login) throws LoginException {
        User user = cache.getUser(login);
        if (user == null) {
            throw new LoginException();
        }
        return new AuthenticationToken(user.getId(), user.getSecret());
    }


    @Override
    public AuthenticationToken reissueToken(long userId) throws LoginException {
        User user = cache.getUser( userId );

        if ( user == null ) {
            LOG.error("User " + userId + " not found." );
            throw new LoginException( );
        }

        String newSecret = UUID.randomUUID().toString();
        try {

            cache.updateUser(user.getId(), user.getIdentifier(), user.getPasswordHash(), newSecret, user.isIsFdsAdmin());

        } catch (TException e) {

            LOG.error("Error updating config", e);
            throw new RuntimeException(e);

        }
        return new AuthenticationToken(user.getId(), newSecret);
    }

    @Override
    public AuthenticationToken parseToken(String signature) throws LoginException {
        AuthenticationToken token;
        try {
            token = new TokenEncrypter().tryParse(secretKey, signature);
        } catch (SecurityException e) {
            throw new LoginException();
        }

        long userId = token.getUserId();
        String secret = token.getSecret();

        User user = cache.getUser( userId );

        if ( user == null ) {
            LOG.error("User " + userId + " not found." );
            throw new LoginException( );
        }

        if (!user.getSecret().equals( secret )) {
            LOG.error("Authentication failed for user " + userId + "(" + user.getIdentifier() + ") not found." );
            throw new LoginException( );
        }

        return token;
    }
}
