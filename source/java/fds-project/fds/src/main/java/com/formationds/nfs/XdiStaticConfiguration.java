package com.formationds.nfs;

import org.joda.time.Duration;

public class XdiStaticConfiguration {
    private int threadPoolSize = 64;
    private boolean activateStats;
    private boolean deferMetadataUpdates;
    private int maxLiveNfsCookies;
    private int amTimeoutSeconds;
    private int amRetryAttempts;
    private int amRetryIntervalSeconds;
    private int statsPort;

    public XdiStaticConfiguration(int threadPoolSize,
                                  boolean activateStats,
                                  boolean deferMetadataUpdates,
                                  int maxLiveNfsCookies,
                                  int amTimeoutSeconds,
                                  int amRetryAttempts,
                                  int amRetryIntervalSeconds,
                                  int statsPort) {
        this.threadPoolSize = threadPoolSize;
        this.activateStats = activateStats;
        this.deferMetadataUpdates = deferMetadataUpdates;
        this.maxLiveNfsCookies = maxLiveNfsCookies;
        this.amTimeoutSeconds = amTimeoutSeconds;
        this.amRetryAttempts = amRetryAttempts;
        this.amRetryIntervalSeconds = amRetryIntervalSeconds;
        this.statsPort = statsPort;
    }

    public int getStatsPort( )
    {
        return statsPort;
    }

    public int getThreadPoolSize( ) {
        return threadPoolSize;
    }

    public boolean deferMetadataUpdates() {
        return deferMetadataUpdates;
    }

    public boolean activateStats() {
        return activateStats;
    }

    public int getMaxLiveNfsCookies() {
        return maxLiveNfsCookies;
    }

    public Duration getAmTimeout() {
        return Duration.standardSeconds(amTimeoutSeconds);
    }

    public Duration getAmRetryInterval() {
        return Duration.standardSeconds(amRetryIntervalSeconds);
    }

    public int getAmRetryAttempts() {
        return amRetryAttempts;
    }
}
