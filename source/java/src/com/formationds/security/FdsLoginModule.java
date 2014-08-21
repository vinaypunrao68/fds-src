package com.formationds.security;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.User;
import com.formationds.om.CachedConfiguration;
import org.apache.commons.lang.StringUtils;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import javax.security.auth.login.LoginException;
import java.util.Map;

public class FdsLoginModule implements LoginModule {
    private final static Logger LOG = Logger.getLogger(FdsLoginModule.class);

    private CachedConfiguration config;

    public FdsLoginModule(CachedConfiguration config) {
        this.config = config;
    }

    @Override
    public User login(String login, String password) throws LoginException {
        if (!passwordMatches(login, password)) {
            throw new LoginException();
        }

        try {
            return config.usersByLogin().get(login);
        } catch (TException e) {
            LOG.error("Error polling configuration", e);
            throw new LoginException();
        }
    }

    private boolean passwordMatches(String login, String candidatePassword) {
        Map<String, User> map = null;
        try {
            map = config.usersByLogin();
        } catch (TException e) {
            LOG.error("Error polling configuration", e);
            throw new RuntimeException(e);
        }

        if (!map.containsKey(login)) {
            return false;
        }

        HashedPassword hasher = new HashedPassword();

        if (StringUtils.isBlank(candidatePassword)) {
            return false;
        }

        return hasher.verify(map.get(login).getPasswordHash(), candidatePassword);
    }
}
