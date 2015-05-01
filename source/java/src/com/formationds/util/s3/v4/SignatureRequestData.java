package com.formationds.util.s3.v4;

import com.formationds.spike.later.HttpContext;
import org.apache.commons.codec.digest.DigestUtils;
import org.bouncycastle.util.encoders.Hex;
import org.joda.time.DateTime;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.stream.Collectors;

public class SignatureRequestData {
    private static String method;
    private static String path;
    private Map<String, Collection<String>> queryParameters;
    private Map<String, List<String>> signedHeaders;
    private DateTime date;

    public SignatureRequestData(HttpContext context) {
        method = context.getRequestMethod().toUpperCase();
        path = context.getRequestURI();
        queryParameters = context.getQueryParameters();
        V4AuthHeader authHeader = new V4AuthHeader(context.getRequestHeader("Authorization"));
        List<String> signedHeaderList = Arrays.asList(authHeader.getSignedHeaders());
        signedHeaders = getSignedHeaderMap(context.getRequestHeaderNames().stream().collect(Collectors.toMap(k -> k, k -> context.getRequestHeaderValues(k))), signedHeaderList);

        String dateStr = context.getRequestHeader("x-amz-date");
        if(dateStr == null)
            dateStr = context.getRequestHeader("Date");

        if(dateStr == null) {
            throw new IllegalArgumentException("No Date of x-amz-date headers found in request");
        } else {
            try {
                date = DateTime.parse(dateStr);
            } catch(Exception ex) {
                throw new IllegalArgumentException("Request date format is unparsable", ex);
            }
        }
    }

    public byte[] fullCanonicalRequestHash(byte[] payloadSha256) {
        String canonicalRequest = buildFullCanonicalRequest(payloadSha256);
        return DigestUtils.sha256(canonicalRequest);
    }

    public byte[] chunkedCanonicalRequestHash() {
        return DigestUtils.sha256(chunkedCanonicalRequestHash());
    }

    public String buildFullCanonicalRequest(byte[] payloadSha256) {
        StringBuilder builder = new StringBuilder();
        builder.append(method.toUpperCase());
        builder.append("\n");

        builder.append(path);
        builder.append("\n");

        appendCanonicalQueryString(builder, queryParameters);
        builder.append("\n");

        String[] orderedHeaders = signedHeaders.entrySet().toArray(new String[signedHeaders.size()]);
        Arrays.sort(orderedHeaders);
        for(String header : orderedHeaders) {
            builder.append(header);
            builder.append(":");
            builder.append(signedHeaders.get(header).get(0).trim());
            builder.append("\n");
        }
        builder.append("\n");

        builder.append(String.join(";", orderedHeaders));
        builder.append("\n");

        builder.append(Hex.encode(payloadSha256));

        return builder.toString();
    }

    public String buildChunkedCanonicalRequest() {
        return buildFullCanonicalRequest("STREAMING-AWS4-HMAC-SHA256-PAYLOAD".getBytes());
    }

    private Map<String, List<String>> getSignedHeaderMap(Map<String, Collection<String>> headers, List<String> headersToSign) {
        Map<String,List<String>> hmap =  new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
        for(String header : headersToSign)
            headers.computeIfAbsent(header.toLowerCase(), hdr -> new ArrayList<>());

        for(Map.Entry<String, Collection<String>> headerEntry : headers.entrySet()) {
            if(headers.containsKey(headerEntry.getKey())) {
                hmap.get(headerEntry.getKey()).addAll(headerEntry.getValue());
            }
        }

        for(String header : headersToSign) {
            if (hmap.get(header).size() == 0)
                throw new IllegalArgumentException("Missing header required for signing '" + header + "'");
        }
        return hmap;
    }

    private void appendCanonicalQueryString(StringBuilder builder, Map<String, Collection<String>> queryParameters) {
        List<String> queryStringKeys = queryParameters.keySet().stream().sorted().collect(Collectors.toList());
        boolean isFirst = true;
        for(String key : queryStringKeys) {
            if(!isFirst) {
                builder.append("&");
                isFirst = false;
            }

            // S3's specification should guarantee that query string keys are unique, otherwise we might have to sort the values
            for(String value : queryParameters.get(key)) {
                builder.append(uriEncode(key));
                builder.append("=");
                builder.append(uriEncode(value));
            }
        }
    }

    private static String uriEncode(CharSequence input) {
        return uriEncode(input, true);
    }

    // taken from: http://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html
    private static String uriEncode(CharSequence input, boolean encodeSlash) {
        StringBuilder result = new StringBuilder();
        for (int i = 0; i < input.length(); i++) {
            char ch = input.charAt(i);
            if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '~' || ch == '.') {
                result.append(ch);
            } else if (ch == '/') {
                result.append(encodeSlash ? "%2F" : ch);
            } else {
                CharBuffer buf = CharBuffer.wrap(input, i, i + 1);
                ByteBuffer encoded = StandardCharsets.UTF_8.encode(buf);
                while(encoded.remaining() > 0) {
                    byte b = encoded.get();
                    result.append(String.format("%%%02X"));
                }
            }
        }
        return result.toString();
    }

    public DateTime getDate() {
        return date;
    }
}
