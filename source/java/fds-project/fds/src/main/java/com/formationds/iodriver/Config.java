package com.formationds.iodriver;

import java.lang.reflect.AnnotatedElement;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Member;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.net.MalformedURLException;
import java.net.URI;
import java.util.Arrays;
import java.util.Collection;
import java.util.EnumSet;
import java.util.function.Function;
import java.util.function.Predicate;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

import com.formationds.commons.AbstractConfig;
import com.formationds.commons.Fds;
import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.ConsoleLogger;
import com.formationds.commons.util.logging.Logger;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.endpoints.OrchestrationManagerEndpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
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
public final class Config extends AbstractConfig
{
    /**
     * Actions that can be logged.
     */
    public enum LogTargets
    {
        /**
         * Log all operations performed.
         */
        OPERATIONS
    }
    
    /**
     * Default static configuration not pulled from command-line.
     */
    public final static class Defaults extends AbstractConfig.Defaults
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
            try
            {
                URI s3Endpoint = Fds.getS3Endpoint();
                String s3EndpointText = s3Endpoint.toString();

                URI apiBase = Fds.Api.getBase();
                URI v8ApiBase = Fds.Api.V08.getBase();
                OrchestrationManagerEndpoint omEndpoint =
                        new OrchestrationManagerEndpoint(apiBase,
                                                         "admin",
                                                         "admin",
                                                         AbstractConfig.Defaults.getLogger(),
                                                         true,
                                                         new OrchestrationManagerEndpoint(v8ApiBase,
                                                                                          "admin",
                                                                                          "admin",
                                                                                          AbstractConfig.Defaults.getLogger(),
                                                                                          true,
                                                                                          null));

                _endpoint = new S3Endpoint(s3EndpointText, omEndpoint, AbstractConfig.Defaults.getLogger());
            }
            catch (MalformedURLException e)
            {
                // Should be impossible.
                throw new IllegalStateException(e);
            }
            _listener = new WorkflowEventListener(AbstractConfig.Defaults.getLogger());
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
    	super(args);
        
        _availableWorkloadNames = null;
        _disk_iops_max = -1;
        _disk_iops_min = -1;
        _workloadName = null;
    }

    /**
     * Get the default assured-rate test workload.
     *
     * @return A workload.
     *
     * @throws ConfigUndefinedException when the system throttle IOPS rate is not defined.
     * @throws ParseException when the command-line cannot be parsed.
     */
    @WorkloadProvider
    public S3AssuredRateTestWorkload getAssuredWorkload() throws ConfigUndefinedException,
                                                                 ParseException
    {
        final int competingBuckets = 2;
        final int systemThrottle = getSystemIopsMax();
        
        return new S3AssuredRateTestWorkload(competingBuckets,
                                             systemThrottle,
                                             getOperationLogging());
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
     * Things that should be logged.
     * 
     * @return The current property value.
     * @throws ParseException when the command-line cannot be parsed.
     */
    public EnumSet<LogTargets> getLogTargets() throws ParseException
    {
        if (_logTargets == null)
        {
            CommandLine commandLine = getCommandLine();
            if (commandLine.hasOption("debug"))
            {
                _logTargets = EnumSet.copyOf(StreamSupport.stream(
                        Arrays.asList(commandLine.getOptionValues("debug")).spliterator(),
                        false).map(logTarget -> Enum.valueOf(LogTargets.class, logTarget))
                              .collect(Collectors.toList()));
            }
            else
            {
                _logTargets = EnumSet.noneOf(LogTargets.class);
            }
        }
        
        return _logTargets;
    }
    
    /**
     * Get whether to log individual operations.
     * 
     * @return The configured property value.
     * @throws ParseException when the command-line cannot be parsed.
     */
    public boolean getOperationLogging() throws ParseException
    {
        return getLogTargets().contains(LogTargets.OPERATIONS);
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
     * Get the default rate-limit test workload.
     * 
     * @return A workload.
     * 
     * @throws ConfigurationException when the system assured and throttle IOPS rates are not within
     *             a testable range.
     * @throws ParseException when the command-line cannot be parsed.
     */
    @WorkloadProvider
    public S3RateLimitTestWorkload getRateLimitWorkload() throws ConfigurationException,
                                                                 ParseException
    {
        final int systemAssured = getSystemIopsMin();
        final int systemThrottle = getSystemIopsMax();
        final int headroomNeeded = 50;

        if (systemAssured <= 0)
        {
            throw new ConfigurationException("System-wide assured rate of " + systemAssured
                                             + " does not result in a sane configuration.");
        }
        if (systemThrottle < headroomNeeded)
        {
            throw new ConfigurationException("System-wide throttle of " + systemThrottle
                                             + " leaves less than " + headroomNeeded
                                             + " IOPS of headroom, not enough for a good test.");
        }

        return new S3RateLimitTestWorkload(systemThrottle - headroomNeeded, getOperationLogging());
    }

    /**
     * Get the user-selected workload.
     * 
     * @param endpointType The type of endpoint the workload will target.
     * @param operationType The type of operation the workload can run.
     * 
     * @return A workload.
     * 
     * @throws ParseException when the command-line arguments cannot be parsed.
     * @throws ConfigurationException when system configuration is invalid.
     */
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
            // HACK: We should allow the annotation to specify the workload name, and then store
            //       a map. We definitely shouldn't be adding and removing the pre/suffixes in two
            //       very different places (see getAvailableWorkloadNames()).
            workloadFactoryMethod = myClass.getMethod("get" + workloadName + "Workload");
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

    @Override
    protected void addOptions(Options options)
    {
    	if (options == null) throw new NullArgumentException("options");
    	
        options.addOption("d",
                          "debug",
                          true,
                          "Operations to debug. Available operations are "
                          + String.join(", ",
                                        StreamSupport.stream(Arrays.asList(LogTargets.values())
                                                                   .spliterator(),
                                                             false)
                                                     .map(logTarget -> logTarget.toString())
                                                     .collect(Collectors.toList())));
        options.addOption("w",
                          "workload",
                          true,
                          "The workload to run. Available options are "
                          + String.join(", ", getAvailableWorkloadNames()) + ".");
    }

    @Override
    protected String getProgramName()
    {
    	return "iodriver";
    }
    
    /**
     * Workloads that may be chosen to run, {@code null} prior to
     * {@link #getAvailableWorkloadNames()}.
     */
    private Collection<String> _availableWorkloadNames;

    /**
     * Maximum IOPS allowed by the system, {@code -1} prior to {@link #getSystemIopsMax()}.
     */
    private int _disk_iops_max;

    /**
     * Minimum IOPS guaranteed by the system, {@code -1} prior to {@link #getSystemIopsMin()}.
     */
    private int _disk_iops_min;

    /**
     * Things that should be logged.
     */
    private EnumSet<LogTargets> _logTargets;

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
            Function<Method, String> getWorkloadName =
                    method ->
                    {
                        return method.getName()
                                     .replaceAll("^get", "")
                                     .replaceAll("Workload$", "");
                    };

            Stream<Method> myMethods = Stream.of(myClass.getMethods());
            Stream<Method> myPublicMethods = myMethods.filter(memberIsPublic);
            Stream<Method> myAnnotatedMethods = myPublicMethods.filter(memberHasAnnotation);
            Stream<Method> methodsWithNoArguments = myAnnotatedMethods.filter(methodHasNoArguments);
            Stream<Method> methodsWithCorrectReturnType =
                    methodsWithNoArguments.filter(methodHasCorrectReturnType);
            Stream<String> availableWorkloadNames =
                    methodsWithCorrectReturnType.map(getWorkloadName);

            _availableWorkloadNames = availableWorkloadNames.collect(Collectors.toSet());
        }
        return _availableWorkloadNames;
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
