/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository;

/**
 * @author ptinius
 */
public enum SingletonRepositoryManager {

    instance;

    /**
     * singleton instance of SingletonMetricsRepository
     */
    public static SingletonRepositoryManager instance() {
        return instance;
    }

    private MetricsRepository metricsRepository = null;
    private final EventRepository eventRepository = new EventRepository();

    /**
     * @return the {@link com.formationds.om.repository.EventRepository}
     */
    public EventRepository getEventRepository() { return eventRepository; }

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
