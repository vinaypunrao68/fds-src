package com.formationds.xdi.s3;

import com.formationds.security.AuthenticationToken;
import com.formationds.spike.later.HttpContext;
import com.formationds.spike.later.SyncRequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.xdi.Xdi;

import javax.servlet.MultipartConfigElement;
import javax.servlet.http.Part;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class PostObjectUpload implements SyncRequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public PostObjectUpload(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(HttpContext context) throws Exception {
//        String bucketName = context.getRouteParameter("bucket");
//
//        if (context.getRequestContentType() != null && context.getRequestContentType().startsWith("multipart/form-data")) {
//            request.setAttribute(Request.__MULTIPART_CONFIG_ELEMENT, MULTI_PART_CONFIG);
//        }
//
//        Map<String, String> metadata = Maps.newHashMap();
//
//        Part filePart = request.getPart("file");
//        // String fileName = getFileName(filePart);
//        String key = IOUtils.toString(request.getPart("key").getInputStream());
//
//        Part contentTypePart = request.getPart("Content-Type");
//
//        if (contentTypePart != null) {
//            metadata.put("Content-Type", IOUtils.toString(contentTypePart.getInputStream()));
//        } else {
//            metadata.put("Content-Type", S3Endpoint.S3_DEFAULT_CONTENT_TYPE);
//        }
//
//        byte[] digest = xdi.writeStream(token, S3Endpoint.FDS_S3, bucketName, key, filePart.getInputStream(), metadata);
//
//        return new TextResource("") {
//            @Override
//            public Multimap<String, String> extraHeaders() {
//                LinkedListMultimap<String, String> map = LinkedListMultimap.create();
//                map.put("ETag", Hex.encodeHexString(digest));
//                return map;
//            }
//        };
        return null;
    }

    private String getFileName(Part part) {
        String contentDisp = part.getHeader("content-disposition");
        System.out.println("content-disposition header= "+contentDisp);
        String[] tokens = contentDisp.split(";");
        for (String token : tokens) {
            if (token.trim().startsWith("filename")) {
                return token.substring(token.indexOf("=") + 2, token.length()-1);
            }
        }
        return "";
    }

    private static final MultipartConfigElement MULTI_PART_CONFIG = new MultipartConfigElement(System.getProperty("java.io.tmpdir"));

}
