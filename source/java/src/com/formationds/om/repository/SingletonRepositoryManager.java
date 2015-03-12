/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository;

import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.repository.influxdb.InfluxMetricRepository;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.List;

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

    /**
     * An invocation handler for dynamic proxies to the MetricRepository interface
     */
    static class MetricRepositoryProxyIH implements InvocationHandler {

        /**
         * @return a proxy to the MetricRepository.
         *
         * @throws IllegalStateException if no database implementation is enabled.
         */
        public static MetricRepository newMetricsProxy() {
            boolean influxEnabled = FdsFeatureToggles.PERSIST_INFLUXDB.isActive();
            boolean objectDbEnabled = FdsFeatureToggles.PERSIST_OBJECTDB.isActive();
            boolean influxQueryEnabled = FdsFeatureToggles.QUERY_INFLUXDB.isActive();

            if ( !(influxEnabled || objectDbEnabled) ) {
                throw new IllegalStateException( "At least one of the metric database implementations must be enabled." );
            }

            JDOMetricsRepository jdoMetricsRepository = null;
            InfluxMetricRepository influxMetricRepository = null;

            if ( objectDbEnabled ) {
                jdoMetricsRepository = new JDOMetricsRepository();
            }

            if ( influxEnabled ) {
                // TODO: need values from config
                influxMetricRepository = new InfluxMetricRepository( "http://localhost:8086",
                                                                     "root",
                                                                     "root".toCharArray() );
            }

            return (MetricRepository) Proxy.newProxyInstance( MetricRepository.class.getClassLoader(),
                                                              new Class[] { MetricRepository.class },
                                                              new MetricRepositoryProxyIH( jdoMetricsRepository,
                                                                                           influxMetricRepository,
                                                                                           influxQueryEnabled ) );
        }

        private final JDOMetricsRepository jdoMetricsRepository;
        private final InfluxMetricRepository influxMetricRepository;
        private final boolean influxQueryEnabled;

        public MetricRepositoryProxyIH( JDOMetricsRepository jdoMetricsRepository,
                                        InfluxMetricRepository influxMetricRepository, boolean influxQueryEnabled ) {
            this.jdoMetricsRepository = jdoMetricsRepository;
            this.influxMetricRepository = influxMetricRepository;
            this.influxQueryEnabled = influxQueryEnabled;
        }

        @Override
        public Object invoke( Object proxy, Method method, Object[] args ) throws Throwable {

            switch ( method.getName() )
            {
                case "save":
                    List<VolumeDatapoint> results = null;
                    if ( jdoMetricsRepository != null ) {
                        results = (List<VolumeDatapoint>)method.invoke( jdoMetricsRepository, args );
                    }

                    if ( influxMetricRepository != null ) {

                        List<VolumeDatapoint> influxResult = (List<VolumeDatapoint>)method.invoke( influxMetricRepository, args );
                        if (jdoMetricsRepository == null) {
                            // use the results from the influx repo
                            results = influxResult;
                        }
                    }

                    return results;

                case "query":

                    if ( influxMetricRepository != null && influxQueryEnabled ) {
                        return method.invoke( influxMetricRepository, args );
                    } else {
                        return method.invoke( jdoMetricsRepository, args );
                    }

                default:
                    // everything else allow to pass-through on the JDO repository for now
                    return method.invoke( jdoMetricsRepository, args );
            }
        }
    }

    private MetricRepository metricsRepository = null;
    private EventRepository eventRepository = null;

    /**
     * @throws IllegalStateException if no database implementation is enabled, or if already initialized
     */
    synchronized public void initializeRepositories() {

        if ( metricsRepository != null && eventRepository != null ) {
            throw new IllegalStateException( "Repositories are already initialized.  Can not re-initialize them." );
        }

        initializeMetricRepository( );
        initializeEventRepository( );
    }

    protected void initializeMetricRepository() {
        metricsRepository = MetricRepositoryProxyIH.newMetricsProxy();
    }

    protected void initializeEventRepository() {
        // TODO: use toggles on event repository too
        eventRepository = new JDOEventRepository( );
    }


    /**
     * @return the {@link EventRepository}
     */
    public EventRepository getEventRepository() {
        return eventRepository;
    }

    /**
     * @return Returns the {@link MetricRepository}
     */
    public MetricRepository getMetricsRepository() {
        if ( metricsRepository == null ) {
            this.metricsRepository = new JDOMetricsRepository();
        }

        return metricsRepository;
    }
}
