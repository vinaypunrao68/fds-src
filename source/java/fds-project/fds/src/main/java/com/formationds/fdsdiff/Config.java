package com.formationds.fdsdiff;

import static com.formationds.commons.util.ExceptionHelper.tunnel;
import static com.formationds.commons.util.Strings.javaString;

import java.net.MalformedURLException;
import java.net.URI;
import java.nio.file.FileSystem;
import java.nio.file.FileSystems;
import java.nio.file.InvalidPathException;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.Optional;
import java.util.stream.Collectors;

import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

import com.formationds.commons.AbstractConfig;
import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.CommandLineConfigurationException;
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
		
		_comparisonDataFormat = null;
		_defaultEndpoint = null;
		_endpointA = null;
		_endpointAHost = null;
		_endpointB = null;
		_endpointBHost = null;
		_inputAFilename = null;
		_inputAPath = null;
		_inputBFilename = null;
		_inputBPath = null;
		_outputAFilename = null;
		_outputAPath = null;
		_outputBFilename = null;
		_outputBPath = null;
	}
	
	public ComparisonDataFormat getComparisonDataFormat() throws ParseException
	{
	    return (_comparisonDataFormat == null
	            ? (_comparisonDataFormat = getCommandLineOptionValue("format").map(
	                    fs -> Enum.valueOf(ComparisonDataFormat.class, fs)))
	            : _comparisonDataFormat).orElse(Defaults.COMPARISON_DATA_FORMAT);
	}
	
	public FdsEndpoint getDefaultEndpoint() throws ConfigurationException
	{
	    return _defaultEndpoint == null
	           ? (_defaultEndpoint = getFdsEndpoint(Fds.getFdsHost()))
	           : _defaultEndpoint;
	}
	
	public Optional<FdsEndpoint> getEndpointA() throws ConfigurationException, ParseException
	{
	    return _endpointA == null
	           ? (_endpointA = tunnel(ConfigurationException.class,
	                                  in -> { return getEndpointAHost().map(in); },
	                                  (String h) -> getFdsEndpoint(h)))
	           : _endpointA;
	}
	
	public Optional<FdsEndpoint> getEndpointB() throws ConfigurationException, ParseException
	{
	    return _endpointB == null
	           ? (_endpointB = tunnel(ConfigurationException.class,
                                      in -> { return getEndpointBHost().map(in); },
                                      (String h) -> getFdsEndpoint(h)))
	           : _endpointB;
	}
	
	public Optional<String> getEndpointAHost() throws ParseException
	{
		return _endpointAHost == null
		       ? (_endpointAHost = getCommandLineOptionValue("endpoint-a"))
		       : _endpointAHost;
	}
	
	public Optional<String> getEndpointBHost() throws ParseException
	{
	    return _endpointBHost == null
	           ? (_endpointBHost = getCommandLineOptionValue("endpoint-b"))
	           : _endpointBHost;
	}
	
	public Optional<String> getInputAFilename() throws ParseException
	{
		return _inputAFilename == null
		       ? (_inputAFilename = getCommandLineOptionValue("input-a"))
		       : _inputAFilename;
	}
	
	public Optional<Path> getInputAPath() throws ParseException, CommandLineConfigurationException
	{
	    return _inputAPath == null
	           ? (_inputAPath = getCommandLinePath(getInputAFilename()))
	           : _inputAPath;
	}
	
	public Optional<String> getInputBFilename() throws ParseException
	{
	    return _inputBFilename == null
	           ? (_inputBFilename = getCommandLineOptionValue("input-b"))
	           : _inputBFilename;
	}
	
	public Optional<Path> getInputBPath() throws CommandLineConfigurationException, ParseException
	{
	    return _inputBPath == null
	           ? (_inputBPath = getCommandLinePath(getInputBFilename()))
	           : _inputBPath;
	}
	
	public Optional<String> getOutputAFilename() throws ParseException
	{
	    return _outputAFilename == null
	           ? (_outputAFilename = getCommandLineOptionValue("output-a"))
	           : _outputAFilename;
	}
	
	public Optional<Path> getOutputAPath() throws CommandLineConfigurationException, ParseException
	{
	    return _outputAPath == null
	           ? (_outputAPath = getCommandLinePath(getOutputAFilename()))
	           : _outputAPath;
	}
	
	public Optional<String> getOutputBFilename() throws ParseException
	{
	    return _outputBFilename == null
	           ? (_outputBFilename = getCommandLineOptionValue("output-b"))
	           : _outputBFilename;
	}
	
	public Optional<Path> getOutputBPath() throws CommandLineConfigurationException, ParseException
	{
	    return _outputBPath == null
	           ? (_outputBPath = getCommandLinePath(getOutputBFilename()))
	           : _outputBPath;
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
        options.addOption(null, "input-a", true, "Read comparison data from the specified file.");
        options.addOption(null, "input-b", true, "Read comparison data from the specified file.");
		options.addOption(null, "output-a", true, "Output comparison data to the specified file.");
		options.addOption(null, "output-b", true, "Output comparison data to the specified file.");
	}
	
	@Override
	protected String getProgramName()
	{
		return "fdsdiff";
	}
	
	private Optional<ComparisonDataFormat> _comparisonDataFormat;
    
    private FdsEndpoint _defaultEndpoint;
	
	private Optional<FdsEndpoint> _endpointA;
	
	private Optional<String> _endpointAHost;
	
	private Optional<FdsEndpoint> _endpointB;

	private Optional<String> _endpointBHost;
	
	private FileSystem _fs;
	
	private Optional<String> _inputAFilename;
	
	private Optional<Path> _inputAPath;
	
	private Optional<String> _inputBFilename;
	
	private Optional<Path> _inputBPath;
	
	private Optional<String> _outputAFilename;
	
	private Optional<Path> _outputAPath;
	
	private Optional<String> _outputBFilename;
	
	private Optional<Path> _outputBPath;
    
    private Optional<Path> getCommandLinePath(Optional<String> filename)
            throws CommandLineConfigurationException
    {
        if (filename == null) throw new NullArgumentException("filename");
        
        try
        {
            return filename.map(fn -> getFs().getPath(fn));
        }
        catch (InvalidPathException e)
        {
            throw new CommandLineConfigurationException(
                    "Path " + javaString(filename.get()) + " is not valid.", e);
        }
    }
    
	private FileSystem getFs()
	{
	    return _fs == null
	           ? (_fs = FileSystems.getDefault())
	           : _fs;
	}
	
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
