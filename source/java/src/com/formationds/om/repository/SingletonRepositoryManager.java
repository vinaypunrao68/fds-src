/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository;

/**
 * @author ptinius
 */
public class SingletonRepositoryManager {
    private static SingletonRepositoryManager instance = null;

    /**
     * singleton instance of SingletonMetricsRepository
     */
    public static SingletonRepositoryManager instance() {
        if (instance == null) {
            instance = new SingletonRepositoryManager();
        }

        return instance;
    }

    /**
     * singleton default constructor
     */
    private SingletonRepositoryManager() {
    }

    private MetricsRepository metricsRepository = null;

    /**
     * @return Returns the {@link com.formationds.om.repository.MetricsRepository}
     */
    public MetricsRepository getMetricsRepository() {
        if (metricsRepository == null) {
            setMetricsRepository(new MetricsRepository());
        }

        return metricsRepository;
    }

    /**
     * @param metricsRepository the {@link com.formationds.om.repository.MetricsRepository}
     */
    public void setMetricsRepository(final MetricsRepository metricsRepository) {
        this.metricsRepository = metricsRepository;
    }
}
