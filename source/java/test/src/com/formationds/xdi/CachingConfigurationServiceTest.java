package com.formationds.xdi;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.VolumeDescriptor;
import org.junit.Test;

import static org.mockito.Mockito.*;

public class CachingConfigurationServiceTest {
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
