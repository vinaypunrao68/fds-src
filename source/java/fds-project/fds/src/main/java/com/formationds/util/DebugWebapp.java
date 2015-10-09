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
import org.jfree.ui.RectangleInsets;
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

                    sb.append("<h1>Incoming NFS requests</h1>");
                    sb.append(makeImgLink(Counters.Key.inodeCreate));
                    sb.append(makeImgLink(Counters.Key.inodeAccess));
                    sb.append(makeImgLink(Counters.Key.lookupFile));
                    sb.append(makeImgLink(Counters.Key.link));
                    sb.append(makeImgLink(Counters.Key.listDirectory));
                    sb.append(makeImgLink(Counters.Key.mkdir));
                    sb.append(makeImgLink(Counters.Key.move));
                    sb.append(makeImgLink(Counters.Key.read));
                    sb.append(makeImgLink(Counters.Key.bytesRead));
                    sb.append(makeImgLink(Counters.Key.readLink));
                    sb.append(makeImgLink(Counters.Key.remove));
                    sb.append(makeImgLink(Counters.Key.symlink));
                    sb.append(makeImgLink(Counters.Key.write));
                    sb.append(makeImgLink(Counters.Key.bytesWritten));
                    sb.append(makeImgLink(Counters.Key.getAttr));
                    sb.append(makeImgLink(Counters.Key.setAttr));

                    sb.append("<h1>Cache activity</h1>");
                    sb.append(makeImgLink(Counters.Key.metadataCacheMiss));
                    sb.append(makeImgLink(Counters.Key.metadataCacheHit));
                    sb.append(makeImgLink(Counters.Key.objectCacheMiss));
                    sb.append(makeImgLink(Counters.Key.objectCacheHit));
                    sb.append(makeImgLink(Counters.Key.deferredMetadataMutation));

                    sb.append("<h1>AM activity</h1>");
                    sb.append(makeImgLink(Counters.Key.AM_statBlob));
                    sb.append(makeImgLink(Counters.Key.AM_updateMetadataTx));
                    sb.append(makeImgLink(Counters.Key.AM_getBlobWithMeta));
                    sb.append(makeImgLink(Counters.Key.AM_updateBlobOnce_objectAndMetadata));
                    sb.append(makeImgLink(Counters.Key.AM_updateBlob));

                    sb.append("</body></html>");
                    OutputStreamWriter osw = new OutputStreamWriter(outputStream);
                    osw.write(sb.toString());
                    osw.flush();
                }
            };
        }

        private String makeImgLink(Counters.Key key) {
            return "<img src=\"/stats/" + key.name() + "\" /><br /><br />\n";
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

            chart.getLegend().setVisible(false);
            chart.setBorderVisible(false);
            
            XYPlot plot = (XYPlot) chart.getPlot();
            plot.setOutlinePaint(Color.white);
            plot.getRangeAxis().setAxisLineVisible(false);
            plot.getRangeAxis().setLabelPaint(Color.white);
            plot.getDomainAxis().setAxisLineVisible(false);
            plot.setInsets(new RectangleInsets(0, 0, 0, 0));
            plot.setBackgroundPaint(Color.white);
            plot.setRangeGridlinePaint(Color.gray);
            plot.setDomainGridlinePaint(Color.gray);
            plot.setBackgroundPaint(Color.lightGray);
            plot.getRenderer().setSeriesStroke(0, new BasicStroke(2));
            return new Resource() {
                @Override
                public String getContentType() {
                    return "image/png";
                }

                @Override
                public void render(OutputStream outputStream) throws IOException {
                    ChartUtilities.writeChartAsPNG(outputStream, chart, 600, 200);
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
            List<BlobDescriptor> bds = asyncAm.volumeContents("",
                                                              volumeName,
                                                              100,
                                                              0,
                                                              "",
                                                              PatternSemantics.PCRE,
                                                              "",
                                                              BlobListOrder.UNSPECIFIED,
                                                              false).get().getBlobs();
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
