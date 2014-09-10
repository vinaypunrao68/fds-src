package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ConfigurationService;
import com.formationds.security.HashedPassword;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.util.UUID;

public class EnsureAdminUser {
    private final static Logger LOG = Logger.getLogger(EnsureAdminUser.class);

    private ConfigurationService.Iface config;

    public EnsureAdminUser(ConfigurationService.Iface config) {
        this.config = config;
    }

    public void execute() throws TException {
        boolean hasAdmin = config.allUsers(0).stream()
                .filter(u -> "admin".equals(u.getIdentifier()))
                .findFirst()
                .isPresent();

        if (!hasAdmin) {
            config.createUser("admin", new HashedPassword().hash("admin"), UUID.randomUUID().toString(), true);
            LOG.info("First time boot, created admin user");
        }
    }
}
