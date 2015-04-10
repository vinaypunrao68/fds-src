package com.formationds.xdi.swift;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeStatus;
import com.formationds.security.AuthenticationToken;
import com.formationds.util.JsonArrayCollector;
import com.formationds.web.Dom4jResource;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.TextResource;
import com.formationds.xdi.Xdi;
import com.google.common.base.Joiner;
import org.dom4j.Document;
import org.dom4j.DocumentHelper;
import org.dom4j.Element;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class ListContainers implements SwiftRequestHandler {
    private Xdi xdi;
    private AuthenticationToken token;

    public ListContainers(Xdi xdi, AuthenticationToken token) {
        this.xdi = xdi;
        this.token = token;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        String accountName = requiredString(routeParameters, "account");

        ResponseFormat format = obtainFormat(request);
        // TODO: get xdi call for a subset of this info
        // TODO: implement limit, marker, end_marker, format, prefix, delimiter query string variables
        List<VolumeDescriptor> volumes = xdi.listVolumes(token, accountName);

        volumes = new SkipUntil<VolumeDescriptor>(request.getParameter("marker"), v -> v.getName())
                .apply(volumes);

        Resource result;
        switch (format) {
            case xml:
                result = xmlView(volumes, accountName);
                break;

            case json:
                result = jsonView(volumes, accountName);
                break;

            default:
                result = plainView(volumes);
                break;
        }

        // TODO: Implement X-Account-Object-Count, X-Account-Bytes-Used, X-Account-Meta-*
        //
        return SwiftUtility.swiftResource(result);
    }

    private Resource plainView(List<VolumeDescriptor> volumes) {
        List<String> volumeNames = volumes.stream().map(v -> v.getName()).collect(Collectors.toList());
        String joined = Joiner.on("\n").join(volumeNames);
        return new TextResource(joined);
    }

    private Resource jsonView(List<VolumeDescriptor> volumes, String accountName) {
        JSONArray array = volumes.stream()
                .map(v -> {
                    VolumeStatus status = null;
                    try {
                        status = xdi.statVolume(token, accountName, v.getName()).get();
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                    return new JSONObject()
                            .put("name", v.getName())
                            .put("count", status.getBlobCount())
                            .put("bytes", status.getCurrentUsageInBytes());
                })
                .collect(new JsonArrayCollector());
        return new JsonResource(array);
    }

    private Resource xmlView(List<VolumeDescriptor> volumes, String accountName) {
        Document document = DocumentHelper.createDocument();
        Element root = document.addElement("account").addAttribute("name", accountName);

        volumes.stream()
                .forEach(v -> {
                    VolumeStatus status = null;
                    try {
                        status = xdi.statVolume(token, accountName, v.getName()).get();
                    } catch (Exception e) {
                        throw new RuntimeException(e);
                    }
                    Element object = root.addElement("container");
                    object.addElement("name").addText(v.getName());
                    object.addElement("count").addText(Long.toString(status.getBlobCount()));
                    object.addElement("bytes").addText(Long.toString(status.getCurrentUsageInBytes()));
                });

        return new Dom4jResource(document);
    }
}
