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
import com.formationds.commons.RuntimeConfig;
import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.endpoints.OmV7Endpoint;
import com.formationds.iodriver.endpoints.OmV8Endpoint;
import com.formationds.iodriver.endpoints.S3Endpoint;
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
        public static FdsEndpoint getFdsEndpoint()
        {
            return _fdsEndpoint;
        }
        
        public static OmV7Endpoint getOmV7Endpoint()
        {
            return _omV7Endpoint;
        }
        
        /**
         * Get the default OM v8 API endpoint.
         *
         * @return An endpoint to the local system.
         */
        public static OmV8Endpoint getOmV8Endpoint()
        {
            return _omV8Endpoint;
        }

        /**
         * Get the default S3 endpoint.
         * 
         * @return An endpoint to the local system.
         */
        public static S3Endpoint getS3Endpoint()
        {
            return _s3Endpoint;
        }

        static
        {
            try
            {
                URI s3EndpointUrl = Fds.getS3Endpoint();
                String s3EndpointText = s3EndpointUrl.toString();

                @SuppressWarnings("deprecation")
                URI v7ApiBase = Fds.Api.V07.getBase();
                URI v8ApiBase = Fds.Api.V08.getBase();
                OmV8Endpoint omEndpointV8 = new OmV8Endpoint(v8ApiBase,
                                                             "admin",
                                                             "admin",
                                                             AbstractConfig.Defaults.getLogger(),
                                                             true);
                OmV7Endpoint omEndpointV7 = new OmV7Endpoint(v7ApiBase,
                                                             "admin",
                                                             "admin",
                                                             AbstractConfig.Defaults.getLogger(),
                                                             true);
                S3Endpoint s3 = new S3Endpoint(s3EndpointText,
                                               omEndpointV8,
                                               AbstractConfig.Defaults.getLogger());

                _fdsEndpoint = new FdsEndpoint(omEndpointV7, omEndpointV8, s3);
                _s3Endpoint = s3;
                _omV7Endpoint = omEndpointV7;
                _omV8Endpoint = omEndpointV8;
            }
            catch (MalformedURLException e)
            {
                // Should be impossible.
                throw new IllegalStateException(e);
            }
        }

        /**
         * Prevent instantiation.
         */
        private Defaults()
        {
            throw new UnsupportedOperationException("Instantiating a utility class.");
        }

        private static final FdsEndpoint _fdsEndpoint;

        /**
         * Default S3 endpoint.
         */
        private static final S3Endpoint _s3Endpoint;

        private static final OmV7Endpoint _omV7Endpoint;
        
        /**
         * Default version-8 API OM endpoint.
         */
        private static final OmV8Endpoint _omV8Endpoint;
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

    @Override
    public void addOptions(Options options)
    {
    	if (options == null) throw new NullArgumentException("options");
    	
    	super.addOptions(options);
    	
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
                                             systemThrottle);
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

        return new S3RateLimitTestWorkload(systemThrottle - headroomNeeded);
    }

    /**
     * Get the user-selected workload.
     * 
     * @return A workload.
     * 
     * @throws ParseException when the command-line arguments cannot be parsed.
     * @throws ConfigurationException when system configuration is invalid.
     */
    // @eclipseFormat:off
    public Workload getSelectedWorkload() throws ParseException, ConfigurationException
    // @eclipseFormat:on
    {
        String workloadName = getSelectedWorkloadName();
        Method workloadFactoryMethod = null;
        RuntimeConfig workloadFactory = null;
        try
        {
            Stream<RuntimeConfig> configs = Stream.concat(Stream.of(this),
                                                          getConfigs().stream());
            
        	NoSuchMethodException finalE = null;
        	for (RuntimeConfig config : configs.toArray(size -> new RuntimeConfig[size]))
        	{
        	    Class<?> clazz = config.getClass();
        		try
        		{
                    // HACK: We should allow the annotation to specify the workload name, and then
        			//       store a map. We definitely shouldn't be adding and removing the
        			//       pre/suffixes in two very different places (see
        			//       getAvailableWorkloadNames()).
        			workloadFactoryMethod = clazz.getMethod("get" + workloadName + "Workload");
        			workloadFactory = config;
        		}
        		catch (NoSuchMethodException e)
        		{
        			if (finalE == null)
        			{
        				finalE = e;
        			}
        			else
        			{
        				finalE.addSuppressed(e);
        			}
        		}
        	}
        	
            if (workloadFactoryMethod == null)
            {
            	if (finalE == null)
            	{
            		throw new ConfigurationException("No method get" + workloadName + "Workload() "
            										 + "found.");
            	}
            	else
            	{
            		throw new ConfigurationException("Error looking up workload method get"
            										 + workloadName + "Workload().", finalE);
            	}
            }
        }
        catch (SecurityException e)
        {
            throw new ConfigurationException("Security error looking up method name get"
            						         + workloadName + "Workload().", e);
        }

        Workload workload;
        try
        {
            try
            {
                workload = (Workload)workloadFactoryMethod.invoke(workloadFactory);
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

        return workload;
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

        	Class<?>[] classes =
        			Stream.concat(Stream.of(getClass()),
        			              getConfigs().stream().map(c -> c.getClass()))
        			      .toArray(size -> new Class[size]);
        	@SuppressWarnings("unchecked")
            Class<? extends RuntimeConfig>[] typedClasses =
        	        (Class<? extends RuntimeConfig>[])classes;
        	
        	Stream<Method> methods = Arrays.asList(typedClasses)
        	                               .stream().flatMap(c -> Arrays.asList(c.getMethods())
        	                                                            .stream());
        	Stream<Method> publicMethods = methods.filter(memberIsPublic);
            Stream<Method> annotatedMethods = publicMethods.filter(memberHasAnnotation);
            Stream<Method> methodsWithNoArguments = annotatedMethods.filter(methodHasNoArguments);
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
