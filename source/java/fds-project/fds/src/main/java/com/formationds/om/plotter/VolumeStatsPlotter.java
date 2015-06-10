package com.formationds.om.plotter;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.jfree.chart.ChartFactory;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.block.BlockBorder;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.renderer.xy.XYItemRenderer;
import org.jfree.data.time.FixedMillisecond;
import org.jfree.data.time.TimeSeries;
import org.jfree.data.time.TimeSeriesCollection;
import org.jfree.data.xy.XYDataset;

import java.awt.*;
import java.awt.image.BufferedImage;
import java.util.HashMap;
import java.util.Map;

/**
 * @deprecated the POC functionality has been replaced the UI now creates
 *             all graphs.
 *
 *             will be removed in the future
 */
@Deprecated
public class VolumeStatsPlotter {

    public BufferedImage plot(String volumeName, VolumeDatapoint[] datapoints) {
        XYDataset dataset = createDataset(datapoints);
        JFreeChart chart = ChartFactory.createTimeSeriesChart(
                "Statistics for volume " + volumeName,
                "Time",
                "Value",
                dataset,
                true,
                true,
                true
        );

        XYPlot plot = (XYPlot) chart.getPlot();
        plot.getRangeAxis().setAxisLineVisible(false);
        plot.getDomainAxis().setAxisLineVisible(false);
        plot.setOutlineVisible(false);
        plot.setBackgroundPaint(Color.WHITE);
        plot.setDomainGridlinePaint(Color.lightGray);
        plot.setRangeGridlinePaint(Color.lightGray);

        XYItemRenderer renderer = plot.getRenderer();
        for (int i = 0; i < dataset.getSeriesCount(); i++) {
            renderer.setSeriesStroke(i, new BasicStroke(3f, BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND));
        }
        chart.getLegend().setFrame(BlockBorder.NONE);

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
