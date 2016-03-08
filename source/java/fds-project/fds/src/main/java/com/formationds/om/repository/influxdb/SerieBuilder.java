package com.formationds.om.repository.influxdb;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Function;

import org.influxdb.dto.Serie;

/**
 * Wrap a influxdb-java Serie.Builder to track the row count and reset the builder
 *
 * This class is not thread safe.
 */
public class SerieBuilder {

    private final Function<VolumeInfo,String> seriesNameFunction;
    private final List<String> columns;
    private Map<VolumeInfo,Serie.Builder> serieBuilders = new HashMap<>();
    private int rowCount = 0;

    public SerieBuilder( Function<VolumeInfo,String> seriesNameFunction,
                         List<String> columns ) {
        super();
        this.seriesNameFunction = seriesNameFunction;
        this.columns = columns;
    }

    public int getRowCount() {
        return rowCount;
    }

    private Serie.Builder getSerieBuilder(VolumeInfo vinfo) {
        Serie.Builder b = serieBuilders.computeIfAbsent( vinfo, (v) -> {
            return new Serie.Builder( seriesNameFunction.apply( vinfo ) )
                    .columns( columns.toArray( new String[columns.size()] ) );
        } );
        return b;
    }

    /**
     *
     * @param metricValues
     * @return the current number of rows after addition
     */
    public int addRow( VolumeInfo v, Object[] metricValues ) {
        getSerieBuilder(v).values( metricValues );
        return ++rowCount;
    }

    /**
     * Build the series from the current set of rows and reset the builder.
     *
     * @return the series.
     */
    public Serie[] build() {
        Serie[] s = new Serie[ serieBuilders.size() ];
        int i = 0;
        for (Serie.Builder sb : serieBuilders.values() ) {
            s[i++] = sb.build();
        }
        serieBuilders.clear();
        rowCount = 0;
        return s;
    }
}