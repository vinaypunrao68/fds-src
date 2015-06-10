package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.StreamingRegistrationMsg;
import com.formationds.streaming.Streaming;
import com.formationds.streaming.volumeDataPoints;
import org.apache.commons.lang3.StringUtils;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.methods.HttpPut;
import org.apache.http.client.methods.HttpRequestBase;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClients;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;
import org.json.JSONArray;

import java.util.List;

public class StatisticsPublisher implements Streaming.Iface {
    private static final Logger LOG = Logger.getLogger(StatisticsPublisher.class);
    private ConfigurationService.Iface configClient;

    public StatisticsPublisher(ConfigurationService.Iface configClient) {
        this.configClient = configClient;
    }

    @Override
    public void publishMetaStream(int registrationId, List<volumeDataPoints> dataPoints) throws TException {
        List<StreamingRegistrationMsg> regs = configClient.getStreamRegistrations(0);
        for (StreamingRegistrationMsg reg : regs) {
            if (reg.getId() == registrationId) {
                try {
                    publish(reg, dataPoints);
                } catch (Exception e) {
                    LOG.error("Error publishing datapoints to " + reg.getUrl(), e);
                    throw new RuntimeException(e);
                }
            }
        }
    }

    private void publish(StreamingRegistrationMsg reg, List<volumeDataPoints> dataPoints) throws Exception {
        LOG.info(String.format("Publishing %d datapoints to %s", dataPoints.size(), reg.getUrl()));

        JSONArray jsonArray = new JsonStatisticsFormatter().format(dataPoints);
        String url = reg.getUrl();
        String httpMethod = reg.getHttp_method();
        CloseableHttpClient client = HttpClients.createDefault();
        HttpRequestBase request = buildRequest(httpMethod, url, jsonArray);
        client.execute(request);
    }

    private HttpRequestBase buildRequest(String httpMethod, String url, JSONArray jsonArray) throws Exception {
        StringEntity entity = new StringEntity(jsonArray.toString(4));
        if (StringUtils.isBlank(httpMethod) || "post".equals(httpMethod.toLowerCase())) {
            HttpPost post = new HttpPost(url);
            post.setEntity(entity);
            return post;
        } else if ("put".equals(httpMethod.toLowerCase())) {
            HttpPut put = new HttpPut(url);
            put.setEntity(entity);
            return put;
        } else {
            throw new RuntimeException("Unsupported method [" + httpMethod + "]");
        }
    }
}


