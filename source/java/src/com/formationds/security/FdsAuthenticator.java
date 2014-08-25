package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.User;
import org.apache.commons.lang.StringUtils;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import javax.security.auth.login.LoginException;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.stream.Collectors;

public class FdsAuthenticator implements Authenticator {
    private final static Logger LOG = Logger.getLogger(FdsAuthenticator.class);

    private ConfigurationService.Iface config;

    public FdsAuthenticator(ConfigurationService.Iface config) {
        this.config = config;
    }

    @Override
    public AuthenticationToken authenticate(String login, String password) throws LoginException {
        Map<String, User> map = null;
        try {
            map = config.allUsers(0)
                    .stream()
                    .collect(Collectors.toMap(u -> u.getIdentifier(), u -> u));
        } catch (TException e) {
            LOG.error("Error loading configuration", e);
            throw new LoginException();
        }

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
    public AuthenticationToken reissueToken(long userId) throws LoginException {
        List<User> users = null;
        try {
            users = config.allUsers(0);
        } catch (TException e) {
            LOG.error("Error reading config", e);
            throw new LoginException();
        }

        return users.stream()
                .filter(u -> u.getId() == userId)
                .map(user -> {
                    String newSecret = UUID.randomUUID().toString();
                    try {
                        config.updateUser(user.getId(), user.getIdentifier(), user.getPasswordHash(), newSecret, user.isFdsAdmin);
                    } catch (TException e) {
                        LOG.error("Error updating config", e);
                        throw new RuntimeException(e);
                    }
                    return new AuthenticationToken(user.getId(), newSecret);
                })
                .findFirst()
                .get();
    }

    @Override
    public AuthenticationToken resolveToken(String signature) throws LoginException {
        AuthenticationToken token = null;
        try {
            token = new TokenEncrypter().tryParse(KEY, signature);
        } catch (SecurityException e) {
            throw new LoginException();
        }

        long userId = token.getUserId();
        String secret = token.getSecret();
        long count = 0;
        try {
            count = config.allUsers(0).stream()
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
