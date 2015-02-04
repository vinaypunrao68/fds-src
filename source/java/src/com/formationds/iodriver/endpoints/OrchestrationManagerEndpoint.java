package com.formationds.iodriver.endpoints;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.net.URLConnection;

import org.json.JSONException;
import org.json.JSONObject;

import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.Uris;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.operations.OrchestrationManagerOperation;
import com.formationds.iodriver.reporters.WorkflowEventListener;

// @eclipseFormat:off
public final class OrchestrationManagerEndpoint
    extends AbstractHttpsEndpoint<OrchestrationManagerEndpoint, OrchestrationManagerOperation>
// @eclipseFormat:on
{
    public static class AuthToken
    {
        @Override
        public final String toString()
        {
            return _value;
        }

        protected AuthToken(String value)
        {
            if (value == null) throw new NullArgumentException("value");

            _value = value;
        }

        private final String _value;
    }

    public OrchestrationManagerEndpoint(URI uri,
                                        String username,
                                        String password,
                                        Logger logger,
                                        boolean trusting) throws MalformedURLException
    {
        super(uri, logger, trusting);

        if (username == null) throw new NullArgumentException("username");
        if (password == null) throw new NullArgumentException("password");

        _username = username;
        _password = password;
    }

    @Override
    public OrchestrationManagerEndpoint copy()
    {
        CopyHelper copyHelper = new CopyHelper();
        return new OrchestrationManagerEndpoint(copyHelper);
    }

    public AuthToken getAuthToken() throws IOException
    {
        if (_authToken == null)
        {
            URI authUri;
            try
            {
                authUri = Uris.withQueryParameters(Fds.Api.getAuthToken(),
                                                   Fds.USERNAME_QUERY_PARAMETER,
                                                   _username,
                                                   Fds.PASSWORD_QUERY_PARAMETER,
                                                   _password);
            }
            catch (URISyntaxException e)
            {
                throw new RuntimeException("Unexpected error creating auth URI.", e);
            }
            URL authUrl = toUrl(authUri);
            HttpURLConnection authConnection =
                    (HttpURLConnection)openConnectionWithoutAuth(authUrl);

            JSONObject userObject;
            try
            {
                userObject = new JSONObject(getResponse(authConnection));
            }
            catch (JSONException | HttpException e)
            {
                throw new IOException("Error authenticating.", e);
            }
            _authToken = new AuthToken(userObject.getString("token"));
        }

        return _authToken;
    }

    public String getUsername()
    {
        return _username;
    }

    protected class CopyHelper
                              extends AbstractHttpsEndpoint<OrchestrationManagerEndpoint,
                              OrchestrationManagerOperation>.CopyHelper
    {
        public final String password = _password;
        public final String username = _username;
    }

    protected OrchestrationManagerEndpoint(CopyHelper helper)
    {
        super(helper);

        _password = helper.password;
        _username = helper.username;
    }

    @Override
    protected URLConnection openConnection(URL url) throws IOException
    {
        if (url == null) throw new NullArgumentException("url");

        getAuthToken();

        return openConnectionWithoutAuth(url);
    }

    protected URLConnection openConnectionWithoutAuth(URL url) throws IOException
    {
        if (url == null) throw new NullArgumentException("url");

        URLConnection connection = super.openConnection(url);
        connection.setRequestProperty(Fds.FDS_AUTH_HEADER, _authToken == null ? null
                : _authToken.toString());

        return connection;
    }

    private AuthToken _authToken;

    private final String _password;

    private final String _username;
}
