package com.formationds.web.toolkit;

import com.google.common.collect.Multimap;
import org.apache.log4j.Logger;
import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class Dispatcher extends HttpServlet {
    private static final Logger LOG = Logger.getLogger(Dispatcher.class);

    private RouteFinder routeFinder;
    private String webDir;

    public Dispatcher(RouteFinder routeFinder, String webDir) {
        this.routeFinder = routeFinder;
        this.webDir = webDir;
    }


    @Override
    protected void doGet(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        service(req, resp);
    }

    @Override
    protected void doPost(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        service(req, resp);
    }

    @Override
    protected void doPut(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        service(req, resp);
    }

    @Override
    protected void doDelete(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        service(req, resp);
    }

    @Override
    public void service(HttpServletRequest httpServletRequest, HttpServletResponse response) throws IOException, ServletException {
        long then = System.currentTimeMillis();
        Request request = (Request) httpServletRequest;
        RequestHandler requestHandler;
        Map<String, String> routeAttributes = new HashMap<>();

        if (isStaticAsset(request) && webDir != null) {
            requestHandler = new StaticFileHandler(webDir);
        } else {
            Optional<Route> route = routeFinder.resolve(request);
            if (!route.isPresent()) {
                route = Optional.of(new Route(request, new HashMap<>(), () -> new FourOhFour()));
            }
            requestHandler = route.get().getHandler().get();
            request = route.get().getRequest();
            routeAttributes = route.get().getAttributes();
        }

        Resource resource = new FourOhFour();
        try {
            resource = requestHandler.handle(request, routeAttributes);
        } catch (UsageException e) {
            resource = new JsonResource(new JSONObject().put("message", e.getMessage()), HttpServletResponse.SC_BAD_REQUEST);
        } catch (Throwable t) {
            LOG.error(t.getMessage(), t);
            resource = new ErrorPage(t.getMessage(), t);
        }

        Arrays.stream(resource.cookies()).forEach(c -> response.addCookie(c));
        response.addHeader("Access-Control-Allow-Origin", "*");
        response.setContentType(resource.getContentType());
        response.setStatus(resource.getHttpStatus());
        response.setHeader("Server", "Formation Data Systems");
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

        long elapsed = System.currentTimeMillis() - then;
        LOG.info("Request URI: [" + request.getMethod() + " " + request.getRequestURI() + "], HTTP status: " + resource.getHttpStatus() + ", " + elapsed + "ms");
    }

    private boolean isStaticAsset(HttpServletRequest request) {
        String path = request.getRequestURI()
                .replaceAll("^" + request.getServletPath() + "/", "")
                .replaceAll("^/", "");

        return path.matches(".*\\..+");
    }
}
