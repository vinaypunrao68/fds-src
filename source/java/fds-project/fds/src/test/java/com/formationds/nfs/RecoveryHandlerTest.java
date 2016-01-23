package com.formationds.nfs;

import com.formationds.apis.ObjectOffset;
import com.formationds.xdi.RecoverableException;
import org.joda.time.Duration;
import org.junit.Test;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.atomic.AtomicInteger;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

public class RecoveryHandlerTest {
    public static final String DOMAIN = "domain";
    public static final String VOLUME = "volume";
    public static final String BLOB = "blob";

    @Test
    public void testHandleRecoverySuccess() throws Exception {
        AtomicInteger exceptionCount = new AtomicInteger(0);
        IoOps ops = new MemoryIoOps() {
            int invocationCount = 0;

            @Override
            public Optional<FdsMetadata> readMetadata(String domain, String volumeName, String blobName) throws IOException {
                if (invocationCount++ < 3) {
                    exceptionCount.incrementAndGet();
                    throw new RecoverableException();
                }
                return super.readMetadata(domain, volumeName, blobName);
            }
        };

        IoOps withTimeoutHandling = new RecoveryHandler(ops, 4, Duration.millis(10));
        HashMap<String, String> map = new HashMap<>();
        map.put("hello", "world");
        withTimeoutHandling.writeMetadata(DOMAIN, VOLUME, BLOB, new FdsMetadata(map), false);
        Map<String, String> result = withTimeoutHandling.readMetadata(DOMAIN, VOLUME, BLOB).get().lock(m -> m.mutableMap());
        assertEquals(3, exceptionCount.get());
        assertEquals("world", result.get("hello"));
    }

    @Test(expected = FileNotFoundException.class)
    public void testBubbleExceptions() throws Exception {
        IoOps ops = new MemoryIoOps() {
            @Override
            public FdsObject readCompleteObject(String domain, String volumeName, String blobName, ObjectOffset objectOffset, int maxObjectSize) throws IOException {
                throw new FileNotFoundException();
            }
        };

        IoOps withTimeoutHandling = new RecoveryHandler(ops, 10, Duration.ZERO);
        withTimeoutHandling.readCompleteObject("foo", "bar", "hello", new ObjectOffset(0), 42);
    }

    @Test
    public void testHandleRecoveryFailure() throws Exception {
        AtomicInteger exceptionCount = new AtomicInteger(0);

        IoOps ops = new MemoryIoOps() {
            @Override
            public Optional<FdsMetadata> readMetadata(String domain, String volumeName, String blobName) throws IOException {
                exceptionCount.incrementAndGet();
                throw new RecoverableException();
            }
        };

        IoOps withTimeoutHandling = new RecoveryHandler(ops, 4, Duration.millis(10));
        try {
            withTimeoutHandling.readMetadata(DOMAIN, VOLUME, BLOB).get();
        } catch (IOException e) {
            assertEquals(4, exceptionCount.get());
            return;
        }

        fail("Should have gotten a TimeoutException!");
    }
}