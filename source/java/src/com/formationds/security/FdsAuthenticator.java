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
    public AuthenticationToken issueToken(String login, String password) throws LoginException {
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
        try {
            String newSecret = UUID.randomUUID().toString();
            config.updateUser(user.getId(), user.getIdentifier(), user.getPasswordHash(), newSecret, user.isFdsAdmin);
            return new AuthenticationToken(KEY, user.getId(), newSecret);
        } catch (TException e) {
            throw new LoginException();
        }
    }
}
