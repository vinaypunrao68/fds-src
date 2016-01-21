package com.formationds.nfs;

import com.formationds.apis.MediaPolicy;
import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeSettings;
import com.formationds.apis.VolumeType;
import com.formationds.protocol.NfsOption;
import com.formationds.protocol.svc.types.ResourceState;
import org.junit.Before;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class ExportConfigurationBuilderTest {
    private VolumeSettings settings;
    private NfsOption nfsOptions;
    private VolumeDescriptor vd;

    @Test
    public void testEmptyNfsOptions() throws Exception {
        nfsOptions.setClient("localhost,192.168.1.0/8");
        String exportEntry = new ExportConfigurationBuilder().buildConfigurationEntry(vd);
        assertEquals("/foo localhost(rw,noacl,no_root_squash) 192.168.1.0/8(rw,noacl,no_root_squash)",
                exportEntry);
    }

    @Test
    public void testSyncOptionGetsFilteredOut() throws Exception {
        nfsOptions.setClient("*");
        nfsOptions.setOptions("rw,async,no_root_squash");
        String exportEntry = new ExportConfigurationBuilder().buildConfigurationEntry(vd);
        assertEquals("/foo *(rw,no_root_squash)",
                exportEntry);
    }

    @Before
    public void setUp() throws Exception {
        nfsOptions = new NfsOption();
        settings = new VolumeSettings(1024, VolumeType.NFS, 0, 0, MediaPolicy.HDD_ONLY);
        settings.setNfsOptions(nfsOptions);
        vd = new VolumeDescriptor("foo", 0, settings, 0, 0, ResourceState.Active);
    }
}