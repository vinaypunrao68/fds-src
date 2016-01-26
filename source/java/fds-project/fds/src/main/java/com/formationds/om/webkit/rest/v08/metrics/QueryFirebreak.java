/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.webkit.rest.v08.metrics;

import java.io.InputStreamReader;
import java.io.Reader;
import java.lang.reflect.Type;
import java.util.Map;

import javax.servlet.http.HttpServletRequest;

import org.eclipse.jetty.server.Request;
import org.json.JSONObject;

import com.formationds.commons.model.Statistics;
import com.formationds.commons.model.helper.ObjectModelHelper;
import com.formationds.om.repository.helper.FirebreakHelper;
import com.formationds.om.repository.query.FirebreakQueryCriteria;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.RequestHandler;
import com.formationds.web.toolkit.RequestLog;
import com.formationds.web.toolkit.Resource;
import com.google.gson.reflect.TypeToken;

public class QueryFirebreak implements RequestHandler {

    private static final Type TYPE = new TypeToken<FirebreakQueryCriteria>() { }.getType();
    private Authorizer authorizer;
    private AuthenticationToken token;


    public QueryFirebreak( final Authorizer authorizer, AuthenticationToken token ) {
        super();

        this.token = token;
        this.authorizer = authorizer;
    }

    @Override
    public Resource handle(Request request, Map<String, String> routeParameters)
            throws Exception {

        try( final Reader reader =
                new InputStreamReader( request.getInputStream(), "UTF-8" ) ) {

            final Statistics stats = new FirebreakHelper().execute( ObjectModelHelper.toObject( reader, TYPE ),
                                                                    authorizer, token );

            return new JsonResource( new JSONObject( stats ) );
        }
    }

}
