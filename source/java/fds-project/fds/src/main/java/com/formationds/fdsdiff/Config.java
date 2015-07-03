package com.formationds.fdsdiff;

import static com.formationds.commons.util.Strings.javaString;

import java.util.Optional;

import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

import com.formationds.commons.AbstractConfig;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.ConsoleLogger;
import com.formationds.commons.util.logging.Logger;

/**
 * Global configuration for {@link com.formationds.fdsdiff}.
 */
public final class Config extends AbstractConfig
{
	public final static class Defaults
	{
		public static final ComparisonDataFormat COMPARISON_DATA_FORMAT;
		
		public static final Logger LOGGER;
		
		static
		{
			COMPARISON_DATA_FORMAT = ComparisonDataFormat.FULL;
			LOGGER = new ConsoleLogger();
		}
	}
	
	public Config(String[] args)
	{
		super(args);
		
		_comparisonDataFormat = Optional.empty();
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
	
	public Optional<String> getInputFilename() throws ParseException
	{
		if (_inputFilename == null)
		{
			_inputFilename = getCommandLineOptionValue("input");
		}
		
		return _inputFilename;
	}
	
	public Optional<String> getOutputFilename() throws ParseException
	{
		if (_outputFilename == null)
		{
			_outputFilename = getCommandLineOptionValue("output");
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
	
	private Optional<String> _inputFilename;
	
	private Optional<String> _outputFilename;
}
