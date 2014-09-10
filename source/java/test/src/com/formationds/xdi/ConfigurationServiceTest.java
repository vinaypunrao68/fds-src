package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_VolumeDescType;
import com.formationds.apis.ConfigurationService;

import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;

public class ConfigurationServiceTest {
    // @Test
    public void testIntegration() throws Exception {
        ConfigurationService.Iface config = new XdiClientFactory().remoteOmService("localhost", 9090);
        List<Long> versions = new ArrayList<>();
        versions.add(config.configurationVersion(0));
        long tenantId = config.createTenant("Goldman");
        versions.add(config.configurationVersion(0));
        long userId = config.createUser("fab", "foobar", "secret", false);
        versions.add(config.configurationVersion(0));
        config.assignUserToTenant(userId, tenantId);
        versions.add(config.configurationVersion(0));
        assertEquals("fab", config.allUsers(0).get(0).getIdentifier());
        assertEquals("foobar", config.allUsers(0).get(0).getPasswordHash());
        assertEquals("secret", config.allUsers(0).get(0).getSecret());

        assertEquals("fab", config.listUsersForTenant(tenantId).get(0).getIdentifier());

        long v = versions.get(0);
        for (int i = 1; i < versions.size(); i++) {
            assertNotEquals(v, (long) versions.get(i));
            v = versions.get(i);
        }
    }

    //@Test
    public void testVolumeTenant() throws Exception {
        ConfigurationService.Iface config = new XdiClientFactory().remoteOmService("localhost", 9090);
        config.listVolumes("")
                .forEach(v -> System.out.println(v.getTenantId()));
    }

    //@Test
    public void testLegacyClient() throws Exception {
        FDSP_ConfigPathReq.Iface client = new XdiClientFactory().legacyConfig("localhost", 8903);
        List<FDSP_VolumeDescType> list = client.ListVolumes(new FDSP_MsgHdrType());
        System.out.println(list.size());
    }
}
