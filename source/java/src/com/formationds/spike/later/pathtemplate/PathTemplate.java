package com.formationds.spike.later.pathtemplate;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import java.util.*;

public class PathTemplate {
    private ArrayList<PathTemplateElement> pathTemplateElements;

    public PathTemplate(List<PathTemplateElement> elements) {
        pathTemplateElements = new ArrayList<>(elements);
    }

    public static PathTemplate parseSimple(String pathSpecifier) {
        String[] segments = pathSpecifier.split("/");
        List<PathTemplateElement> elts = new ArrayList<>();

        for(String segment : segments) {
            if(segment.isEmpty()) {
                elts.add(LiteralPathTemplateElement.empty);
            } else if(segment.startsWith(":")) {
                if(segment.length() == 1)
                    throw new IllegalArgumentException("colon (:) path element must have a name");
                elts.add(new CaptureSegmentTemplateElement(segment.substring(1), new AnyPathTemplateElement()));
            } else {
                elts.add(new LiteralPathTemplateElement(segment, true));
            }
        }
        return new PathTemplate(elts);
    }

    public TemplateMatch match(String path) {
        Map<String, String> parameters = new HashMap<>();
        String[] segments = path.split("/");
        if(segments.length != pathTemplateElements.size())
            return new TemplateMatch(false, null);

        for (int i = 0; i < segments.length; i++) {
            if(!pathTemplateElements.get(i).matchPathSegment(segments[i]))
                return new TemplateMatch(false, null);

            Optional<KeyValuePair<String, String>> kvp = pathTemplateElements.get(i).captureParameter(segments[i]);
            if(kvp.isPresent())
                parameters.put(kvp.get().getKey(), kvp.get().getValue());
        }

        return new TemplateMatch(true, parameters);
    }

    public List<PathTemplateElement> getPathTemplateElements() {
        return pathTemplateElements;
    }

    public class TemplateMatch {
        private boolean matched;
        private Map<String, String> parameters;

        public TemplateMatch(boolean matched, Map<String, String> parameters) {
            this.matched = matched;
            this.parameters = parameters;
        }

        public Map<String, String> getParameters() {
            return parameters;
        }

        public boolean isMatch() {
            return matched;
        }
    }
}
