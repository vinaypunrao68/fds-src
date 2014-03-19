package com.formationds.web.toolkit;

import com.google.common.collect.Multimap;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.handler.AbstractHandler;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.util.Arrays;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class Dispatcher extends AbstractHandler {
    private static final Logger LOG = Logger.getLogger(Dispatcher.class);

    private RouteFinder routeFinder;
    private String webDir;

    public Dispatcher(RouteFinder routeFinder, String webDir) {
        this.routeFinder = routeFinder;
        this.webDir = webDir;
    }

    @Override
    public void handle(String target, Request request, HttpServletRequest httpServletRequest, HttpServletResponse response) throws IOException, ServletException {
        RequestHandler requestHandler;

        if (isStaticAsset(request)) {
            requestHandler = new StaticFileHandler(webDir);
        } else {
            Route route = routeFinder.resolve(request);
            requestHandler = route.getHandler().get();
            request = route.getRequest();
        }

        Resource resource = new FourOhFour();
        try {
            resource = requestHandler.handle(request);
        } catch (Throwable t) {
            LOG.error(t.getMessage(), t);
            resource = new ErrorPage(t.getMessage(), t);
        } finally {
            LOG.info("Request URI: [" + request.getRequestURI() + "], HTTP status: " + resource.getHttpStatus());
        }

        Arrays.stream(resource.cookies()).forEach(c -> response.addCookie(c));
        response.addHeader("Access-Control-Allow-Origin", "*");
        response.setContentType(resource.getContentType());
        response.setStatus(resource.getHttpStatus());
        Multimap<String, String> extraHeaders = resource.extraHeaders();
        for (String headerName : extraHeaders.keySet()) {
            for (String value : extraHeaders.get(headerName)) {
                response.addHeader(headerName, value);
            }
        }

        ClosingInterceptor outputStream = new ClosingInterceptor(response.getOutputStream());
        resource.render(outputStream);
        outputStream.flush();
        outputStream.doCloseForReal();
    }

    private boolean isStaticAsset(HttpServletRequest request) {
        String path = request.getRequestURI()
                .replaceAll("^" + request.getServletPath() + "/", "")
                .replaceAll("^/", "");

        return path.matches(".*\\..+");
    }
}
