package com.formationds.sc;

import com.formationds.protocol.svc.types.*;
import com.formationds.util.time.Clock;
import com.formationds.util.time.SystemClock;

import java.nio.ByteBuffer;
import java.util.*;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

// TODO: this could be made generic to handle a wide variety of situations
public class AwaitableResponseHandler implements AutoCloseable {
    private ServiceStatus status;
    private Clock clock;
    private final Map<Long, ResponseHandle> responseHandles;
    private boolean cleanupTimerStarted;

    public AwaitableResponseHandler() {
        status = ServiceStatus.SVC_STATUS_ACTIVE;
        clock = SystemClock.current();
        responseHandles = new HashMap<>();
        cleanupTimerStarted = false;
    }

    public void setClock(Clock clock) {
        this.clock = clock;
    }

    private void cleanup() {
        synchronized (responseHandles) {
            Set<Map.Entry<Long, ResponseHandle>> entries = responseHandles.entrySet();
            List<Map.Entry<Long, ResponseHandle>> toRemove = new ArrayList<>();
            for(Map.Entry<Long, ResponseHandle> entry : entries) {
                if(entry.getValue().timeout < clock.currentTimeMillis()) {
                    entry.getValue().completionHandle.completeExceptionally(new TimeoutException("wait for response timed out"));
                }

                if(entry.getValue().completionHandle.isDone())
                    toRemove.add(entry);
            }

            entries.removeAll(toRemove);
        }
    }

    private void startCleanupTimer() {
        if(!cleanupTimerStarted) {
            cleanupTimerStarted = true;
            cleanupCycle();
        }
    }

    private void cleanupCycle() {
        cleanup();
        if(cleanupTimerStarted)
            clock.delay(1000, TimeUnit.MILLISECONDS).thenRunAsync(this::cleanupCycle);
    }

    public CompletableFuture<AsyncSvcResponse> awaitResponse(long msgId, long timeout, TimeUnit timeoutUnits) {
        ResponseHandle handle = new ResponseHandle();
        CompletableFuture<AsyncSvcResponse> cf = new CompletableFuture<>();

        handle.completionHandle = cf;
        handle.timeout = clock.currentTimeMillis() + TimeUnit.MILLISECONDS.convert(timeout, timeoutUnits);
        handle.id = msgId;

        synchronized (responseHandles) {
            if(responseHandles.containsKey(msgId))
                    throw new IllegalArgumentException("already waiting for message with the supplied id");

            startCleanupTimer();
            responseHandles.put(msgId, handle);
        }

        return cf;
    }

    public CompletableFuture<AsyncSvcResponse> awaitResponse(long msgId) {
        return awaitResponse(msgId, 30, TimeUnit.SECONDS);
    }

    public void recieveResponse(AsyncHdr asyncHdr, ByteBuffer payload) {
        synchronized (responseHandles) {
            ResponseHandle responseHandle = responseHandles.get(asyncHdr.msg_src_id);
            if(asyncHdr.getMsg_code() == 0)
                responseHandle.completionHandle.complete(new AsyncSvcResponse(asyncHdr, payload));
            else
                responseHandle.completionHandle.completeExceptionally(new SvcException(asyncHdr.getMsg_code()));

        }
    }

    @Override
    public void close() throws Exception {
        cleanupTimerStarted = false;
    }

    private class ResponseHandle {
        long id;
        long timeout;
        CompletableFuture<AsyncSvcResponse> completionHandle;
    }
}
