package com.formationds.sc;

import com.formationds.protocol.svc.PlatNetSvc;
import com.formationds.protocol.svc.UpdateSvcMapMsg;
import com.formationds.protocol.svc.types.*;
import com.formationds.util.time.Clock;
import com.formationds.util.time.SystemClock;
import org.apache.thrift.TException;

import java.nio.ByteBuffer;
import java.util.*;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.stream.Collectors;

// TODO: this could be made generic to handle a wide variety of situations
public class AwaitableResponseHandler {
    private ServiceStatus status;
    private Clock clock;
    private final Map<Long, ResponseHandle> responseHandles;

    private Timer cleanupTimer;

    public AwaitableResponseHandler() {
        status = ServiceStatus.SVC_STATUS_ACTIVE;
        clock = new SystemClock();
        responseHandles = new HashMap<>();
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
        if(cleanupTimer == null) {
            cleanupTimer = new Timer(true);
            cleanupTimer.schedule(new TimerTask() {
                @Override
                public void run() {
                    cleanup();
                }
            }, 1000L, 1000L);
        }
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
            responseHandle.completionHandle.complete(new AsyncSvcResponse(asyncHdr, payload));
        }
    }

    private class ResponseHandle {
        long id;
        long timeout;
        CompletableFuture<AsyncSvcResponse> completionHandle;
    }
}
