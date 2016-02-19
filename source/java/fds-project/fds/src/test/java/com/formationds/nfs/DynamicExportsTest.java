package com.formationds.nfs;

import com.formationds.apis.MediaPolicy;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.protocol.NfsOption;
import com.formationds.protocol.svc.types.ResourceState;
import com.formationds.xdi.XdiConfigurationApi;
import com.google.common.collect.Lists;
import org.junit.After;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.assertEquals;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class DynamicExportsTest {

    private DynamicExports dynamicExports;

    @Test
    public void testVolumeDeleted() throws Exception {
        List<String> events = new ArrayList<>();
        XdiConfigurationApi config = mock(XdiConfigurationApi.class);
        when(config.listVolumes(anyString())).thenReturn(
                Lists.newArrayList(makeNfsVolume(1, "foo"), makeNfsVolume(2, "bar"))
        );
        dynamicExports = new DynamicExports(config);
        dynamicExports.addVolumeDeleteEventHandler(v -> events.add("Volume " + v + " deleted"));

        assertEquals(2, dynamicExports.exportNames().size());

        when(config.listVolumes(anyString())).thenReturn(
                Lists.newArrayList(makeNfsVolume(2, "bar"))
        );
        dynamicExports.refreshOnce();
        assertEquals(1, dynamicExports.exportNames().size());
        assertEquals(1, events.size());
        assertEquals("Volume foo deleted", events.get(0));
    }

    private VolumeDescriptor makeNfsVolume(long volumeId, String name) {
        VolumeSettings volumeSettings = new VolumeSettings(1024, VolumeType.NFS, 0, 0, MediaPolicy.HDD_ONLY);
        NfsOption nfsOptions = new NfsOption();
        nfsOptions.setOptions("rw,no_root_squash,noacl");
        nfsOptions.setClient("*");
        volumeSettings.setNfsOptions(nfsOptions);
        VolumeDescriptor vd = new VolumeDescriptor(name, 0, volumeSettings, 0, 0, ResourceState.Active);
        vd.setVolId(volumeId);
        return vd;
    }

    @After
    public void tearDown() throws Exception {
        dynamicExports.deleteExportFile();
    }
}