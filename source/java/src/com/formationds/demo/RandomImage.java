package com.formationds.demo;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;

import java.util.Map;

public class RandomImage implements RequestHandler {
    private DemoState state;

    public RandomImage(DemoState state) {
        this.state = state;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
//        return state.getUrls()
//                .map(a -> {
//                    List<String> urls = Lists.newArrayList(a);
//                    Collections.shuffle(urls);
//                    return (Resource) new JsonResource(new JSONObject().put("url", urls.get(0)));
//                }).orElse(new FourOhFour());
        return null;
    }
}
