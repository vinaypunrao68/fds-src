package com.formationds.spike.later;

import com.formationds.web.toolkit.ClosingInterceptor;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.google.common.collect.Multimap;
import org.eclipse.jetty.server.Response;

import java.util.Arrays;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class AsyncBridge implements Function<HttpPathContext, CompletableFuture<Void>> {
    private RequestHandler rh;

    public AsyncBridge(RequestHandler rh) {
        this.rh = rh;
    }

    @Override
    public CompletableFuture<Void> apply(HttpPathContext httpPathContext) {
        CompletableFuture<Void> cf = new CompletableFuture<>();

        try {
            Resource resource = rh.handle(httpPathContext.getRequest(), httpPathContext.getRouteParameters());
            Response response = httpPathContext.getResponse();
            Arrays.stream(resource.cookies()).forEach(response::addCookie);
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
            cf.complete(null);
        } catch (Exception e) {
            cf.completeExceptionally(e);
        }

        return cf;
    }
}
