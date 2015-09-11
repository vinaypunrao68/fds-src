package com.formationds.nfs;

public class NfsConfiguration {
    private int threadPoolSize = 64;
    private int workQueueSize = 2000;
    private long incomingRequestTimeoutSeconds = 120;
    private boolean activateStats;

    public NfsConfiguration(int threadPoolSize, int threadPoolWorkQueueSize, long incomingRequestTimeoutSeconds, boolean activateStats) {
        this.threadPoolSize = threadPoolSize;
        this.workQueueSize = threadPoolWorkQueueSize;
        this.incomingRequestTimeoutSeconds = incomingRequestTimeoutSeconds;
        this.activateStats = activateStats;
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
