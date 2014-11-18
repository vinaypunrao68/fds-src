/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

import com.formationds.commons.model.exception.UnsupportedMetricException;
import com.formationds.commons.model.i18n.ModelResource;

import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;

/**
 * @author ptinius
 */
public enum Metrics {
    PUTS( "Puts" ),
    GETS( "Gets" ),
    QFULL( "Queue Full" ),
    /**
     * @deprecated use {@link #SSD_GETS}
     */
    @Deprecated
    PSSDA( "Percent of SSD Accesses" ),
    MBYTES( "Metadata Bytes" ),
    BLOBS( "Blobs" ),
    OBJECTS( "Objects" ),
    ABS( "Ave Blob Size" ),
    AOPB( "Ave Objects per Blob" ),
    // System usage ( sum the most recent of each volume
    LBYTES( "Logical Bytes" ),
    PBYTES( "Physical Bytes" ),
    // firebreak metrics
    STC_SIGMA( "Short Term Capacity Sigma" ),
    LTC_SIGMA( "Long Term Capacity Sigma" ),
    STP_SIGMA( "Short Term Perf Sigma" ),
    LTP_SIGMA( "Long Term Perf Sigma" ),
    // Performance IOPS
    STC_WMA( "Short Term Capacity WMA" ),
    LTC_WMA( "Long Term Capacity WMA" ),
    STP_WMA( "Short Term Perf WMA" ),
    LTP_WMA( "Long Term Perf WMA" ),
    /**
     * gets from SSD and cache
     */
    SSD_GETS( "SSD Gets" );

    public static EnumSet<Metrics> FIREBREAK = EnumSet.of(Metrics.STC_SIGMA,
                                                          Metrics.LTC_SIGMA,
                                                          Metrics.STP_SIGMA,
                                                          Metrics.LTP_SIGMA);

    public static EnumSet<Metrics> FIREBREAK_CAPACITY = EnumSet.of(Metrics.STC_SIGMA,
                                                                   Metrics.LTC_SIGMA);

    public static EnumSet<Metrics> FIREBREAK_PERFORMANCE = EnumSet.of(Metrics.STP_SIGMA,
                                                                      Metrics.LTP_SIGMA);

    public static EnumSet<Metrics> PERFORMANCE = EnumSet.of(Metrics.STP_WMA);

    public static final EnumSet<Metrics> CAPACITY = EnumSet.of(Metrics.PBYTES,
                                                               Metrics.LBYTES);

    private static final String UNKNOWN_METRIC =
        ModelResource.getString( "model.metrics.unsupported" );

    private final String key;

    /**
     * @param key the {@link String} representing the metadata key
     */
    Metrics( String key ) {
        this.key = key;
    }

    /**
     * @return Returns{@link String} representing the metadata key
     */
    public String key() {
        return this.key;
    }

    /**
     * @param metadataKey the case sensitive {@link String} representing the
     *                    metadata key name
     *
     * @return Returns {@link com.formationds.commons.model.type.Metrics}
     * associated with {@code metadataKey}
     *
     * @throws UnsupportedMetricException if the the {@code metadataKey} is not
     *                                    found
     */
    public static Metrics byMetadataKey( final String metadataKey )
        throws UnsupportedMetricException {
        for( final Metrics m : values() ) {
            if( m.key()
                 .equals( metadataKey ) ) {
                return m;
            }
        }

        throw new UnsupportedMetricException( String.format( UNKNOWN_METRIC,
                                                             metadataKey ) );
    }

}
