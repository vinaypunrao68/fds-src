package com.formationds.web.toolkit;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.eclipse.jetty.server.Request;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.HashMap;
import java.util.Map;

public class StaticFileHandler implements RequestHandler {
    private String webDir;

    private static final Map<String, String> MIME_TYPES;

    static {
        String[][] EXTENSIONS = new String[][]{
                {".gif", "image/gif"},
                {".pdf", "application/pdf"},
                {".doc", "application/vnd.ms-word"},
                {".jpg", "image/jpg"},
                {".jpeg", "image/jpeg"},
                {".png", "image/png"},
                {".ico", "image/bmp"},
                {".css", "text/css"},
                {".js", "text/javascript"},
                {".coffee", "text/coffeescript"},
                {".svg", "text/svg"},
                {".txt", "text/plain"},
                {".html", "text/html"},
                {".gz", "application/gzip"},
                {".flv", "video/x-flv"}
        };

        MIME_TYPES = new HashMap<>();
        for (String[] mapping : EXTENSIONS) {
            MIME_TYPES.put(mapping[0], mapping[1]);
        }
    }



    public StaticFileHandler(String webDir) {
        this.webDir = webDir;
    }

    public static String getMimeType(String objectName) {
        int offset = objectName.lastIndexOf('.');
        String defaultValue = "application/octet-stream";
        if (offset < 0) {
            return defaultValue;
        }
        return MIME_TYPES.getOrDefault(objectName.substring(offset).toLowerCase(), defaultValue);
    }


    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws FileNotFoundException {
        String resource = request.getRequestURI();
        String mimeType = getMimeType(resource);

        File file = new File(webDir, resource);
        if (!file.exists()) {
            return new FourOhFour();
        }

        return new StreamResource(new FileInputStream(file), mimeType);
    }

    private String getExtension(String resource) {
        int offset = resource.lastIndexOf('.');
        return resource.substring(offset).toLowerCase();
    }
}