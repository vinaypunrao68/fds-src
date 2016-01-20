/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.events;

import com.formationds.apis.Tenant;
import com.formationds.apis.User;
import com.formationds.commons.model.entity.Event;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.repository.EventRepository;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.security.AuthenticatedRequestContext;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.Resource;
import com.formationds.util.thrift.ConfigurationApi;
import com.google.gson.reflect.TypeToken;

import org.apache.thrift.TException;
import org.eclipse.jetty.server.Request;
import org.json.JSONArray;
import java.io.InputStreamReader;
import java.io.Reader;
import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Optional;

/**
 * @author dsetzke based on QueryMetrics by ptinius
 */
public class QueryEvents implements RequestHandler {

    private static final Type TYPE = new TypeToken<QueryCriteria>() { }.getType();

    public QueryEvents() {
        super();
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters) throws Exception {

        try (final Reader reader = new InputStreamReader(request.getInputStream(), "UTF-8")) {
            final QueryCriteria eventQuery = ObjectModelHelper.toObject(reader, TYPE);

            // Filter the events to remove any that the current user should not be able to access.
            // If user is a global admin, can access all events;
            // if user is a tenant admin, can access only events associated with the specific tenant(s) the
            // the user has access to.
            long uid = AuthenticatedRequestContext.getToken().getUserId();
            User requestUser = findUser(uid);

            EventRepository er = SingletonRepositoryManager.instance().getEventRepository();
            List<? extends Event> events = null;
            if (requestUser == null || requestUser.isIsFdsAdmin()) {
                events = er.query(eventQuery);
            } else {
                final List<Long> tenantUserIds = findTenantUserIds(uid);
                events = er.queryTenantUsers(eventQuery, tenantUserIds);
            }
            return new JsonResource( new JSONArray( events ) );
        }
    }

    private List<Long> findTenantUserIds(long currentUser) throws TException {
        ConfigurationApi cfg = SingletonConfigAPI.instance().api();
        Optional<Tenant> ot = cfg.tenantFor(currentUser);
        List<Long> tenantUserIds = new ArrayList<>();
        if (ot.isPresent()) {
            Tenant t = ot.get();
            List<User> tenantUsers = cfg.listUsersForTenant(t.getId());
            tenantUsers.forEach((n) -> tenantUserIds.add(n.getId()));
        }
        return tenantUserIds;
    }

    private User findUser(long uid) throws TException {
        ConfigurationApi cfg = SingletonConfigAPI.instance().api();
        for (User u : cfg.allUsers(0)) {
            if (u.getId() == uid)
                return u;
        }
        return null;
    }

}
