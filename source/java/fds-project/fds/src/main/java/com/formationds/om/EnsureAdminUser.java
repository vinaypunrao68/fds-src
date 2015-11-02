package com.formationds.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ConfigurationService;
import com.formationds.om.webkit.rest.v08.users.GetUser;
import com.formationds.security.HashedPassword;

import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.util.UUID;

class EnsureAdminUser {
    private final static Logger LOG = Logger.getLogger(EnsureAdminUser.class);
    public static final String ADMIN_USERNAME = "admin";
    public static final String STATS_PASSWORD = "$t@t$";

    /**
     * Bootstrap the admin user if not already defined.
     *
     * @param config
     * @throws TException
     */
    static void bootstrapAdminUser(ConfigurationService.Iface config) throws TException {
        new EnsureAdminUser(config).execute();
    }

    private ConfigurationService.Iface config;

    public EnsureAdminUser(ConfigurationService.Iface config) {
        this.config = config;
    }

    public void execute() throws TException {
        boolean hasAdmin = config.allUsers(0).stream()
                .filter(u -> ADMIN_USERNAME.equals(u.getIdentifier()))
                .findFirst()
                .isPresent();

        if (!hasAdmin) {
            // TODO: default passwords are a security risk (even when "secure" passwords are used).
            // Installer should set or admin webapp should detect that this is a first time boot
            // and step the customer through first-time configuration steps, including defining a
            // secure password.
            config.createUser(ADMIN_USERNAME, new HashedPassword().hash(ADMIN_USERNAME), UUID.randomUUID().toString(), true);
            LOG.info("First time boot, created admin user");
        }
        
        boolean hasStats = config.allUsers(0).stream()
                .filter(u -> GetUser.STATS_USERNAME.equals(u.getIdentifier()))
                .findFirst()
                .isPresent();
        
        if (!hasAdmin) {
            // TODO: default passwords are a security risk (even when "secure" passwords are used).
            // Installer should set or admin webapp should detect that this is a first time boot
            // and step the customer through first-time configuration steps, including defining a
            // secure password.
            config.createUser( GetUser.STATS_USERNAME, new HashedPassword().hash( STATS_PASSWORD ), UUID.randomUUID().toString(), true);
            LOG.info("First time boot, created stats user");
        }
    }
}
