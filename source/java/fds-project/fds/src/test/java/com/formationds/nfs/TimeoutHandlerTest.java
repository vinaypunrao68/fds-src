package com.formationds.nfs;

import com.formationds.xdi.TimeoutException;
import org.junit.Test;

import java.io.IOException;
import java.time.Duration;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

public class TimeoutHandlerTest {
    public static final String DOMAIN = "domain";
    public static final String VOLUME = "volume";
    public static final String BLOB = "blob";

    @Test
    public void testHandleTimeoutSuccess() throws Exception {
        AtomicInteger exceptionCount = new AtomicInteger(0);
        IoOps ops = new MemoryIoOps() {
            int invocationCount = 0;

            @Override
            public Optional<Map<String, String>> readMetadata(String domain, String volumeName, String blobName) throws IOException {
                if (invocationCount++ < 3) {
                    exceptionCount.incrementAndGet();
                    throw new TimeoutException();
                }
                return super.readMetadata(domain, volumeName, blobName);
            }
        };

        IoOps withTimeoutHandling = TimeoutHandler.buildProxy(ops, 4, Duration.ofMillis(10));
        HashMap<String, String> map = new HashMap<>();
        map.put("hello", "world");
        withTimeoutHandling.writeMetadata(DOMAIN, VOLUME, BLOB, map, false);
        Map<String, String> result = withTimeoutHandling.readMetadata(DOMAIN, VOLUME, BLOB).get();
        assertEquals(3, exceptionCount.get());
        assertEquals("world", result.get("hello"));
    }

    @Test
    public void testHandleTimeoutFailure() throws Exception {
        AtomicInteger exceptionCount = new AtomicInteger(0);

        IoOps ops = new MemoryIoOps() {
            @Override
            public Optional<Map<String, String>> readMetadata(String domain, String volumeName, String blobName) throws IOException {
                exceptionCount.incrementAndGet();
                throw new TimeoutException();
            }
        };

        IoOps withTimeoutHandling = TimeoutHandler.buildProxy(ops, 4, Duration.ofMillis(10));
        try {
            withTimeoutHandling.readMetadata(DOMAIN, VOLUME, BLOB).get();
        } catch (TimeoutException e) {
            assertEquals(4, exceptionCount.get());
            return;
        }

        fail("Should have gotten a TimeoutException!");
    }
}