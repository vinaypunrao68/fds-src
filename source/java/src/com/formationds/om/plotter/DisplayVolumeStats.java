package com.formationds.om.plotter;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.jfree.chart.ChartUtilities;

import java.awt.image.BufferedImage;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Map;

/**
 * @deprecated the POC functionality has been replaced by
 *             {@link com.formationds.om.rest.metrics.QueryMetrics}
 *
 *             will be removed in the future
 */
@Deprecated
public class DisplayVolumeStats implements RequestHandler {
    private VolumeStatistics volumeStatistics;

    public DisplayVolumeStats(VolumeStatistics volumeStatistics) {
        this.volumeStatistics = volumeStatistics;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String volumeName = routeParameters.get("volume");
        VolumeDatapoint[] datapoints = volumeStatistics.datapoints(volumeName);
        BufferedImage plot = new VolumeStatsPlotter().plot(volumeName, datapoints);
        byte[] bytes = ChartUtilities.encodeAsPNG(plot);

        return new Resource() {
            @Override
            public String getContentType() {
                return "image/png";
            }

            @Override
            public void render(OutputStream outputStream) throws IOException {
                outputStream.write(bytes);
                outputStream.flush();
            }
        };
    }
}
