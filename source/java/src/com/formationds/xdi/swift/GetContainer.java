package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.ApiException;
import com.formationds.apis.BlobDescriptor;
import com.formationds.apis.ErrorCode;
import com.formationds.protocol.BlobListOrder;
import com.formationds.security.AuthenticationToken;
import com.formationds.util.JsonArrayCollector;
import com.formationds.web.Dom4jResource;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import com.google.common.base.Joiner;
import org.apache.thrift.TException;
import org.dom4j.Document;
import org.dom4j.DocumentHelper;
import org.dom4j.Element;
import org.eclipse.jetty.server.Request;
import org.joda.time.DateTime;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class GetContainer  implements SwiftRequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public GetContainer(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }


    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String accountName = requiredString(routeParameters, "account");
        String containerName = requiredString(routeParameters, "container");

        int limit = optionalInt(request, "limit", Integer.MAX_VALUE);
        ResponseFormat format = obtainFormat(request);
        List<BlobDescriptor> descriptors = null;
        try {
            descriptors = xdi.volumeContents(token, accountName, containerName, limit, 0, "", BlobListOrder.UNSPECIFIED, false);
        } catch (TException e) {
            throw new ApiException("Not found", ErrorCode.MISSING_RESOURCE);
        }

        descriptors = new SkipUntil<BlobDescriptor>(request.getParameter("marker"), b -> b.getName())
                .apply(descriptors);

        Resource result;
        switch (format) {
            case xml:
                result = xmlView(containerName, descriptors);
                break;

            case json:
                result = jsonView(descriptors);
                break;

            default:
                result = plainView(descriptors);
                break;
        }

        return SwiftUtility.swiftResource(result);
    }

    private Resource plainView(List<BlobDescriptor> descriptors) {
        String textView = Joiner.on("\n").join(descriptors.stream().map(d -> d.getName()).collect(Collectors.toList()));
        return new TextResource(textView);
    }

    private Resource jsonView(List<BlobDescriptor> descriptors) {
        JSONArray array = descriptors.stream()
                .map(d -> new JSONObject()
                        .put("hash", digest(d))
                        .put("last_modified", lastModified(d))
                        .put("bytes", d.getByteCount())
                        .put("name", d.getName())
                        .put("content_type", contentType(d)))
                .collect(new JsonArrayCollector());
        return new JsonResource(array);
    }

    private String contentType(BlobDescriptor d) {
        return d.getMetadata().getOrDefault("Content-Type", "application/octet-stream");
    }

    private String digest(BlobDescriptor d) {
        return d.getMetadata().getOrDefault("etag", "");
    }

    private String lastModified(BlobDescriptor d) {
        String timestamp = d.getMetadata().get(Xdi.LAST_MODIFIED);
        try {
            return new DateTime(Long.parseLong(timestamp)).toString();
        } catch (Exception e) {
            return DateTime.now().toString();
        }
    }

    private Resource xmlView(String containerName, List<BlobDescriptor> descriptors) throws Exception {
        Document document = DocumentHelper.createDocument();
        Element root = document.addElement("container").addAttribute("name", containerName);

        descriptors.stream()
                .forEach(d -> {
                    Element object = root.addElement("object");
                    object.addElement("name").addText(d.getName());
                    object.addElement("hash").addText(digest(d));
                    object.addElement("bytes").addText(Long.toString(d.getByteCount()));
                    object.addElement("content_type").addText(contentType(d));
                    object.addElement("last_modified").addText(lastModified(d));
                });

        return new Dom4jResource(document);
    }
}
