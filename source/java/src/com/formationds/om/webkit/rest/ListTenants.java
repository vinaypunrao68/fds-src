package com.formationds.om.webkit.rest;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.Tenant;
import com.formationds.util.thrift.ConfigurationApi;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import org.json.JSONObject;

import javax.crypto.SecretKey;
import java.util.Map;

public class ListTenants implements RequestHandler {
    private ConfigurationApi config;
    private SecretKey        secretKey;

    public ListTenants(ConfigurationApi configCache, SecretKey secretKey) {
        this.secretKey = secretKey;
        this.config = configCache;
    }

    // wrap a checked exception within a lambda to unwrap outside of the lambda block
    private class WrappedException extends RuntimeException {
        private Exception exception;
        public WrappedException(Exception e) { exception = e; }
        public Exception unwrap() { return exception; }
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {
        JSONArray array = new JSONArray();
        try {
            config.listTenants(0).stream()
                  .map(t -> {
                      JSONObject o = new JSONObject();
                      o.put("id", Long.toString(t.getId()));
                      o.put("name", t.getIdentifier());
                      try {
                          o.put("users", users(t));
                      } catch (Exception e) {
                          throw new WrappedException(e);
                      }
                      return o;
                  })
                  .forEach(array::put);

            return new JsonResource(array);
        } catch (WrappedException re) {
            throw re.unwrap();
        }
    }

    private JSONArray users(Tenant t) throws Exception {
        JSONArray array = new JSONArray();
        config.listUsersForTenant(t.getId()).stream()
                .map(u -> new JSONObject()
                        .put("id", Long.toString(u.getId()))
                        .put("identifier", u.getIdentifier()))
                .forEach(array::put);
        return array;
    }
}
