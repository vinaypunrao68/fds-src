package com.formationds.web.toolkit;

import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.google.common.collect.Multimap;

import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Response;
import org.json.JSONObject;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import java.io.IOException;
import java.util.*;
import java.util.concurrent.CompletableFuture;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
public class Dispatcher extends HttpServlet {
    private static final Logger LOG = LogManager.getLogger(Dispatcher.class);

    private RouteFinder routeFinder;
    private String webDir;
    private List<AsyncRequestExecutor> asyncRequestExecutorList;

    public Dispatcher(RouteFinder routeFinder, String webDir, List<AsyncRequestExecutor> executors) {
        this.routeFinder = routeFinder;
        this.webDir = webDir;
        this.asyncRequestExecutorList = executors;
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

        boolean featureLoggingRequestWrapperEnabled = FdsFeatureToggles.WEB_LOGGING_REQUEST_WRAPPER.isActive();
        Request request = ( featureLoggingRequestWrapperEnabled ?
                              RequestLog.newJettyRequestLogger( (Request) httpServletRequest ) :
                              (Request) httpServletRequest );

        @SuppressWarnings("unchecked")
		String requestLogEntryBase = (featureLoggingRequestWrapperEnabled ?
		                                 ((RequestLog.LoggingRequestWrapper<Request>)request).getRequestInfoHeader() :
		                                 String.format( "%s %s", request.getMethod(), request.getRequestURI() ) );
        LOG.info("{} Starting", requestLogEntryBase);
        Resource resource = new FourOhFour();
        try {
	        RequestHandler requestHandler;
	        Map<String, String> routeAttributes = new HashMap<>();

	        if (isStaticAsset(request) && webDir != null) {
	            requestHandler = new StaticFileHandler(webDir);
	        } else {

	            Optional<CompletableFuture<Void>> execution = tryExecuteAsyncApp(request, (Response) response);
	            if(execution.isPresent())
	                return;

	            Optional<Route> route = routeFinder.resolve(request);
	            if (!route.isPresent()) {
	                route = Optional.of(new Route(request, new HashMap<>(),
	                                              FourOhFour::new ));
	            }
	            requestHandler = route.get().getHandler().get();
	            request = route.get().getRequest();
	            routeAttributes = route.get().getAttributes();
	        }

	        try {
	            resource = requestHandler.handle(request, routeAttributes);
	        } catch (UsageException e) {
	            resource = new JsonResource(new JSONObject().put("message", e.getMessage()), HttpServletResponse.SC_BAD_REQUEST);
	        } catch (Throwable t) {
	            LOG.fatal(t.getMessage(), t);
	            resource = new ErrorPage(t.getMessage(), t);
	        }

	        Arrays.stream(resource.cookies()).forEach( response::addCookie );
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

        } finally {
	        long elapsed = System.currentTimeMillis() - then;
	        LOG.info("{} Complete HTTP status: {}, {} ms", requestLogEntryBase, resource.getHttpStatus(), elapsed);
        	if ( featureLoggingRequestWrapperEnabled ) {
        	    RequestLog.requestComplete(request);
        	}
        }
    }

    private Optional<CompletableFuture<Void>> tryExecuteAsyncApp(Request request, Response response) {
        if(asyncRequestExecutorList != null) {
            for (AsyncRequestExecutor executor : asyncRequestExecutorList) {
                Optional<CompletableFuture<Void>> cf = executor.tryExecuteRequest(request, response);
                if (cf.isPresent())
                    return cf;
            }
        }

        return Optional.empty();
    }

    private static boolean isStaticAsset(HttpServletRequest request) {
        String path = request.getRequestURI()
                .replaceAll("^" + request.getServletPath() + "/", "")
                .replaceAll("^/", "");

        return path.matches(".*\\..+");
    }
}
