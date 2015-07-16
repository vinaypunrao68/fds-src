package com.formationds.iodriver.model;

/**
 * The format of data gathered to compare two systems.
 */
public enum ComparisonDataFormat
{
	// TODO: There should probably be a choice about canonicalization, character encodings and the
	//       like.
	/**
	 * Compare all data, including full object content.
	 */
	FULL,
	
	/**
	 * Compare all metadata, SHA-512 of content.
	 */
	EXTENDED,
	
	/**
	 * Compare only name, size, modification date, and MD5.
	 */
	BASIC,
	
	/**
	 * Compare only filenames.
	 */
	MINIMAL
}
