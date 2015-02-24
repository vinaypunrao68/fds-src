package com.formationds.xdi;

import com.formationds.apis.ApiException;
import com.formationds.apis.AsyncAmServiceRequest;
import com.formationds.apis.ErrorCode;
import com.formationds.apis.RequestId;
import org.junit.Test;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;

public class RealAsyncAmTest {
    @Test
    public void testTimeoutsBubbleUp() throws Exception {
        RealAsyncAm asyncAm = new RealAsyncAm(mock(AsyncAmServiceRequest.Iface.class), 10000, 100, TimeUnit.MILLISECONDS);
        asyncAm.start(false);

        try {
            asyncAm.statBlob("foo", "bar", "hello").get();
        } catch (ExecutionException e) {
            if (e.getCause() instanceof ApiException) {
                assertEquals(ErrorCode.INTERNAL_SERVER_ERROR, ((ApiException) e.getCause()).getErrorCode());
                return;
            }
        }

        fail("Should have gotten an ExecutionException!");
    }

    @Test
    public void testThriftErrorsAreBubbled() throws Exception {
        AsyncAmServiceRequest.Iface requestService = mock(AsyncAmServiceRequest.Iface.class);
        doThrow(new IllegalStateException()).when(requestService).statBlob(any(RequestId.class), anyString(), anyString(), anyString());
        RealAsyncAm asyncAm = new RealAsyncAm(requestService, 10000, 100, TimeUnit.MILLISECONDS);
        asyncAm.start(false);

        try {
            asyncAm.statBlob("foo", "bar", "hello").get();
        } catch (ExecutionException e) {
            if (e.getCause() instanceof IllegalStateException) {
                return;
            }
        }

        fail("Should have gotten an IllegalStateException!");

    }
}