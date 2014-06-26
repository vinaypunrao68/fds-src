package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.AmazonWebServiceRequest;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.http.HttpMethodName;
import com.amazonaws.util.AWSRequestMetrics;
import com.formationds.security.Authenticator;
import com.formationds.security.AuthorizationToken;
import com.formationds.web.toolkit.RequestHandler;
import com.google.common.collect.Maps;
import com.sun.security.auth.UserPrincipal;
import org.eclipse.jetty.server.Request;

import java.io.InputStream;
import java.net.URI;
import java.net.URISyntaxException;
import java.text.MessageFormat;
import java.text.ParseException;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;
import java.util.function.Supplier;

public class S3Authorizer implements Supplier<RequestHandler> {
    private Supplier<RequestHandler> supplier;

    public S3Authorizer(Supplier<RequestHandler> supplier) {
        this.supplier = supplier;
    }

    @Override
    public RequestHandler get() {
        return (request, routeParameters) -> {
            String candidateHeader = request.getHeader("Authorization");
            BasicAWSCredentials candidateCredentials = null;
            try {
                candidateCredentials = tryParse(candidateHeader);
            } catch (SecurityException e) {
                return new S3Failure(S3Failure.ErrorCode.AccessDenied, "Access denied", request.getRequestURI());
            }

            String requestHash = hashRequest(request, candidateCredentials);

            if (candidateHeader.equals(requestHash)) {
                return supplier.get().handle(request, routeParameters);
            } else {
                return new S3Failure(S3Failure.ErrorCode.AccessDenied, "Access denied", request.getRequestURI());
            }
        };
    }

    private String hashRequest(Request request, BasicAWSCredentials candidateCredentials) {
        CustomS3Signer signer = new CustomS3Signer(request.getMethod(), request.getRequestURI());
        com.amazonaws.Request amazonRequest = makeAmazonRequest(request);
        signer.sign(amazonRequest, candidateCredentials);
        return amazonRequest.getHeaders().get("Authorization").toString();
    }

    private com.amazonaws.Request makeAmazonRequest(Request request) {
        Map<String, String> headers = Maps.newHashMap();
        Enumeration en = request.getHeaderNames();
        while (en.hasMoreElements()) {
            String headerName = (String) en.nextElement();
            String value = request.getHeader(headerName);
            if (!headerName.equals("Authorization")) {
                headers.put(headerName, value);
            }
        }

        return new com.amazonaws.Request() {

            @Override
            public void addHeader(String key, String value) {
                headers.put(key, value);
            }

            @Override
            public Map<String, String> getHeaders() {
                return headers;
            }

            @Override
            public void setHeaders(Map map) {
                headers.putAll(map);
            }

            @Override
            public void setResourcePath(String s) {

            }

            @Override
            public String getResourcePath() {
                return request.getRequestURI();
            }

            @Override
            public void addParameter(String s, String s2) {

            }

            @Override
            public com.amazonaws.Request withParameter(String s, String s2) {
                return null;
            }

            @Override
            public Map<String, String> getParameters() {
                HashMap<String, String> result = new HashMap<>();
                Enumeration en = request.getParameterNames();

                while (en.hasMoreElements()) {
                    String key = (String) en.nextElement();
                    String value = request.getParameter(key);
                    result.put(key, value);
                }
                return result;
            }

            @Override
            public void setParameters(Map map) {

            }

            @Override
            public URI getEndpoint() {
                try {
                    return new URI("http://localhost/");
                } catch (URISyntaxException e) {
                    throw new RuntimeException(e);
                }
            }

            @Override
            public void setEndpoint(URI uri) {

            }

            @Override
            public HttpMethodName getHttpMethod() {
                return HttpMethodName.valueOf(request.getMethod().toUpperCase());
            }

            @Override
            public void setHttpMethod(HttpMethodName httpMethodName) {

            }

            @Override
            public InputStream getContent() {
                return null;
            }

            @Override
            public void setContent(InputStream inputStream) {

            }

            @Override
            public String getServiceName() {
                return null;
            }

            @Override
            public AmazonWebServiceRequest getOriginalRequest() {
                return null;
            }

            @Override
            public int getTimeOffset() {
                return 0;
            }

            @Override
            public void setTimeOffset(int i) {

            }

            @Override
            public com.amazonaws.Request withTimeOffset(int i) {
                return null;
            }

            @Override
            public AWSRequestMetrics getAWSRequestMetrics() {
                return null;
            }

            @Override
            public void setAWSRequestMetrics(AWSRequestMetrics awsRequestMetrics) {

            }
        };
    }

    private BasicAWSCredentials tryParse(String header) {
        String pattern = "AWS {0}:{1}";
        Object[] parsed = new Object[0];
        try {
            parsed = new MessageFormat(pattern).parse(header);
        } catch (ParseException e) {
            throw new SecurityException("invalid credentials");
        }
        String principal = (String) parsed[0];
        AuthorizationToken token = new AuthorizationToken(Authenticator.KEY, new UserPrincipal(principal));

        return new BasicAWSCredentials(principal, token.getKey().toBase64());
    }
}
