package com.formationds.nbd;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import io.netty.buffer.ByteBuf;
import org.apache.log4j.Logger;
import org.apache.log4j.LogManager;

import java.io.IOException;
import java.util.concurrent.CompletableFuture;

public class LoggingOperationsWrapper implements NbdServerOperations {
    public static final String LOGGER_NAME = LoggingOperationsWrapper.class.getPackage().getName() + ".NBDOperations";
    private static final Logger log = LogManager.getLogger( LOGGER_NAME );
    private NbdServerOperations inner;
    private long interactionId;


    public LoggingOperationsWrapper(NbdServerOperations inner) throws IOException {
        this.inner = inner;
        interactionId = 0L;
    }

    private String formatLogMessage(String message, long interactionId) {
        return message + " [" + interactionId + "]";
    }

    private synchronized long getInteractionId() {
        return interactionId++;
    }


    @Override
    public boolean exists(String exportName) {
        long interactionId = getInteractionId();
        log.info(formatLogMessage("EXISTS START " + exportName, interactionId));
        boolean result = inner.exists(exportName);
        log.info(formatLogMessage("EXISTS OK " + exportName + " RESULT " + result, interactionId));
        return result;
    }

    @Override
    public long size(String exportName) {
        long interactionId = getInteractionId();
        log.info(formatLogMessage("SIZE START " + exportName, interactionId));
        long size = inner.size(exportName);
        log.info(formatLogMessage("SIZE OK " + exportName + " RESULT " + size, interactionId));
        return size;
    }

    @Override
    public CompletableFuture<Void> read(String exportName, ByteBuf target, long offset, int len) throws Exception {
        long interactionId = getInteractionId();
        log.info(formatLogMessage("READ START " + exportName + " offset=" + offset + " length=" + len, interactionId));
        CompletableFuture<Void> result = new CompletableFuture<>();
        CompletableFuture<Void> innerResult = inner.read(exportName, target, offset, len);
        innerResult.handle((r, exception) -> {
            if(exception != null) {
                log.error(formatLogMessage("READ FAIL " + exportName + " offset=" + offset + " length=" + len, interactionId), exception);
                result.completeExceptionally(exception);
            } else {
                log.info(formatLogMessage("READ OK " + exportName + " offset=" + offset + " length=" + len, interactionId));
                result.complete(null);
            }
            return null;
        });
        return result;
    }

    @Override
    public CompletableFuture<Void> write(String exportName, ByteBuf source, long offset, int len) throws Exception {
        long interactionId = getInteractionId();
        log.info(formatLogMessage("WRITE START " + exportName + " offset=" + offset + " length=" + len, interactionId));
        CompletableFuture<Void> result = new CompletableFuture<>();
        CompletableFuture<Void> innerResult = inner.write(exportName, source, offset, len);
        innerResult.handle((r, exception) -> {
            if(exception != null) {
                log.error(formatLogMessage("WRITE FAIL " + exportName + " " + " offset=" + offset + " length=" + len, interactionId), exception);
                result.completeExceptionally(exception);
            } else {
                log.info(formatLogMessage("WRITE OK " + exportName + " " + " offset=" + offset + " length=" + len, interactionId));
                result.complete(null);
            }
            return null;
        });
        return result;
    }

    @Override
    public CompletableFuture<Void> flush(String exportName) {
        long interactionId = getInteractionId();
        log.info(formatLogMessage("FLUSH START " + exportName, interactionId));
        return inner.flush(exportName).whenComplete((r, ex) -> {
            if(ex != null) {
                log.error(formatLogMessage("FLUSH FAIL " + exportName, interactionId), ex);
            } else {
                log.info(formatLogMessage("FLUSH OK " + exportName, interactionId));
            }
        });
    }
}
