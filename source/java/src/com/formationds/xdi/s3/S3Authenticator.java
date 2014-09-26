package com.formationds.xdi.s3;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.amazonaws.AmazonWebServiceRequest;
import com.amazonaws.auth.BasicAWSCredentials;
import com.amazonaws.http.HttpMethodName;
import com.amazonaws.services.s3.internal.S3Signer;
import com.amazonaws.services.s3.internal.ServiceUtils;
import com.amazonaws.util.AWSRequestMetrics;
import com.formationds.security.AuthenticationToken;
import com.formationds.xdi.Xdi;
import com.google.common.collect.Maps;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.util.MultiMap;

import javax.crypto.SecretKey;
import java.io.InputStream;
import java.net.URI;
import java.net.URISyntaxException;
import java.text.MessageFormat;
import java.text.ParseException;
import java.util.Date;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Map;

public class S3Authenticator {
    private Xdi xdi;
    private SecretKey secretKey;

    public S3Authenticator(Xdi xdi, SecretKey secretKey) {
        this.xdi = xdi;
        this.secretKey = secretKey;
    }

    public AuthenticationToken authenticate(Request request) throws SecurityException {
        if (xdi.getAuthenticator().allowAll()) {
            return AuthenticationToken.ANONYMOUS;
        }

        String candidateHeader = request.getHeader("Authorization");
        AuthenticationComponents authenticationComponents = resolveFdsCredentials(candidateHeader);

        String requestHash = hashRequest(request, authenticationComponents);

        if (candidateHeader.equals(requestHash)) {
            return authenticationComponents.fdsToken;
        } else {
            throw new SecurityException();
        }
    }

    private String hashRequest(Request request, AuthenticationComponents authComponents) {
        com.amazonaws.Request amazonRequest = makeAmazonRequest(request);
        S3Signer s3Signer = new S3Signer(amazonRequest.getHttpMethod().name(), amazonRequest.getResourcePath()) {
            @Override
            protected Date getSignatureDate(int timeOffset) {
                try {
                    return ServiceUtils.parseRfc822Date(request.getHeader("Date"));
                } catch (ParseException e) {
                    throw new RuntimeException(e);
                }
            }
        };
        BasicAWSCredentials credentials = new BasicAWSCredentials(authComponents.principalName, authComponents.fdsToken.signature(secretKey));
        s3Signer.sign(amazonRequest, credentials);
        return amazonRequest.getHeaders().get("Authorization").toString();
    }

    private com.amazonaws.Request makeAmazonRequest(Request request) {
        Map<String, String> headers = Maps.newHashMap();
        Enumeration en = request.getHeaderNames();
        while (en.hasMoreElements()) {
            String headerName = (String) en.nextElement();
            String value = request.getHeader(headerName);
            if (!headerName.equals("Authorization")) {
                headers.put(headerName, value.toLowerCase());
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
                if(request.getQueryParameters() == null)
                    return result;

                MultiMap<String> qp = request.getQueryParameters();
                // NB: the S3 signer wants fields without parameters (e.g. ?uploads) to return null instead
                // of the empty string.
                // assuming the empty string is null is a good heuristic, but it could fail if somehow
                // someone encodes the empty string in another way besides a field-without-a-parameter
                for(String key : qp.keySet()) {
                    String value = qp.getString(key);
                    if(value.equals(""))
                        value = null;
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

    private AuthenticationComponents resolveFdsCredentials(String header) {
        String pattern = "AWS {0}:{1}";
        Object[] parsed = new Object[0];
        try {
            parsed = new MessageFormat(pattern).parse(header);
            String principal = (String) parsed[0];
            AuthenticationToken fdsToken = xdi.getAuthenticator().currentToken(principal);
            return new AuthenticationComponents(principal, fdsToken);
        } catch (Exception e) {
            throw new SecurityException("invalid credentials");
        }
    }


    private class AuthenticationComponents {
        String principalName;
        AuthenticationToken fdsToken;

        AuthenticationComponents(String principalName, AuthenticationToken fdsToken) {
            this.principalName = principalName;
            this.fdsToken = fdsToken;
        }
    }
}

