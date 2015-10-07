package com.formationds.iodriver.workloads;

import java.util.HashSet;
import java.util.Set;

import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

import com.formationds.commons.NullArgumentException;
import com.formationds.commons.RuntimeConfig;
import com.formationds.iodriver.Config;
import com.formationds.iodriver.WorkloadProvider;

public class RandomFillConfig implements RuntimeConfig
{
	public RandomFillConfig(Config baseConfig)
	{
	    if (baseConfig == null) throw new NullArgumentException("baseConfig");
	    
	    _baseConfig = baseConfig;
		_configs = new HashSet<>();
		_maxDirectoriesPerLevel = null;
		_maxDirectoryDepth = null;
		_maxObjectSize = null;
		_maxObjectsPerDirectory = null;
		_maxVolumes = null;
		_pathSeparator = null;
	}
	
	@Override
	public void addConfig(RuntimeConfig config)
	{
	    if (config == null) throw new NullArgumentException("config");
	    
		_configs.add(config);
	}
	
	@Override
	public void addOptions(Options options)
	{
	    if (options == null) throw new NullArgumentException("options");
	    
	    _configs.forEach(c -> c.addOptions(options));
	    
	    options.addOption(null,
	                      "maxDirectoriesPerLevel",
	                      true,
	                      "Add from one to this many directories at each level, random each time.");
	    options.addOption(null,
	                      "maxDirectoryDepth",
	                      true,
	                      "Directories will not exceed this depth.");
	    options.addOption(null,
	                      "maxObjectSize",
	                      true,
	                      "Create objects of 0 to this many bytes, random each time.");
	    options.addOption(null,
	                      "maxObjectsPerDirectory",
	                      true,
	                      "Put up to this many objects in a directory.");
		options.addOption(null,
		                  "maxVolumes",
		                  true, "Add from 1 to this many volumes, random number.");
		options.addOption(null, "pathSeparator", true, "Separates directories.");
		
	}
	
	public int getMaxDirectoriesPerLevel() throws ParseException
	{
	    return _maxDirectoriesPerLevel == null
	           ? (_maxDirectoriesPerLevel =
	                   _baseConfig.getCommandLineOptionValue("maxDirectoriesPerLevel")
	                              .map(Integer::parseInt)
	                              .orElse(5))
	          : _maxDirectoriesPerLevel;
	}
	
	public int getMaxDirectoryDepth() throws ParseException
	{
	    return _maxDirectoryDepth == null
	           ? (_maxDirectoryDepth = _baseConfig.getCommandLineOptionValue("maxDirectoryDepth")
	                                              .map(Integer::parseInt)
	                                              .orElse(3))
	           : _maxDirectoryDepth;
	}
	
	public int getMaxObjectSize() throws ParseException
	{
	    return _maxObjectSize == null
	           ? (_maxObjectSize = _baseConfig.getCommandLineOptionValue("maxObjectSize")
	                                          .map(Integer::parseInt)
	                                          .orElse(200 * 1024 * 1024
	                                                  / (getMaxVolumes()
	                                                     * getMaxDirectoriesPerLevel()
	                                                     * getMaxDirectoryDepth()
	                                                     * getMaxObjectsPerDirectory())))
	           : _maxObjectSize;
	}
	
	public int getMaxObjectsPerDirectory() throws ParseException
	{
	    return _maxObjectsPerDirectory == null
	           ? (_maxObjectsPerDirectory =
	                   _baseConfig.getCommandLineOptionValue("maxObjectsPerDirectory")
	                              .map(Integer::parseInt)
	                              .orElse(20))
	           : _maxObjectsPerDirectory;
	}
	
	public int getMaxVolumes() throws ParseException
	{
	    return _maxVolumes == null
	           ? (_maxVolumes = _baseConfig.getCommandLineOptionValue("maxVolumes")
	                                       .map(Integer::parseInt)
	                                       .orElse(5))
               : _maxVolumes;
	}
	
	public String getPathSeparator() throws ParseException
	{
	    return _pathSeparator == null
	           ? (_pathSeparator = _baseConfig.getCommandLineOptionValue("pathSeparator")
	                                          .orElse("/"))
	           : _pathSeparator;
	}
	
    @WorkloadProvider
    public RandomFill getRandomFillWorkload() throws ParseException
    {
        return new RandomFill(getMaxVolumes(),
                              getPathSeparator(),
                              getMaxDirectoriesPerLevel(),
                              getMaxObjectSize(),
                              getMaxObjectsPerDirectory(),
                              getMaxDirectoryDepth());
    }
    
	@Override
	public boolean isHelpNeeded()
	{
		return false;
	}
	
	private Config _baseConfig;
	
	private final Set<RuntimeConfig> _configs;
    
	private Integer _maxDirectoriesPerLevel;
	
	private Integer _maxDirectoryDepth;
	
	private Integer _maxObjectSize;
	
	private Integer _maxObjectsPerDirectory;
	
    private Integer _maxVolumes;
    
    private String _pathSeparator;
}
