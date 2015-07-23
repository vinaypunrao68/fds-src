package com.formationds.util.s3.auth;

import com.formationds.spike.later.HttpContext;
import org.apache.commons.codec.digest.DigestUtils;
import org.apache.commons.httpclient.util.DateParseException;
import org.apache.commons.httpclient.util.DateUtil;
import org.bouncycastle.util.encoders.Hex;
import org.joda.time.DateTime;
import org.joda.time.DateTimeZone;
import org.joda.time.format.ISODateTimeFormat;

import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.stream.Collectors;

public class SignatureRequestData {
    public static final String AMZ_DATE_HEADER_KEY = "x-amz-date";
    public static final String HTTP_DATE_HEADER_KEY = "Date";
    private static String method;
    private static String path;
    private Map<String, Collection<String>> queryParameters;
    private Map<String, List<String>> signedHeaders;
    private DateTime date;

    public SignatureRequestData(String method, String path, Map<String, Collection<String>> queryParameters, Map<String, Collection<String>> headers, List<String> headersToSign) {
        this.method = method;
        this.path = uriEncode(normalize(path), false);
        this.queryParameters = queryParameters;
        signedHeaders = getSignedHeaderMap(headers, headersToSign);

        String amzDateHdr = headers.containsKey(AMZ_DATE_HEADER_KEY) ? getAny(headers.get(AMZ_DATE_HEADER_KEY)) : null;
        String httpDateHdr = headers.containsKey(HTTP_DATE_HEADER_KEY) ? getAny(headers.get(HTTP_DATE_HEADER_KEY)) : null;

        date = getDateForRequest(amzDateHdr, httpDateHdr);
        if (this.date == null)
            throw new IllegalArgumentException("no valid date header was found or is invalid (expecting Date or x-amz-date header)");
    }

    public SignatureRequestData(HttpContext context) {
        method = context.getRequestMethod().toUpperCase();
        path = normalize(context.getRequestURI()).replaceAll("%2(f|F)", "/");  // unencode slash
        queryParameters = context.getQueryParameters();
        V4AuthHeader authHeader = new V4AuthHeader(context.getRequestHeader("Authorization"));
        List<String> signedHeaderList = Arrays.asList(authHeader.getSignedHeaders());
        signedHeaders = getSignedHeaderMap(context.getRequestHeaderNames().stream().collect(Collectors.toMap(k -> k, k -> context.getRequestHeaderValues(k))), signedHeaderList);


        date = getDateForRequest(context.getRequestHeader(AMZ_DATE_HEADER_KEY), context.getRequestHeader(HTTP_DATE_HEADER_KEY));
        if(date == null)
            throw new IllegalArgumentException("No valid date header (e.g. Date, x-amz-date) headers found in request");
    }

    private DateTime getDateForRequest(String xamzDateHeaderValue, String httpDateHeaderValue) {
        DateTime date = null;
        if (xamzDateHeaderValue != null) {
            date = DateTime.parse(xamzDateHeaderValue, ISODateTimeFormat.basicDateTimeNoMillis()).withZone(DateTimeZone.UTC);
        }

        if (this.date == null && httpDateHeaderValue != null) {
            // date = DateTime.parse(httpDateHeaderValue, DateTimeFormats.RFC1123());
            try {
                date = new DateTime(DateUtil.parseDate(httpDateHeaderValue));
            } catch (DateParseException e) {

            }
        }
        return date;
    }

    private String  normalize(String path) {
        String[] pathElements = path.split("/", -1);
        Stack<String> pathElementsOut = new Stack<>();
        for(int i = 0; i < pathElements.length; i++) {
            String element = pathElements[i];
            if(element.isEmpty() && i != 0 && i != pathElements.length - 1)
                continue;

            if(element.equals("."))
                continue;

            if(element.equals("..")) {
                pathElementsOut.pop();
            } else {
                pathElementsOut.add(element);
            }
        }

        if(pathElementsOut.size() == 1)
            pathElementsOut.push("");

        return String.join("/", pathElementsOut);
    }

    private <T> T getAny(Collection<T> collection) {
        for(T t : collection)
            return t;
        return null;
    }

    public byte[] fullCanonicalRequestHash(byte[] payloadSha256) {
        String canonicalRequest = buildCanonicalRequest(payloadSha256);
        return DigestUtils.sha256(canonicalRequest);
    }

    public byte[] chunkedCanonicalRequestHash() {
        return DigestUtils.sha256(buildChunkedCanonicalRequest());
    }

    public String buildCanonicalRequest(byte[] sha) {
        return buildCanonicalRequest(Hex.toHexString(sha));
    }

    public String buildCanonicalRequest(String payload) {
        StringBuilder builder = new StringBuilder();
        builder.append(method.toUpperCase());
        builder.append("\n");

        builder.append(path);
        builder.append("\n");

        appendCanonicalQueryString(builder, queryParameters);
        builder.append("\n");

        String[] orderedHeaders = signedHeaders.keySet().toArray(new String[signedHeaders.size()]);
        Arrays.sort(orderedHeaders);
        for(String header : orderedHeaders) {
            builder.append(header);
            builder.append(":");
            builder.append(String.join(",", signedHeaders.get(header)));
            builder.append("\n");
        }
        builder.append("\n");

        builder.append(String.join(";", orderedHeaders));
        builder.append("\n");

        builder.append(payload);

        return builder.toString();
    }

    public String buildChunkedCanonicalRequest() {
        return buildCanonicalRequest("STREAMING-AWS4-HMAC-SHA256-PAYLOAD");
    }

    private Map<String, List<String>> getSignedHeaderMap(Map<String, Collection<String>> headers, List<String> headersToSign) {
        Map<String,List<String>> hmap =  new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
        for(String header : headersToSign)
            hmap.computeIfAbsent(header.toLowerCase(), hdr -> new ArrayList<>());

        for(Map.Entry<String, Collection<String>> headerEntry : headers.entrySet()) {
            if(hmap.containsKey(headerEntry.getKey())) {
                for(String value : headerEntry.getValue()) {
                    hmap.get(headerEntry.getKey()).add(value.trim());
                }
            }
        }

        for(String header : headersToSign) {
            if (hmap.get(header).size() == 0)
                throw new IllegalArgumentException("Missing header required for signing '" + header + "'");
            hmap.get(header).sort(Comparator.<String>naturalOrder());
        }
        return hmap;
    }

    private void appendCanonicalQueryString(StringBuilder builder, Map<String, Collection<String>> queryParameters) {
        List<String> queryStringParts = new ArrayList<>();

        for(Map.Entry<String, Collection<String>> queryParam : queryParameters.entrySet()) {
            for(String val : queryParam.getValue()) {
                queryStringParts.add(uriEncode(queryParam.getKey()) + "=" + uriEncode(val));
            }
        }

        queryStringParts.sort(Comparator.<String>naturalOrder());
        builder.append(String.join("&", queryStringParts));
    }

    private static String uriEncode(CharSequence input) {
        return uriEncode(input, true);
    }

    // taken from: http://docs.aws.amazon.com/AmazonS3/latest/API/sig-auth-header-based-auth.html
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
                    result.append(String.format("%%%02X", b));
                }
            }
        }
        return result.toString();
    }

    public DateTime getDate() {
        return date;
    }
}
