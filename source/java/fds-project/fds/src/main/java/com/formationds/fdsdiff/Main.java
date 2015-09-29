package com.formationds.fdsdiff;

import static com.formationds.commons.util.ExceptionHelper.tunnel;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.Optional;
import java.util.function.Consumer;
import java.util.function.Supplier;

import org.apache.commons.cli.ParseException;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.util.logging.Logger;
import com.formationds.fdsdiff.SystemContent.VolumeWrapper;
import com.formationds.fdsdiff.workloads.GetObjectsDetailsWorkload;
import com.formationds.fdsdiff.workloads.GetSystemConfigWorkload;
import com.formationds.fdsdiff.workloads.GetVolumeObjectsWorkload;
import com.formationds.iodriver.ConfigurationException;
import com.formationds.iodriver.ExecutionException;
import com.formationds.iodriver.endpoints.Endpoint;
import com.formationds.iodriver.endpoints.FdsEndpoint;
import com.formationds.iodriver.model.BasicObjectManifest;
import com.formationds.iodriver.model.ComparisonDataFormat;
import com.formationds.iodriver.model.ExtendedObjectManifest;
import com.formationds.iodriver.model.FullObjectManifest;
import com.formationds.iodriver.model.ObjectManifest;
import com.formationds.iodriver.reporters.WorkloadEventListener;
import com.google.gson.Gson;

/**
 * Entry class for fdsdiff.
 */
public final class Main
{
    public static final int EXIT_SUCCESS;
    public static final int EXIT_FAILURE;
    public static final int EXIT_COMMAND_LINE_ERROR;
    public static final int EXIT_NO_MATCH;
    
    /**
     * Program entry point.
     * 
     * @param args Command-line arguments.
     */
    public static void main(String[] args)
    {
        int result = Integer.MIN_VALUE;
        Logger logger = null;
        try
        {
            if (args == null) throw new NullArgumentException("args");
            
            logger = Config.Defaults.getLogger();
            
            Config config = new Config(args);
            logger = config.getLogger();
            
            if (config.isHelpNeeded())
            {
                config.showHelp();
                if (config.isHelpExplicitlyRequested())
                {
                    result = EXIT_SUCCESS;
                }
                else
                {
                    result = EXIT_COMMAND_LINE_ERROR;
                }
            }
            else
            {
                result = _execPrimaryPath(config, logger);
            }
        }
        catch (Exception ex)
        {
            if (logger != null)
            {
                logger.logError("Unexpected exception.", ex);
            }
            result = 1;
        }
        
        System.exit(result);
    }
    
    static
    {
        EXIT_SUCCESS = 0;
        EXIT_FAILURE = 1;
        EXIT_COMMAND_LINE_ERROR = 2;
        EXIT_NO_MATCH = 3;
    }
    
    /**
     * Prevent instantiation.
     */
    private Main()
    {
        throw new UnsupportedOperationException("Trying to instantiate a utility class.");
    }
    
    private static int _execPrimaryPath(Config config, Logger logger)
            throws IOException, RuntimeException, ParseException, ExecutionException, ConfigurationException
    {
        if (config == null) throw new NullArgumentException("config");
        if (logger == null) throw new NullArgumentException("logger");
        
        ComparisonDataFormat format = config.getComparisonDataFormat();
        
        Optional<Path> inputAPath = config.getInputAPath();
        Optional<Path> inputBPath = config.getInputBPath();
        Optional<Path> outputAPath = config.getOutputAPath();
        Optional<Path> outputBPath = config.getOutputBPath();
        boolean haveInputA = inputAPath.isPresent();
        boolean haveInputB = inputBPath.isPresent();
        boolean haveOutputB = outputBPath.isPresent();
        boolean haveEndpointA = config.getEndpointAHost().isPresent();
        boolean haveEndpointB = config.getEndpointBHost().isPresent();
        if ((haveInputA && haveEndpointA) || (haveInputB && haveEndpointB))
        {
            logger.logError("Cannot specify both input file and endpoint.");
            return EXIT_COMMAND_LINE_ERROR;
        }
        else if (haveOutputB && !haveInputA && !haveEndpointB)
        {
            logger.logError("No endpoint specified for output-b.");
            return EXIT_COMMAND_LINE_ERROR;
        }
        else
        {
            Gson gson = new Gson();

            // First try to deserialize any provided input files.
            Optional<SystemContent> aContentWrapper =
                    tunnel(IOException.class,
                           in -> { return inputAPath.map(in); },
                           (Path path) -> _getSystemContent(path, gson));
            Optional<SystemContent> bContentWrapper =
                    tunnel(IOException.class,
                           in -> { return inputBPath.map(in); },
                           (Path path) -> _getSystemContent(path, gson));
            
            // We can only use the default endpoint once.
            boolean defaultEndpointUsed = false;

            // We always have an "A" content--if absolutely nothing is specified on the command-
            // line, we gather data from the default host (defined by FDS config) and output to
            // stdout.
            if (!aContentWrapper.isPresent())
            {
                Optional<FdsEndpoint> endpointAWrapper = config.getEndpointA();
                if (!endpointAWrapper.isPresent())
                {
                    defaultEndpointUsed = true;
                    endpointAWrapper = Optional.of(config.getDefaultEndpoint());
                }
                FdsEndpoint endpointA = endpointAWrapper.get();
                aContentWrapper = Optional.of(_getSystemContent(endpointA, logger, format));
            }
            SystemContent aContent = aContentWrapper.get();
            
            // Spit out the content if requested.
            if (outputAPath.isPresent())
            {
                _outputContent(outputAPath.get(), aContent, gson);
            }
            
            // "B" content is optional.
            if (!bContentWrapper.isPresent())
            {
                Optional<FdsEndpoint> endpointBWrapper = config.getEndpointB();
                if (!endpointBWrapper.isPresent() && !defaultEndpointUsed)
                {
                    defaultEndpointUsed = true;
                    endpointBWrapper = Optional.of(config.getDefaultEndpoint());
                }
                if (endpointBWrapper.isPresent())
                {
                    FdsEndpoint endpointB = endpointBWrapper.get();
                    bContentWrapper = Optional.of(_getSystemContent(endpointB, logger, format));
                }
            }

            // We have as much content as we're going to get. Can we do a compare?
            if (bContentWrapper.isPresent())
            {
                SystemContent bContent = bContentWrapper.get();
                
                if (outputBPath.isPresent())
                {
                    _outputContent(outputBPath.get(), bContent, gson);
                }
                
                if (aContent.equals(bContentWrapper.get()))
                {
                    return EXIT_SUCCESS;
                }
                else
                {
                    return EXIT_NO_MATCH;
                }
            }
            else
            {
                // If we weren't able to do a compare (we're in this else), the only action that
                // can be taken is to output the comparison file. If nobody requested the output,
                // spit it out on stdout anyway.
                if (!outputAPath.isPresent())
                {
                    _outputContent(aContent, gson);
                }
                return EXIT_SUCCESS;
            }
        }
    }
    
    private static void _outputContent(Appendable writer, SystemContent content, Gson gson)
    {
        if (writer == null) throw new NullArgumentException("writer");
        if (content == null) throw new NullArgumentException("content");
        if (gson == null) throw new NullArgumentException("gson");
        
        gson.toJson(content, writer);
    }
    
    private static void _outputContent(Path path, SystemContent content, Gson gson) throws IOException
    {
        if (path == null) throw new NullArgumentException("path");
        if (content == null) throw new NullArgumentException("content");
        if (gson == null) throw new NullArgumentException("gson");
        
        try (BufferedWriter writer = Files.newBufferedWriter(path,
                                                             StandardCharsets.UTF_8,
                                                             StandardOpenOption.CREATE,
                                                             StandardOpenOption.TRUNCATE_EXISTING))
        {
            _outputContent(writer, content, gson);
        }
    }
    
    private static void _outputContent(SystemContent content, Gson gson) throws IOException
    {
        _outputContent(System.out, content, gson);
    }
    
    private static ObjectManifest.Builder<?, ?> _getBasicBuilderSupplier()
    {
        return new BasicObjectManifest.ConcreteBuilder();
    }
    
    private static Supplier<ObjectManifest.Builder<?, ?>> _getBuilderSupplier(
            ComparisonDataFormat format)
    {
        switch (format)
        {
        case FULL: return Main::_getFullBuilderSupplier;
        case EXTENDED: return Main::_getExtendedBuilderSupplier;
        case BASIC: return Main::_getBasicBuilderSupplier;
        case MINIMAL: throw new IllegalArgumentException("Minimal format does not need this.");
        default: throw new IllegalArgumentException(format + " not recognized.");
        }
    }
    
    private static ObjectManifest.Builder<?, ?> _getExtendedBuilderSupplier()
    {
        return new ExtendedObjectManifest.ConcreteBuilder();
    }
    
    private static ObjectManifest.Builder<?, ?> _getFullBuilderSupplier()
    {
        return new FullObjectManifest.ConcreteBuilder();
    }

    private static SystemContent _getSystemContent(Path serializedComparisonFile,
                                                   Gson gson) throws IOException
    {
        if (serializedComparisonFile == null) throw new NullArgumentException(
                "serializedComparisonFile");
        
        try (BufferedReader reader = Files.newBufferedReader(serializedComparisonFile))
        {
            return gson.fromJson(reader, SystemContent.class);
        }
    }
    
    private static SystemContent _getSystemContent(Endpoint endpoint,
                                                   Logger logger,
                                                   ComparisonDataFormat format)
            throws ExecutionException
    {
        if (endpoint == null) throw new NullArgumentException("endpoint");
        if (logger == null) throw new NullArgumentException("logger");
        
        SystemContent content = new SystemContent();
        
        // FIXME: Log operations should be configurable.
        GetSystemConfigWorkload getSystemConfig = new GetSystemConfigWorkload(content, true);
        WorkloadEventListener listener = new WorkloadEventListener(logger);
        getSystemConfig.runOn(endpoint, listener);
        
        // Now gather individual volume contents.
        for (VolumeWrapper volumeWrapper : content.getVolumes())
        {
            String volumeName = volumeWrapper.getVolume().getName();
            
            // First just get object names.
            Consumer<String> objectNameSetter = content.getVolumeObjectNameAdder(volumeWrapper);
            GetVolumeObjectsWorkload getVolumeObjects =
                    new GetVolumeObjectsWorkload(volumeName, objectNameSetter, true);
            getVolumeObjects.runOn(endpoint, listener);
            
            // Now fill in the details (minimal doesn't have any details).
            if (format != ComparisonDataFormat.MINIMAL)
            {
                GetObjectsDetailsWorkload getObjectsDetails =
                        new GetObjectsDetailsWorkload(volumeName,
                                                      content.getObjectNames(volumeWrapper),
                                                      _getBuilderSupplier(format),
                                                      content.getVolumeObjectAdder(volumeWrapper),
                                                      true);
                getObjectsDetails.runOn(endpoint, listener);
            }
        }
        
        return content;
    }
}
