/**
 *
 */
package com.formationds.om.webkit;

import java.io.IOException;
import java.util.Arrays;
import java.util.EnumSet;
import java.util.List;

import javax.servlet.DispatcherType;
import javax.servlet.Filter;
import javax.servlet.FilterChain;
import javax.servlet.FilterConfig;
import javax.servlet.ServletException;
import javax.servlet.ServletRequest;
import javax.servlet.ServletResponse;
import javax.servlet.annotation.WebFilter;
import javax.servlet.http.HttpServletResponse;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.eclipse.jetty.server.HttpConnection;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Response;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.json.JSONObject;

import com.formationds.apis.ConfigurationService;
import com.formationds.commons.togglz.feature.flag.FdsFeatureToggles;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.web.toolkit.Dispatcher;
import com.formationds.web.toolkit.JsonResource;
import com.formationds.web.toolkit.Resource;
import com.formationds.web.toolkit.WebApp;

/**
 */
public class OmWebApp extends WebApp {

    @WebFilter("/fds/config/v08/*")
    public static class DomainStatusFilter implements Filter {

        private static final Logger logger = LogManager.getLogger( DomainStatusFilter.class );

        public static final List<String> allowedPaths = Arrays.asList( "/fds/config/v08/token",
                                                                       "/fds/config/v08/local_domain",
                                                                       "/fds/config/v08/nodes" );
        private FilterConfig filterConfig;
        private ConfigurationService.Iface configSvc;

        @Override
        public void init( FilterConfig cfg ) throws ServletException {
            this.filterConfig = cfg;
            configSvc = SingletonConfigAPI.instance().api();
        }

        @Override
        public void destroy() {
            filterConfig = null;
        }

        public boolean isAllowed(String requestUri) {
            for (String p : allowedPaths) {
                if ( requestUri.contains( p ) )
                    return true;
            }
            return false;
        }

        @Override
        public void doFilter( ServletRequest request, ServletResponse response, FilterChain chain ) throws IOException,
                                                                                                    ServletException {

            if ( ! FdsFeatureToggles.ENABLE_DOMAIN_WEB_FILTER.isActive() ) {
                chain.doFilter( request, response );
                return;
            }

            Request base_request = request instanceof Request ? (Request) request
                                                              : HttpConnection.getCurrentConnection().getHttpChannel()
                                                                              .getRequest();
            Response base_response = response instanceof Response ? (Response) response
                                                                  : HttpConnection.getCurrentConnection()
                                                                                  .getHttpChannel().getResponse();

            // allow local_domain requests to go through
            String requestURI = base_request.getRequestURI();
            if ( isAllowed( requestURI ) ) {
                logger.trace( "DOMAIN_CHECK::ALLOWED::{}", requestURI );

                chain.doFilter( request, response );
                return;
            }

            // for all other requests, determine if the local domain is
            // currently up before allowing the request through
            boolean shortCircuitRequest = false;
            try {
                shortCircuitRequest = !configSvc.isLocalDomainUp();
            } catch ( Exception e ) {
                // most certainly a Thrift connection exception of some kind.
                logger.trace( "DOMAIN_CHECK::FAILED::{}", e.getMessage() );
                throw new ServletException("Failed to access configuration service", e);
            }

            if ( shortCircuitRequest ) {
                logger.trace( "DOMAIN_CHECK::DOWN::Filter request: {}", requestURI );
                Resource r = new JsonResource( new JSONObject().put( "message", "Local domain is down" ),
                                               HttpServletResponse.SC_INTERNAL_SERVER_ERROR );
                Dispatcher.sendResponse( base_response, r );
            } else {
                logger.trace( "DOMAIN_CHECK::UP::Allowing request: {}", requestURI );
                chain.doFilter( request, response );
            }
        }
    }

    /**
     * @param webDir
     */
    public OmWebApp( String webDir ) {
        super( webDir );
    }

    /**
     *
     */
    public OmWebApp() {
        this( null );
    }

    @Override
    protected ServletContextHandler configureDispatcherContextHandler() {
        ServletContextHandler sch = super.configureDispatcherContextHandler();
        sch.addFilter( DomainStatusFilter.class, "/fds/config/v08/*", EnumSet.of( DispatcherType.REQUEST ) );
        return sch;
    }
}
