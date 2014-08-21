package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.VolumeDescriptor;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

public class CachingConfigurationServiceTest {
    //@Test
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
        assertEquals("fab", config.listUsersForTenant(tenantId).get(0).getIdentifier());

        long v = versions.get(0);
        for (int i = 1; i < versions.size(); i++) {
            assertNotEquals(v, (long) versions.get(i));
            v = versions.get(i);
        }
    }

    @Test
    public void testCacheUsers() {
        fail();
    }

    @Test
    public void testCreateVolume() throws Exception {
        ConfigurationService.Iface mock = mock(ConfigurationService.Iface.class);
        when(mock.statVolume("foo", "bar")).thenReturn(new VolumeDescriptor());
        when(mock.statVolume("foo", "hello")).thenReturn(new VolumeDescriptor());
        CachingConfigurationService service = new CachingConfigurationService(mock);
        service.statVolume("foo", "bar");
        service.statVolume("foo", "hello");
        service.statVolume("foo", "bar");
        service.statVolume("foo", "hello");

        verify(mock, times(1)).statVolume("foo", "bar");
        verify(mock, times(1)).statVolume("foo", "hello");
    }
}
