package com.formationds.om.plotter;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.jfree.chart.ChartFactory;
import org.jfree.chart.JFreeChart;
import org.jfree.data.time.FixedMillisecond;
import org.jfree.data.time.TimeSeries;
import org.jfree.data.time.TimeSeriesCollection;
import org.jfree.data.xy.XYDataset;

import java.awt.image.BufferedImage;
import java.util.HashMap;
import java.util.Map;

public class VolumeStatsPlotter {

    public BufferedImage plot(String volumeName, VolumeDatapoint[] datapoints) {
        JFreeChart chart = ChartFactory.createTimeSeriesChart(
                "Statistics for volume " + volumeName,
                "Time",
                "Value",
                createDataset(datapoints),
                true,
                true,
                true
        );

        return chart.createBufferedImage(800, 500);
    }

    private XYDataset createDataset(VolumeDatapoint[] datapoints) {
        Map<String, TimeSeries> byKey = new HashMap<>();

        for (VolumeDatapoint datapoint : datapoints) {
            TimeSeries timeSeries = byKey.compute(datapoint.getKey(), (v, t) -> {
                if (t == null) {
                    return new TimeSeries(datapoint.getKey());
                } else {
                    return t;
                }
            });

            timeSeries.addOrUpdate(new FixedMillisecond(datapoint.getDateTime().getMillis()), datapoint.getValue());
        }

        TimeSeriesCollection dataset = new TimeSeriesCollection();

        for (String key : byKey.keySet()) {
            dataset.addSeries(byKey.get(key));
        }

        return dataset;
    }
}
