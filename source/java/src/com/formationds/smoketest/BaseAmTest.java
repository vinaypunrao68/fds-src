package com.formationds.smoketest;

import com.formationds.apis.*;
import com.formationds.hadoop.FdsFileSystem;
import com.formationds.protocol.ApiException;
import com.formationds.protocol.ErrorCode;
import com.formationds.util.Action;
import com.formationds.xdi.RealAsyncAm;
import com.formationds.xdi.XdiClientFactory;
import org.junit.Before;
import org.junit.BeforeClass;

import java.util.UUID;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

public abstract class BaseAmTest {
    protected static ConfigurationService.Iface configService;
    protected String volumeName;
    protected static final int OBJECT_SIZE = 1024 * 1024 * 2;
    protected static final int MY_AM_RESPONSE_PORT = 9881;
    protected static XdiClientFactory xdiCf;
    protected static RealAsyncAm asyncAm;
    protected static XdiService.Iface amService;

    @BeforeClass
    public static void setUpOnce() throws Exception {
        xdiCf = new XdiClientFactory(MY_AM_RESPONSE_PORT);
        amService = xdiCf.remoteAmService("localhost", 9988);
        configService = xdiCf.remoteOmService("localhost", 9090);
        asyncAm = new RealAsyncAm(xdiCf.remoteOnewayAm("localhost", 8899), MY_AM_RESPONSE_PORT, 10, TimeUnit.MINUTES);
        //asyncAm.start();
    }

    @Before
    public void setUp() throws Exception {
        volumeName = UUID.randomUUID().toString();
        configService.createVolume(FdsFileSystem.DOMAIN, volumeName, new VolumeSettings(OBJECT_SIZE, VolumeType.OBJECT, 0, 0, MediaPolicy.HDD_ONLY), 0);
    }


    public void assertFdsError(ErrorCode errorCode, Action action) throws Exception {
        try {
            action.execute();
        } catch (ApiException e) {
            assertEquals(errorCode, e.getErrorCode());
            return;
        } catch (ExecutionException e) {
            Throwable cause = e.getCause();
            if (!(cause instanceof ApiException)) {
                fail("Should have gotten an ApiException");
            }
            ApiException ex = (ApiException) cause;
            assertEquals(errorCode, ex.getErrorCode());
            return;
        }

        fail("Should have gotten an ApiException");
    }
}
