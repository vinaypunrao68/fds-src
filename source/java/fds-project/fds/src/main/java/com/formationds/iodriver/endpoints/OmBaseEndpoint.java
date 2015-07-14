package com.formationds.iodriver.endpoints;

import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;

import javax.net.ssl.HttpsURLConnection;

import org.json.JSONException;
import org.json.JSONObject;

import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.Uris;
import com.formationds.commons.util.logging.Logger;

public abstract class OmBaseEndpoint<ThisT extends OmBaseEndpoint<ThisT>>
        extends AbstractHttpsEndpoint<ThisT>
{
    /**
     * An OM authentication token. This should be considered an opaque string.
     */
    public static final class AuthToken
    {
        @Override
        public String toString()
        {
            return _value;
        }

        /**
         * Constructor.
         * 
         * @param value The token value itself.
         */
        protected AuthToken(String value)
        {
            if (value == null) throw new NullArgumentException("value");

            _value = value;
        }

        /**
         * The token value.
         */
        private final String _value;
    }

    /**
     * Constructor.
     * 
     * @param uri Base URI of all requests sent through this endpoint.
     * @param username The username to use to acquire the auth token.
     * @param password The password to use to acquire the auth token.
     * @param logger Log messages are sent here.
     * @param trusting Whether to trust all SSL certs. Should not be {@code true} in production.
     * 
     * @throws MalformedURLException when {@code uri} is not a valid absolute URL.
     */
    public OmBaseEndpoint(URI uri,
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
    
    /**
     * Log into the OM and get the authentication token to be used in future requests.
     * 
     * @return An authentication token.
     * 
     * @throws IOException when an error occurs while logging in.
     */
    public final AuthToken getAuthToken() throws IOException
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
            HttpsURLConnection authConnection = openConnectionWithoutAuth(authUrl);

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

    public abstract ThisT copy();
    
    /**
     * Get the username to use to log into the OM.
     * 
     * @return The current property value.
     */
    public final String getUsername()
    {
        return _username;
    }

    /**
     * Extend this class to allow deep copies even when the superclass private members aren't
     * available.
     */
    protected final class CopyHelper extends AbstractHttpsEndpoint<ThisT>.CopyHelper
    {
        public final String password = _password;
        public final String username = _username;
    }

    /**
     * Copy constructor.
     * 
     * @param helper Object holding copied values to assign to the new object.
     */
    protected OmBaseEndpoint(CopyHelper helper)
    {
        super(helper);

        _password = helper.password;
        _username = helper.username;
    }

    @Override
    protected final HttpsURLConnection openConnection(URL url) throws IOException
    {
        if (url == null) throw new NullArgumentException("url");

        getAuthToken();

        return openConnectionWithoutAuth(url);
    }

    /**
     * Open a connection without authenticating first. Primarily used to authenticate.
     * 
     * @param url The URL to connect to.
     * 
     * @return A connection.
     * 
     * @throws IOException when an error occurs while connecting.
     */
    protected final HttpsURLConnection openConnectionWithoutAuth(URL url) throws IOException
    {
        if (url == null) throw new NullArgumentException("url");

        HttpsURLConnection connection = super.openConnection(url);
        connection.setRequestProperty(Fds.FDS_AUTH_HEADER, _authToken == null ? null
                : _authToken.toString());

        return connection;
    }

    /**
     * The token used to authenticate requests. {@code null} prior to login.
     */
    private AuthToken _authToken;
    
    /**
     * The password to use when authenticating.
     */
    private final String _password;

    /**
     * The username to use when authenticating.
     */
    private final String _username;
}
