package com.formationds.client.v08.model.stats;

/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

/**
 * @author nbever
 */
public enum StatTypes {
	
	/**
	 * Number of PUT operations over an interval
	 */
    PUTS( "Puts" ),
    
    /**
     * Number of GET operations over an interval
     */
    GETS( "Gets" ),
    
    /**
     * How many times the queue was seen as full.
     */
    QFULL( "Queue Full" ),
     
    /**
     * 
     */
    MBYTES( "Metadata Bytes" ),
    
    /**
     * Number of blobs in the system
     */
    BLOBS( "Blobs" ),
    
    /**
     * Number of objects in the system
     */
    OBJECTS( "Objects" ),
    
    /**
     * Average size of the blobs in the system
     */
    ABS( "Ave Blob Size" ),
    
    /**
     * Average number of blobs per object in the system
     */
    AOPB( "Ave Objects per Blob" ),
    
    /**
     * The amount of storage used by the system before any de-duplication
     */
    LBYTES( "Logical Bytes" ),
    
    /**
     * The amount of physical storage used by the system
     */
    PBYTES( "Physical Bytes" ),
    
    /**
     * 
     */
    STC_SIGMA( "Short Term Capacity Sigma" ),
    
    /**
     * 
     */
    LTC_SIGMA( "Long Term Capacity Sigma" ),
    
    /**
     * 
     */
    STP_SIGMA( "Short Term Perf Sigma" ),
    
    /**
     * 
     */
    LTP_SIGMA( "Long Term Perf Sigma" ),
    
    /**
     * 
     */
    STC_WMA( "Short Term Capacity WMA" ),
    
    /**
     * 
     */
    LTC_WMA( "Long Term Capacity WMA" ),
    
    /**
     * 
     */
    STP_WMA( "Short Term Perf WMA" ),
    
    /**
     * 
     */
    LTP_WMA( "Long Term Perf WMA" ),
   
    /**
     * gets from SSD and cache
     */
    SSD_GETS( "SSD Gets" );


    private final String key;

    /**
     * @param key the {@link String} representing the metadata key
     */
    StatTypes( String key ) {
        this.key = key;
    }

    /**
     * @return Returns{@link String} representing the metadata key
     */
    public String key() {
        return this.key;
    }
    
    /**
     * Takes a string and checks it against both the name and the metadata key
     *
     * @param aValue the value to check
     *
     * @return true if the value matches either the name or the key, ignoring case
     */
    public boolean matches( String aValue ) {
    	
    	return ( aValue.equalsIgnoreCase( this.key() ) || aValue.equalsIgnoreCase( this.name() ) );
    }

    /**
     * Lookup the metric by the name or the metadata key name.  This does a case-insensitive comparison
     * against the metadata key and the metric enum name
     *
     * @param metadataKey the case sensitive {@link String} representing the
     *                    metric name OR the metadata key name
     *
     * @return Returns {@link StatTypes}
     * associated with {@code metadataKey}
     *
     */
    public static StatTypes lookup( final String metadataKey ) {
        String tmdk = metadataKey.trim();
        for( final StatTypes m : values() ) {
            if( m.matches( tmdk ) ) {
                return m;
            }
        }

        return null;
    }
}
