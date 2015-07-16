package com.formationds.fdsdiff;

import static com.formationds.commons.util.Strings.javaString;

import java.net.MalformedURLException;
import java.net.URI;
import java.util.Arrays;
import java.util.Optional;
import java.util.stream.Collectors;

import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

import com.formationds.commons.AbstractConfig;
import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.ConfigurationException;
import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.endpoints.OmV7Endpoint;
import com.formationds.iodriver.endpoints.OmV8Endpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.model.ComparisonDataFormat;

/**
 * Global configuration for {@link com.formationds.fdsdiff}.
 */
public final class Config extends AbstractConfig
{
	public final static class Defaults extends AbstractConfig.Defaults
	{
		public static final ComparisonDataFormat COMPARISON_DATA_FORMAT;
		
		static
		{
			COMPARISON_DATA_FORMAT = ComparisonDataFormat.EXTENDED;
		}
	}
	
	public Config(String[] args)
	{
		super(args);
		
		_comparisonDataFormat = Optional.empty();
		_endpointA = null;
		_endpointAHost = null;
		_endpointB = null;
		_endpointBHost = null;
		_inputFilename = null;
		_outputFilename = null;
	}
	
	public ComparisonDataFormat getComparisonDataFormat() throws ParseException
	{
		if (!_comparisonDataFormat.isPresent())
		{
			Optional<String> comparisonDataFormatString = getCommandLineOptionValue("format");
			if (comparisonDataFormatString.isPresent())
			{
				_comparisonDataFormat =
						Optional.ofNullable(Enum.valueOf(ComparisonDataFormat.class,
						                                 comparisonDataFormatString.get()));

				if (!_comparisonDataFormat.isPresent())
				{
					throw new ParseException("Unrecognized data format: "
				                             + javaString(comparisonDataFormatString.get()) + ".");
				}
			}
			else
			{
				_comparisonDataFormat = Optional.of(Defaults.COMPARISON_DATA_FORMAT);
			}
		}
		
		return _comparisonDataFormat.get();
	}
	
	public FdsEndpoint getEndpointA() throws ConfigurationException, ParseException
	{
		if (_endpointA == null)
		{
			String endpointAHost = getEndpointAHost();

			_endpointA = getFdsEndpoint(endpointAHost);
		}
		return _endpointA;
	}
	
	public Optional<FdsEndpoint> getEndpointB() throws ConfigurationException, ParseException
	{
	    if (_endpointB == null)
	    {
	        Optional<String> endpointBHost = getEndpointBHost();
	        if (endpointBHost.isPresent())
	        {
	            _endpointB = Optional.of(getFdsEndpoint(endpointBHost.get()));
	        }
	        else
	        {
	            _endpointB = Optional.empty();
	        }
	    }
	    return _endpointB;
	}
	
	public String getEndpointAHost() throws ParseException
	{
		if (_endpointAHost == null)
		{
			Optional<String> newEndpointAHost = getCommandLineOptionValue("endpoint-a");
			if (newEndpointAHost.isPresent())
			{
				_endpointAHost = newEndpointAHost.get();
			}
			else
			{
				_endpointAHost = "localhost";
			}
		}
		return _endpointAHost;
	}
	
	public Optional<String> getEndpointBHost() throws ParseException
	{
	    if (_endpointBHost == null)
	    {
	        _endpointBHost = getCommandLineOptionValue("endpoint-b");
	    }
	    return _endpointBHost;
	}
	
	public String getInputFilename() throws ParseException
	{
		if (_inputFilename == null)
		{
			_inputFilename = getCommandLineOptionValue("input").orElse("-");
		}
		
		return _inputFilename;
	}
	
	public String getOutputFilename() throws ParseException
	{
		if (_outputFilename == null)
		{
			_outputFilename = getCommandLineOptionValue("output").orElse("-");
		}
		
		return _outputFilename;
	}
	
	@Override
	protected void addOptions(Options options)
	{
		if (options == null) throw new NullArgumentException("options");
		
		options.addOption("A", "endpoint-a", true, "The first host to compare.");
		options.addOption("B", "endpoint-b", true, "The second host to compare.");
        options.addOption("f",
                          "format",
                          true,
                          "The data format used for comparing systems. Available formats are: "
                          + Arrays.asList(ComparisonDataFormat.values())
                                  .stream()
                                  .<String>map(v -> v.toString())
                                  .collect(Collectors.joining(", ")));
        options.addOption("i", "input", true, "Read comparison data from the specified file.");
		options.addOption("o", "output", true, "Output comparison data to the specified file.");
	}
	
	@Override
	protected String getProgramName()
	{
		return "fdsdiff";
	}
	
	private Optional<ComparisonDataFormat> _comparisonDataFormat;
	
	private FdsEndpoint _endpointA;
	
	private String _endpointAHost;
	
	private Optional<FdsEndpoint> _endpointB;

	private Optional<String> _endpointBHost;
	
	private String _inputFilename;
	
	private String _outputFilename;
    
    private static FdsEndpoint getFdsEndpoint(String host) throws ConfigurationException
    {
        if (host == null) throw new NullArgumentException("host");
        
        @SuppressWarnings("deprecation")
        URI omV7Url = Fds.Api.V07.getBase(host);
        URI omV8Url = Fds.Api.V08.getBase(host);
        URI s3Url = Fds.getS3Endpoint(host);
        
        // FIXME: Allow UN/PW to be specified.
        OmV7Endpoint omV7;
        try
        {
            omV7 = new OmV7Endpoint(omV7Url, "admin", "admin", Defaults.getLogger(), true);
        }
        catch (MalformedURLException e)
        {
            throw new ConfigurationException("Error setting OM v7 URL: " + omV7Url, e);
        }
        OmV8Endpoint omV8;
        try
        {
            omV8 = new OmV8Endpoint(omV8Url, "admin", "admin", Defaults.getLogger(), true);
        }
        catch (MalformedURLException e)
        {
            throw new ConfigurationException("Error setting OM v8 URL: " + omV8Url, e);
        }
        S3Endpoint s3;
        try
        {
            s3 = new S3Endpoint(s3Url.toString(), omV8, Defaults.getLogger());
        }
        catch (MalformedURLException e)
        {
            throw new ConfigurationException("Error setting S3 URL: " + s3Url, e);
        }
        
        return new FdsEndpoint(omV7, omV8, s3);
    }
}
