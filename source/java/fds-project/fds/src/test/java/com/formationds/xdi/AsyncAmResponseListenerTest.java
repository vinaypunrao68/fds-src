package com.formationds.xdi;

import com.formationds.protocol.ApiException;
import com.formationds.apis.RequestId;
import com.formationds.apis.TxDescriptor;
import org.junit.Test;

import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

public class AsyncAmResponseListenerTest {

    @Test
    public void testTimeout() throws Exception {
        AsyncAmResponseListener listener = new AsyncAmResponseListener(1l, TimeUnit.NANOSECONDS);
        listener.start();
        RequestId requestId = new RequestId("foo");
        CompletableFuture<TxDescriptor> cf = listener.expect(requestId);
        Thread.sleep(10);
        listener.expireOldEntries();
        try {
            cf.get();
        } catch (ExecutionException e) {
            assertTrue(e.getCause()
                    .getClass()
                    .equals(ApiException.class));
            return;
        }

        fail("Should have gotten an ExecutionException!");
    }

    @Test
    public void testCallbackGetsInvoked() throws Exception {
        AsyncAmResponseListener listener = new AsyncAmResponseListener(1l, TimeUnit.DAYS);
        listener.start();
        RequestId requestId = new RequestId("foo");
        CompletableFuture<TxDescriptor> cf = listener.expect(requestId);
        listener.startBlobTxResponse(requestId, new TxDescriptor(42l));
        assertEquals(42l, cf.get()
                .getTxId());
    }

}