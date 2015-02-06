package com.formationds.spike.later;

import com.formationds.web.toolkit.ClosingInterceptor;
import com.formationds.web.toolkit.Resource;
import com.google.common.collect.Multimap;
import io.undertow.server.handlers.CookieImpl;

import javax.servlet.http.Cookie;
import java.util.Arrays;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;

public class AsyncBridge implements Function<HttpContext, CompletableFuture<Void>> {
    private SyncRequestHandler rh;

    public AsyncBridge(SyncRequestHandler rh) {
        this.rh = rh;
    }

    @Override
    public CompletableFuture<Void> apply(HttpContext httpContext) {
        CompletableFuture<Void> cf = new CompletableFuture<>();

        try {
            //HttpServerExchange exchange = httpContext.getExchange();
            Resource resource = rh.handle(httpContext);
            Arrays.stream(resource.cookies()).forEach(c -> httpContext.addCookie(c.getName(), toUndertowCookie(c)));

            httpContext.addResponseHeader("Access-Control-Allow-Origin", "*");
            httpContext.addResponseHeader("Server", "Formation");
            httpContext.setResponseContentType(resource.getContentType());
            httpContext.setResponseStatus(resource.getHttpStatus());

            Multimap<String, String> extraHeaders = resource.extraHeaders();
            for (String headerName : extraHeaders.keySet()) {
                for (String value : extraHeaders.get(headerName)) {
                    httpContext.addResponseHeader(headerName, value);
                }
            }

            ClosingInterceptor outputStream = new ClosingInterceptor(httpContext.getOutputStream());
            resource.render(outputStream);
            outputStream.flush();
            outputStream.doCloseForReal();
            cf.complete(null);
        } catch (Exception e) {
            cf.completeExceptionally(e);
        }
        return cf;
    }

    private io.undertow.server.handlers.Cookie toUndertowCookie(Cookie c) {
        return new CookieImpl(c.getName(), c.getValue()).setDomain(c.getDomain()).setMaxAge(c.getMaxAge());
    }
}
