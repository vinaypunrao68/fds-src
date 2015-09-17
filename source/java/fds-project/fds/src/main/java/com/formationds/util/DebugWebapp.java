package com.formationds.util;

import com.formationds.nfs.Counters;
import com.formationds.protocol.BlobDescriptor;
import com.formationds.protocol.BlobListOrder;
import com.formationds.protocol.PatternSemantics;
import com.formationds.web.toolkit.*;
import com.formationds.xdi.AsyncAm;
import com.formationds.xdi.XdiConfigurationApi;
import com.google.common.collect.EvictingQueue;
import org.eclipse.jetty.server.Request;
import org.jfree.chart.ChartFactory;
import org.jfree.chart.ChartUtilities;
import org.jfree.chart.JFreeChart;
import org.jfree.chart.plot.PlotOrientation;
import org.jfree.chart.plot.XYPlot;
import org.jfree.data.xy.XYSeries;
import org.jfree.data.xy.XYSeriesCollection;
import org.json.JSONArray;
import org.json.JSONObject;

import java.awt.*;
import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class DebugWebapp {

    public static final int WINDOW_SIZE_IN_SECONDS = 60 * 5; // 5 mns

    public void start(int serverPort, AsyncAm asyncAm, XdiConfigurationApi configurationApi, Counters counters) {
        WebApp webApp = new WebApp(".");
        // 10mns window
        EvictingQueue<Map<Counters.Key, Long>> window = EvictingQueue.create(WINDOW_SIZE_IN_SECONDS);
        webApp.route(HttpMethod.GET, "/list/:volume", () -> new ListVolume(asyncAm));
        webApp.route(HttpMethod.GET, "/stats/:key", () -> new GraphOneCounter(window));
        webApp.route(HttpMethod.GET, "/stats", () -> new AllCounters());
        new Thread(() -> {
            while (true) {
                window.add(counters.harvest());
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    break;
                }
            }
        }).start();
        webApp.start(new HttpConfiguration(serverPort));
    }

    class AllCounters implements RequestHandler {
        @Override
        public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
            return new Resource() {
                @Override
                public String getContentType() {
                    return "text/html";
                }

                @Override
                public void render(OutputStream outputStream) throws IOException {
                    StringBuilder sb = new StringBuilder();
                    sb.append("<html><head><title>NFS statistics</title></head><body>");
                    for (Counters.Key key : Counters.Key.values()) {
                        sb.append("<img src=\"/stats/" + key.name() + "\" /><br /><br />\n");
                    }
                    sb.append("</body></html>");
                    OutputStreamWriter osw = new OutputStreamWriter(outputStream);
                    osw.write(sb.toString());
                    osw.flush();
                }
            };
        }
    }

    class GraphOneCounter implements RequestHandler {
        private EvictingQueue<Map<Counters.Key, Long>> window;

        public GraphOneCounter(EvictingQueue<Map<Counters.Key, Long>> window) {
            this.window = window;
        }

        @Override
        public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
            Counters.Key key = Counters.Key.valueOf(routeParameters.get("key"));

            XYSeriesCollection dataset = new XYSeriesCollection();
            XYSeries series = new XYSeries(key.name() + "# per second");
            int i = 0;

            for (Map<Counters.Key, Long> map : new ArrayList<>(window)) {
                long value = map.getOrDefault(key, 0l);
                series.add(i++, value);
            }

            dataset.addSeries(series);
            JFreeChart chart = ChartFactory.createXYLineChart(key.name() + "#/second",
                    "Elapsed time (seconds)", key.name() + "#/second", dataset, PlotOrientation.VERTICAL, true, false, false);

            XYPlot plot = (XYPlot) chart.getPlot();
            plot.setBackgroundPaint(Color.white);
            plot.setRangeGridlinePaint(Color.lightGray);
            plot.setDomainGridlinePaint(Color.lightGray);
            plot.getRenderer().setSeriesStroke(0, new BasicStroke(2));
            return new Resource() {
                @Override
                public String getContentType() {
                    return "image/png";
                }

                @Override
                public void render(OutputStream outputStream) throws IOException {
                    ChartUtilities.writeChartAsPNG(outputStream, chart, 800, 300);
                }
            };
        }
    }

    class ListVolume implements RequestHandler {
        private AsyncAm asyncAm;

        public ListVolume(AsyncAm asyncAm) {
            this.asyncAm = asyncAm;
        }

        @Override
        public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
            String volumeName = routeParameters.get("volume");
            List<BlobDescriptor> bds = asyncAm.volumeContents("", volumeName, 100, 0, "", PatternSemantics.PCRE, BlobListOrder.UNSPECIFIED, false).get();
            JSONArray array = new JSONArray();
            for (BlobDescriptor bd : bds) {
                JSONObject o = new JSONObject();
                o.put("name", bd.getName());
                o.put("size", bd.getByteCount());
                JSONObject metadata = new JSONObject();
                bd.getMetadata().keySet().forEach(k -> metadata.put(k, bd.getMetadata().get(k)));
                o.put("metadata", metadata);
                array.put(o);
            }
            return new JsonResource(array);
        }
    }
}
