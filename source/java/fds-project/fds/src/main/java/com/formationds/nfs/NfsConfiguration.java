package com.formationds.nfs;

public class NfsConfiguration {
    private int threadPoolSize = 64;
    private int workQueueSize = 2000;
    private long incomingRequestTimeoutSeconds = 120;
    private boolean activateStats;
    private boolean deferMetadataUpdates;

    public NfsConfiguration(int threadPoolSize, int threadPoolWorkQueueSize, long incomingRequestTimeoutSeconds, boolean activateStats, boolean deferMetadataUpdates) {
        this.threadPoolSize = threadPoolSize;
        this.workQueueSize = threadPoolWorkQueueSize;
        this.incomingRequestTimeoutSeconds = incomingRequestTimeoutSeconds;
        this.activateStats = activateStats;
        this.deferMetadataUpdates = deferMetadataUpdates;
    }

    public int getThreadPoolSize() {
        return threadPoolSize;
    }

    public int getWorkQueueSize() {
        return workQueueSize;
    }

    public long getIncomingRequestTimeoutSeconds() {
        return incomingRequestTimeoutSeconds;
    }

    public boolean deferMetadataUpdates() {
        return deferMetadataUpdates;
    }

    @Override
    public String toString() {
        return "NfsConfiguration{" +
                "threadPoolSize=" + threadPoolSize +
                ", workQueueSize=" + workQueueSize +
                ", incomingRequestTimeoutSeconds=" + incomingRequestTimeoutSeconds +
                '}';
    }

    public boolean activateStats() {
        return activateStats;
    }
}
