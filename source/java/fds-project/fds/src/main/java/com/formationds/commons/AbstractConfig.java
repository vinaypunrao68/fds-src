package com.formationds.commons;

import java.io.PrintStream;
import java.io.PrintWriter;
import java.util.Optional;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.cli.PosixParser;

/**
 * Base class for global program configuration.
 */
public abstract class AbstractConfig
{
    /**
     * The interactive console.
     */
    public final static class Console
    {
        /**
         * Get the best guess on the current console height.
         *
         * @return The current console height, or 25 if it cannot be determined.
         */
        public static int getHeight()
        {
            return getEnvInt("LINES", 25);
        }

        /**
         * Get the best guess on the current console width.
         *
         * @return The current console width, or 80 if it cannot be determined.
         */
        public static int getWidth()
        {
            return getEnvInt("COLUMNS", 80);
        }

        /**
         * Prevent instantiation.
         */
        private Console()
        {
        	throw new UnsupportedOperationException("Instantiating a utility class.");
        }
        
        /**
         * Get an integer value from the environment.
         * 
         * @param varName The name of the environment variable to retrieve.
         * @param defaultValue The value to return if the environment variable is not present.
         * 
         * @return {@code varName}'s value, or {@code defaultValue} if it is not present.
         */
        private static int getEnvInt(String varName, int defaultValue)
        {
            if (varName == null) throw new NullArgumentException("varName");

            String valueString = System.getenv(varName);
            if (valueString == null)
            {
                return defaultValue;
            }

            try
            {
                return Integer.parseInt(valueString);
            }
            catch (NumberFormatException e)
            {
            	// FIXME: Probably not a great idea to hide errors like this.
                return defaultValue;
            }
        }
    }

    /**
     * Constructor.
     * 
     * @param args Command-line arguments.
     */
    public AbstractConfig(String[] args)
    {
    	if (args == null) throw new NullArgumentException("args");
    	
    	_args = args;
    	_commandLine = null;
    	_replayParseError = null;
    }
    
    /**
     * Get configuration that is determined dynamically at runtime.
     * 
     * @return The current runtime configuration.
     */
    public final Fds.Config getRuntimeConfig()
    {
        if (_runtimeConfig == null)
        {
            _runtimeConfig = new Fds.Config("iodriver", _args);
        }
        return _runtimeConfig;
    }

    /**
     * Determine if help should be shown. This is true if either the user requests it or the
     * command-line cannot be parsed.
     *
     * @return Whether help should be shown.
     */
    public boolean isHelpNeeded()
    {
        try
        {
            return isHelpRequested();
        }
        catch (ParseException e)
        {
            return true;
        }
    }

    /**
     * Determine if we know that the user requested that help be shown.
     *
     * @return Whether the command-line was successfully parsed and the help option was specified.
     */
    public boolean isHelpExplicitlyRequested()
    {
        try
        {
            return isHelpRequested();
        }
        catch (ParseException e)
        {
            return false;
        }
    }

    /**
     * Determine if the user requested that help be shown.
     *
     * @return Whether the user requested that help be shown.
     *
     * @throws ParseException when command-line arguments could not be parsed.
     */
    public boolean isHelpRequested() throws ParseException
    {
        CommandLine commandLine = getCommandLine();

        return commandLine.hasOption("help");
    }

    /**
     * Show the command-line help on standard output.
     */
    public void showHelp()
    {
        showHelp(System.out);
    }

    /**
     * Show the command-line help.
     *
     * @param stream Where to show the help.
     */
    public void showHelp(PrintStream stream)
    {
        if (stream == null) throw new NullArgumentException("stream");

        // We intentionally don't close a stream we don't own.
        PrintWriter writer = new PrintWriter(stream);
        showHelp(writer);
    }

    /**
     * Show the command-line help.
     *
     * @param writer Where to show the help.
     */
    public void showHelp(PrintWriter writer)
    {
        if (writer == null) throw new NullArgumentException("writer");

        HelpFormatter formatter = new HelpFormatter();
        int width = Console.getWidth();
        Options options = getOptions();
        formatter.printHelp(writer, width, getProgramName(), null, options, 0, 2, null, true);

        writer.flush();
    }

    /**
     * Add supported command-line options.
     * 
     * @param options The options object to add to.
     */
    protected void addOptions(Options options)
    {
    	if (options == null) throw new NullArgumentException("options");
    	
		options.addOption("r", "fds-root", true, "Set the root folder for the FDS install.");
		options.addOption("h", "help", false, "Show this help screen.");
    }
    
    /**
     * Get the parsed command line.
     *
     * @return The current property value.
     * 
     * @throws ParseException when the command-line cannot be parsed.
     */
    protected final CommandLine getCommandLine() throws ParseException
    {
        if (_commandLine == null)
        {
            if (_replayParseError == null)
            {
                try
                {
                    Options options = getOptions();
                    CommandLineParser parser = new PosixParser();

                    _commandLine = parser.parse(options, _args);
                }
                catch (ParseException e)
                {
                    _replayParseError = e;
                    throw e;
                }
            }
            else
            {
                throw _replayParseError;
            }
        }
        return _commandLine;
    }
    
    protected final Optional<String> getCommandLineOptionValue(String longName)
    		throws ParseException
    {
    	CommandLine commandLine = getCommandLine();
    	if (commandLine.hasOption(longName))
    	{
    		return Optional.of(commandLine.getOptionValue(longName));
    	}
    	else
    	{
    		return Optional.empty();
    	}
    }
    
    /**
     * Get the supported command-line options.
     * 
     * @return The current property value.
     */
    protected final Options getOptions()
    {
    	if (_options == null)
    	{
    		Options newOptions = new Options();
    		addOptions(newOptions);
    		_options = newOptions;
    	}
    	
    	return _options;
    }

    /**
     * Get the name of the currently-running program.
     * 
     * @return The name of the command invoked.
     */
    protected abstract String getProgramName();
    
    /**
     * Raw command-line arguments.
     */
    private final String[] _args;

    /**
     * The parsed command-line. {@code null} prior to {@link #getCommandLine()}.
     */
    private CommandLine _commandLine;
    
    /**
     * The command-line options supported. {@code null} prior to {@link #getOptions()}.
     */
    private Options _options;

    /**
     * {@code null} unless an error occurred while parsing command-line options, the error that
     * occurred.
     */
    private ParseException _replayParseError;

    /**
     * Runtime configuration {@link #getRuntimeConfig()}.
     */
    private Fds.Config _runtimeConfig;
}
