package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.commons.io.IOUtils;
import org.json.JSONObject;

import java.io.IOException;
import java.net.URL;
import java.net.URLEncoder;
import java.util.Iterator;

public class FlickrStream implements Iterator<FlickrStream.FlickrResource> {
    public static class FlickrResource {
        private String id;
        private String url;

        public FlickrResource(String id, String url) {
            this.id = id;
            this.url = url;
        }

        public String getId() {
            return id;
        }

        public String getUrl() {
            return url;
        }
    }

    private static String IMAGE_URL_TEMPLATE = "http://farm%d.staticflickr.com/%s/%s_%s_z.jpg";
    private static String SEARCH_URL_TEMPLATE =
            "https://api.flickr.com/services/rest/?method=flickr.photos.search&api_key=d2b1269787876896911080f38e494810&text=%s&per_page=%d&page=%d&format=json&nojsoncallback=1";

    private final String query;
    private int pageSize;

    private int currentOffset;
    private int currentPageNum;

    private boolean isDirty;
    private JSONObject currentPage;

    public FlickrStream(String q, int pageSize) {
        this.pageSize = pageSize;
        query = URLEncoder.encode(q);
        currentOffset = 0;
        isDirty = true;
        currentPage = null;
        currentPageNum = 0;
    }

    @Override
    public boolean hasNext() {
        refreshIfNeeded();
        int totalPages = currentPage.getJSONObject("photos").getInt("pages");
        return currentPageNum <= totalPages;
    }

    @Override
    public FlickrResource next() {
        refreshIfNeeded();
        JSONObject jsonObject = currentPage.getJSONObject("photos").getJSONArray("photo").getJSONObject(currentOffset);
        String id = jsonObject.getString("id");
        String server = jsonObject.getString("server");
        int farm = jsonObject.getInt("farm");
        String secret = jsonObject.getString("secret");
        String o = jsonObject.toString();
        String url = String.format(IMAGE_URL_TEMPLATE, farm, server, id, secret);
        currentOffset += 1;
        isDirty = true;


        return new FlickrResource(id, url);
    }


    private void refreshIfNeeded() {
        if (isDirty) {
            if (currentPage == null) {
                refreshPage();
            }

            if (currentOffset == currentPage.getJSONObject("photos").getJSONArray("photo").length()) {
                refreshPage();
                currentOffset = 0;
            }

            isDirty = false;
        }
    }

    private void refreshPage() {
        try {
            URL url = new URL(String.format(SEARCH_URL_TEMPLATE, URLEncoder.encode(query), pageSize, currentPageNum));
            String response = IOUtils.toString(url.openConnection().getInputStream());
            currentPage = new JSONObject(response);
            currentPageNum++;
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}
