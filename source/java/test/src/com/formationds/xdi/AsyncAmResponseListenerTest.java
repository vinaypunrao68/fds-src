package com.formationds.xdi;

import com.formationds.apis.RequestId;
import com.formationds.apis.TxDescriptor;
import org.junit.Test;

import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import static org.junit.Assert.*;

public class AsyncAmResponseListenerTest {

    @Test
    public void testTimeout() throws Exception {
        AsyncAmResponseListener listener = new AsyncAmResponseListener(1l, TimeUnit.NANOSECONDS);
        RequestId requestId = new RequestId("foo");
        CompletableFuture<TxDescriptor> cf = listener.expect(requestId);
        Thread.sleep(1);
        listener.expect(new RequestId("bar"));
        try {
            cf.get();
        } catch (ExecutionException e) {
            assertTrue(e.getCause().getClass().equals(TimeoutException.class));
            return;
        }

        fail("Should have gotten an ExecutionException!");
    }

    @Test
    public void testCallbackGetsInvoked() throws Exception {
        AsyncAmResponseListener listener = new AsyncAmResponseListener(1l, TimeUnit.DAYS);
        RequestId requestId = new RequestId("foo");
        CompletableFuture<TxDescriptor> cf = listener.expect(requestId);
        assertFalse(cf.isDone());
        listener.startBlobTxResponse(requestId, new TxDescriptor(42l));
        assertTrue(cf.isDone());
        assertEquals(42l, cf.get().getTxId());
    }

}