package com.formationds.commons;

import org.apache.commons.cli.Options;

public interface RuntimeConfig
{
	void addConfig(RuntimeConfig config);
	
	void addOptions(Options options);
	
	boolean isHelpNeeded();
}
