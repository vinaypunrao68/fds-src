package com.formationds.iodriver;

import java.io.PrintStream;
import java.io.PrintWriter;
import java.lang.reflect.AnnotatedElement;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.net.MalformedURLException;
import java.net.URI;
import java.util.Collection;
import java.util.function.Predicate;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.cli.PosixParser;

import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
import com.formationds.iodriver.logging.ConsoleLogger;
import com.formationds.iodriver.logging.Logger;
import com.formationds.iodriver.operations.Operation;
import com.formationds.iodriver.reporters.WorkflowEventListener;
import com.formationds.iodriver.validators.RateLimitValidator;
import com.formationds.iodriver.validators.Validator;
import com.formationds.iodriver.workloads.S3AssuredRateTestWorkload;
import com.formationds.iodriver.workloads.S3RateLimitTestWorkload;
import com.formationds.iodriver.workloads.Workload;

/**
 * Global configuration for {@link com.formationds.iodriver}.
 */
public final class Config
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
                return defaultValue;
            }
        }
    }

    /**
     * Default static configuration not pulled from command-line.
     */
    public final static class Defaults
    {
        /**
         * Get the default endpoint.
         * 
         * @return An endpoint to the local system default S3 interface.
         */
        public static S3Endpoint getEndpoint()
        {
            return _endpoint;
        }

        /**
         * Get the default event listener.
         * 
         * @return A stats-gathering and 1-input-many-output event hub.
         */
        public static WorkflowEventListener getListener()
        {
            return _listener;
        }

        /**
         * Get the default logger.
         * 
         * @return A system console logger that writes to standard output.
         */
        public static Logger getLogger()
        {
            return _logger;
        }

        /**
         * Get the default validator.
         *
         * @return A validator that ensures a volumes IOPS did not exceed its configured throttle.
         */
        public static Validator getValidator()
        {
            return _validator;
        }

        static
        {
            Logger newLogger = new ConsoleLogger();

            try
            {
                URI s3Endpoint = Fds.getS3Endpoint();
                String s3EndpointText = s3Endpoint.toString();

                URI apiBase = Fds.Api.getBase();
                OrchestrationManagerEndpoint omEndpoint =
                        new OrchestrationManagerEndpoint(apiBase, "admin", "admin", newLogger, true);

                _endpoint = new S3Endpoint(s3EndpointText, omEndpoint, newLogger);
            }
            catch (MalformedURLException e)
            {
                // Should be impossible.
                throw new IllegalStateException(e);
            }
            _listener = new WorkflowEventListener();
            _logger = newLogger;
            _validator = new RateLimitValidator();
        }

        /**
         * Prevent instantiation.
         */
        private Defaults()
        {
            throw new UnsupportedOperationException("Instantiating a utility class.");
        }

        /**
         * Default endpoint.
         */
        private static final S3Endpoint _endpoint;

        /**
         * Default event listener.
         */
        private static final WorkflowEventListener _listener;

        /**
         * Default logger.
         */
        private static final Logger _logger;

        /**
         * Default validator.
         */
        private static final Validator _validator;
    }

    /**
     * Constructor.
     * 
     * @param args Command-line arguments.
     */
    public Config(String[] args)
    {
        if (args == null) throw new NullArgumentException("args");

        _args = args;
        _availableWorkloadNames = null;
        _commandLine = null;
        _disk_iops_max = -1;
        _disk_iops_min = -1;
        _workloadName = null;
    }

    /**
     * Get the endpoint to run workloads on.
     * 
     * @return The specified configuration.
     */
    public S3Endpoint getEndpoint()
    {
        // TODO: Allow this to be specified.
        return Defaults.getEndpoint();
    }

    /**
     * Get configuration that is determined dynamically at runtime.
     * 
     * @return The current runtime configuration.
     */
    public Fds.Config getRuntimeConfig()
    {
        if (_runtimeConfig == null)
        {
            _runtimeConfig = new Fds.Config("iodriver", _args);
        }
        return _runtimeConfig;
    }

    /**
     * Get the configured listener.
     * 
     * @return An event hub.
     */
    public WorkflowEventListener getListener()
    {
        // TODO: ALlow this to be configured.
        return Defaults.getListener();
    }

    /**
     * Get the configured logger.
     * 
     * @return A logger.
     */
    public Logger getLogger()
    {
        // TODO: Allow this to be configured.
        return Defaults.getLogger();
    }

    /**
     * Get the maximum IOPS this system will allow.
     * 
     * @return A number of IOPS.
     * 
     * @throws ConfigUndefinedException when {@value Fds.Config#DISK_IOPS_MAX_CONFIG} is not defined
     *             in platform.conf.
     */
    public int getSystemIopsMax() throws ConfigUndefinedException
    {
        if (_disk_iops_max < 0)
        {
            _disk_iops_max = getRuntimeConfig().getIopsMax();
        }
        return _disk_iops_max;
    }

    /**
     * Get the guaranteed IOPS this system can support.
     * 
     * @return A number of IOPS.
     * 
     * @throws ConfigUndefinedException when {@value Fds.Config#DISK_IOPS_MIN_CONFIG} is not defined
     *             in platform.conf.
     */
    public int getSystemIopsMin() throws ConfigUndefinedException
    {
        if (_disk_iops_min < 0)
        {
            _disk_iops_min = getRuntimeConfig().getIopsMin();
        }
        return _disk_iops_min;
    }

    /**
     * Get the default assured-rate test workload.
     * 
     * @return A workload.
     * 
     * @throws ConfigUndefinedException when the system throttle IOPS rate is not defined.
     */
    @WorkloadProvider
    public S3AssuredRateTestWorkload getAssuredWorkload() throws ConfigUndefinedException
    {
        final int competingBuckets = 2;
        final int systemThrottle = getSystemIopsMax();

        return new S3AssuredRateTestWorkload(competingBuckets, systemThrottle);
    }

    /**
     * Get the default rate-limit test workload.
     * 
     * @return A workload.
     * 
     * @throws ConfigurationException when the system assured and throttle IOPS rates are not within
     *             a testable range.
     */
    @WorkloadProvider
    public S3RateLimitTestWorkload getRateLimitWorkload() throws ConfigurationException
    {
        final int systemAssured = getSystemIopsMin();
        final int systemThrottle = getSystemIopsMax();
        final int headroomNeeded = 50;

        if (systemAssured <= 0)
        {
            throw new ConfigurationException("System-wide assured rate of " + systemAssured
                                             + " does not result in a sane configuration.");
        }
        if (systemAssured > systemThrottle - headroomNeeded)
        {
            throw new ConfigurationException("System-wide throttle of " + systemThrottle
                                             + " leaves less than " + headroomNeeded
                                             + " IOPS of headroom over system assured "
                                             + systemAssured + " IOPS.");
        }

        return new S3RateLimitTestWorkload(systemAssured + headroomNeeded);
    }

    // @eclipseFormat:off
    public <WorkloadT extends Workload<EndpointT, OperationT>,
            EndpointT extends Endpoint<EndpointT, OperationT>,
            OperationT extends Operation<OperationT, EndpointT>>
    WorkloadT getSelectedWorkload(Class<EndpointT> endpointType,
                                  Class<OperationT> operationType)
    throws ParseException, ConfigurationException
    // @eclipseFormat:on
    {
        if (endpointType == null) throw new NullArgumentException("endpointType");
        if (operationType == null) throw new NullArgumentException("operationType");

        String workloadName = getSelectedWorkloadName();
        Class<?> myClass = getClass();
        Method workloadFactoryMethod;
        try
        {
            workloadFactoryMethod = myClass.getMethod(workloadName);
        }
        catch (NoSuchMethodException | SecurityException e)
        {
            throw new ConfigurationException("No such method " + workloadName + "().");
        }

        Workload<?, ?> workload;
        try
        {
            try
            {
                workload = (Workload<?, ?>)workloadFactoryMethod.invoke(this);
            }
            catch (InvocationTargetException e)
            {
                Throwable cause = e.getCause();
                if (cause == null)
                {
                    throw new ConfigurationException("Invocation exception while trying to build "
                                                     + "workload " + workloadName
                                                     + ", but null cause.",
                                                     e);
                }
                throw cause;
            }
        }
        catch (ConfigurationException e)
        {
            throw e;
        }
        catch (Throwable t)
        {
            throw new ConfigurationException("Unexpected error building workload.", t);
        }

        if (!workload.getEndpointType().equals(endpointType))
        {
            throw new ConfigurationException("Workload endpoint type of "
                                             + workload.getEndpointType().getName()
                                             + " is not the requested type of " + endpointType
                                             + ".");
        }
        if (!workload.getOperationType().equals(operationType))
        {
            throw new ConfigurationException("Workload operation type of "
                                             + workload.getOperationType().getName()
                                             + " is not the requested type of " + operationType
                                             + ".");
        }
        @SuppressWarnings("unchecked")
        WorkloadT retval = (WorkloadT)workload;

        return retval;
    }

    /**
     * Get the configured validator.
     * 
     * @return A validator.
     */
    public Validator getValidator()
    {
        // TODO: Allow this to be configured.
        return Defaults.getValidator();
    }

    /**
     * Determine if help should be shown. This is true if either the user requests it or the
     * command-line cannot be parsed.
     * 
     * @return Whether help should be shown.
     */
    public boolean isHelpNeeded()
    {
        CommandLine commandLine;
        try
        {
            commandLine = getCommandLine();
        }
        catch (ParseException e)
        {
            return true;
        }
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
        formatter.printHelp(writer, width, "iodriver", null, options, 0, 0, null, true);
    }

    /**
     * Raw command-line arguments.
     */
    private final String[] _args;

    /**
     * Workloads that may be chosen to run, {@code null} prior to
     * {@link #getAvailableWorkloadNames()}.
     */
    private Collection<String> _availableWorkloadNames;

    /**
     * The parsed command-line. {@code null} prior to {@link #getCommandLine()}.
     */
    private CommandLine _commandLine;

    /**
     * Maximum IOPS allowed by the system, {@code -1} prior to {@link #getSystemIopsMax()}.
     */
    private int _disk_iops_max;

    /**
     * Minimum IOPS guaranteed by the system, {@code -1} prior to {@link #getSystemIopsMin()}.
     */
    private int _disk_iops_min;

    /**
     * The command-line options supported. {@code null} prior to {@link #getOptions()}.
     */
    private Options _options;

    /**
     * Runtime configuration {@link #getRuntimeConfig()}.
     */
    private Fds.Config _runtimeConfig;

    /**
     * The name of the user-selected workload. {@code null} prior to
     * {@link #getSelectedWorkloadName()} or if not specified.
     */
    private String _workloadName;

    /**
     * Get the names of methods providing selectable workloads.
     * 
     * @return A list of names.
     */
    private Collection<String> getAvailableWorkloadNames()
    {
        if (_availableWorkloadNames == null)
        {
            Class<?> myClass = getClass();

            Predicate<Member> memberIsPublic = member -> Modifier.isPublic(member.getModifiers());
            Predicate<AnnotatedElement> memberHasAnnotation =
                    member ->
                    {
                        WorkloadProvider[] workloadProviderAnnotations =
                                member.getAnnotationsByType(WorkloadProvider.class);
                        return workloadProviderAnnotations.length > 0;
                    };
            Predicate<Method> methodHasNoArguments = method -> method.getParameterCount() == 0;
            Predicate<Method> methodHasCorrectReturnType =
                    method -> Workload.class.isAssignableFrom(method.getReturnType());

            Stream<Method> myMethods = Stream.of(myClass.getMethods());
            Stream<Method> myPublicMethods = myMethods.filter(memberIsPublic);
            Stream<Method> myAnnotatedMethods = myPublicMethods.filter(memberHasAnnotation);
            Stream<Method> methodsWithNoArguments = myAnnotatedMethods.filter(methodHasNoArguments);
            Stream<Method> methodsWithCorrectReturnType =
                    methodsWithNoArguments.filter(methodHasCorrectReturnType);
            Stream<String> availableWorkloadMethodNames =
                    methodsWithCorrectReturnType.map(method -> method.getName());

            _availableWorkloadNames = availableWorkloadMethodNames.collect(Collectors.toSet());
        }
        return _availableWorkloadNames;
    }

    /**
     * Get the parsed command line.
     * 
     * @return The current property value.
     * @throws ParseException
     */
    private CommandLine getCommandLine() throws ParseException
    {
        if (_commandLine == null)
        {
            Options options = getOptions();
            CommandLineParser parser = new PosixParser();

            _commandLine = parser.parse(options, _args);
        }
        return _commandLine;
    }

    /**
     * Get the supported command-line options.
     * 
     * @return The current property value.
     */
    private Options getOptions()
    {
        if (_options == null)
        {
            Options newOptions = new Options();
            newOptions.addOption("h", "help", false, "Show this help screen.");
            newOptions.addOption("w",
                                 "workload",
                                 true,
                                 "The workload to run. Available options are "
                                         + String.join(", ", getAvailableWorkloadNames()) + ".");
            
            _options = newOptions;
        }
        return _options;
    }

    /**
     * Get the name of the selected workload.
     * 
     * @return The name of a method in this class that takes no arguments and returns a
     *         {@link Workload}.
     * 
     * @throws ParseException when the command-line arguments cannot be parsed.
     * @throws ConfigurationException when no workloads are configured.
     */
    private String getSelectedWorkloadName() throws ParseException, ConfigurationException
    {
        if (_workloadName == null)
        {
            CommandLine commandLine = getCommandLine();

            Collection<String> availableWorkloadNames = getAvailableWorkloadNames();
            if (availableWorkloadNames.isEmpty())
            {
                throw new ConfigurationException("No available workloads.");
            }

            if (commandLine.hasOption("workload"))
            {
                String selectedWorkloadName = commandLine.getOptionValue("workload");
                if (selectedWorkloadName == null
                    || !availableWorkloadNames.contains(selectedWorkloadName))
                {
                    throw new ParseException("workload must specify a valid workload name.");
                }

                _workloadName = selectedWorkloadName;
            }
            else
            {
                _workloadName = availableWorkloadNames.iterator().next();
            }
        }
        return _workloadName;
    }
}
