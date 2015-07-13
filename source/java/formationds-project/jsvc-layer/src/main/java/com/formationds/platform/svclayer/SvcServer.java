/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.platform.svclayer;

import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

public class SvcServer {

    /**
     *
     */
    public class ServerLifecycleException extends RuntimeException {
        public ServerLifecycleException( String message, Throwable cause ) {
            super( message, cause );
        }

        public ServerLifecycleException( String message ) {
            super( message );
        }
    }

    // TODO: not yet used anywhere
    public enum SvcServerLifecycleEvent {
        STARTING,
        STARTED,
        STOP_REQUESTED,
        STOPPED,
        ERROR;
    }

    // TODO: Not yet used anywhere
    public interface SvcServerListener {
        void serverLifecycleEvent(SvcServerLifecycleEvent event);
        void errorEvent(Throwable t);
    }

    private List<SvcServerListener> listeners;
    private ExecutorService executorService;

    /**
     * Add a server event listener
     * @param l the listener
     * @return this
     */
    public SvcServer addListener(SvcServerListener l) {
        listeners.add( l );
        return this;
    }

    /**
     * Add a server event listener
     * @param l the listener
     * @return this
     */
    public SvcServer removeListener(SvcServerListener l) {
        listeners.remove( l );
        return this;
    }

    /**
     * Start the server
     * <p/>
     * This method is idempotent.  If the server is already running it will silently return.
     *
     * @throws ServerLifecycleException if an error occurs starting the server
     */
    public void startAndWait(long timeout, TimeUnit unit) throws ServerLifecycleException, InterruptedException,
                                                                  TimeoutException {
        try {
            Future<Void> f = start();
            if (timeout <= 0) {
                f.get();
            } else {
                f.get(timeout, unit);
            }
        } catch (ExecutionException ee) {
            Throwable cause = ee.getCause();
            if (cause instanceof ServerLifecycleException) {
                throw (ServerLifecycleException)cause;
            } else {
                throw new ServerLifecycleException( "Failed to start server.", ee.getCause() );
            }
        }
    }

    /**
     * Start the server and return a future that will be completed once the server is fully started
     * and accepting connections.
     *
     * @return a Future that will be completed once the server is fully started and accepting connections
     */
    public Future<Void> start() {
        CompletableFuture<Void> completableFuture = new CompletableFuture<Void>();
        // TODO: not yet implemented
        completableFuture.completeExceptionally( new ServerLifecycleException( "START NOT IMPLEMENTED" ) );
        return completableFuture;
    }

    /**
     *
     * @throws ServerLifecycleException
     * @throws InterruptedException
     */
    public void stopAndWait(long timeout, TimeUnit unit)  throws ServerLifecycleException, InterruptedException, TimeoutException {
        try {
            Future<Void> f = stop();
            if (timeout <= 0) {
                f.get();
            } else {
                f.get(timeout, unit);
            }
        } catch (ExecutionException ee) {
            throw new ServerLifecycleException( "Failed to start server.", ee.getCause() );
        }
    }

    /**
     * Stop the server.
     * <p/>
     * This method is idempotent.  If the server is already stopped it will silently return without
     * modifying the state of the server and no error is returned.
     *
     * @return a Future that will be completed once the server is fully stopped.
     *
     * @throws ServerLifecycleException if an error occurs stopping the server
     */
    public Future<Void> stop() throws ServerLifecycleException {
        CompletableFuture<Void> completableFuture = new CompletableFuture<Void>();
        // TODO: not yet implemented
        completableFuture.completeExceptionally( new ServerLifecycleException( "STOP NOT IMPLEMENTED" ) );
        return completableFuture;
    }
}
