/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository;

import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.repository.influxdb.InfluxMetricRepository;
import com.formationds.om.repository.influxdb.InfluxRepository;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.Properties;

/**
 * Factory to access to Metrics and Events repositories.
 *
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
                // TODO: hide this in the repo interface (ie. just make it take the user/creds.  dbname is held by repo)
                Properties props = new Properties( );
                props.setProperty( InfluxRepository.CP_DBNAME, InfluxMetricRepository.DEFAULT_METRIC_DB.getName() );
                props.setProperty( InfluxRepository.CP_USER, "root" );
                props.setProperty( InfluxRepository.CP_CRED, "root" );

                influxMetricRepository.open(props);
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

    /**
     * Initialize the metric repository.  The implementation returned is dependent on the settings
     * of feature toggles:
     * <ul>
     *     <li>PERSIST_OBJECTDB</li>
     *     <li>PERSIST_INFLUXDB</li>
     *     <LI>QUERY_INFLUXDB</LI>
     * </ul>
     */
    protected void initializeMetricRepository() {
        metricsRepository = MetricRepositoryProxyIH.newMetricsProxy();
    }

    /**
     * Initialize the event repository.  The implementation returned is dependent on the settings
     * of feature toggles:
     * <ul>
     *     <li>PERSIST_OBJECTDB</li>
     *     <li>PERSIST_INFLUXDB</li>
     *     <LI>QUERY_INFLUXDB</LI>
     * </ul>
     */
    protected void initializeEventRepository() {
        // TODO: fs-1111 use toggles on event repository too
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
