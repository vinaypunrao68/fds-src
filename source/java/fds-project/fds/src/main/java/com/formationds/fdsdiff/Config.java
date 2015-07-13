package com.formationds.fdsdiff;

import static com.formationds.commons.util.Strings.javaString;

import java.net.MalformedURLException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Optional;

import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

import com.formationds.commons.AbstractConfig;
import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.endpoints.OmEndpoint;

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
			COMPARISON_DATA_FORMAT = ComparisonDataFormat.FULL;
		}
	}
	
	public Config(String[] args)
	{
		super(args);
		
		_comparisonDataFormat = Optional.empty();
		_endpointA = null;
		_endpointAUrl = null;
		_endpointB = null;
		_endpointBUrl = null;
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
	
	public OmEndpoint getEndpointA() throws ParseException
	{
		if (_endpointA == null)
		{
			URI endpointAUrl = getEndpointAUrl();
			try {
				_endpointA = new OmEndpoint(endpointAUrl,
						                                      "admin",
						                                      "admin",
						                                      AbstractConfig.Defaults.getLogger(),
						                                      true,
						                                      null);
			} catch (MalformedURLException e) {
				throw new ParseException("Error reading endpoint URL: " + endpointAUrl);
			}
		}
		return _endpointA;
	}
	
	public URI getEndpointAUrl() throws ParseException
	{
		if (_endpointAUrl == null)
		{
			URI newEndpointAUrl = null;
			Optional<String> newEndpointAUrlString = getCommandLineOptionValue("endpoint-a");
			if (newEndpointAUrlString.isPresent())
			{
				String val = newEndpointAUrlString.get();
				try
				{
					newEndpointAUrl = new URI(val);
				}
				catch (URISyntaxException e)
				{
					throw new ParseException("Error parsing URL: " + javaString(val) + ".");
				}
			}
			else
			{
				newEndpointAUrl = Fds.Api.V08.getBase();
			}
			_endpointAUrl = newEndpointAUrl;
		}
		return _endpointAUrl;
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
		
		options.addOption(null, "format", true, "The data format used for comparing systems.");
		options.addOption("o", "output", true, "Output comparison data to the specified file.");
		options.addOption("i", "input", true, "Read comparison data from the specified file.");
	}
	
	@Override
	protected String getProgramName()
	{
		return "fdsdiff";
	}
	
	private Optional<ComparisonDataFormat> _comparisonDataFormat;
	
	private OmEndpoint _endpointA;
	
	private URI _endpointAUrl;
	
	private Optional<Endpoint> _endpointB;

	private Optional<URI> _endpointBUrl;
	
	private String _inputFilename;
	
	private String _outputFilename;
}
