/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public enum Protocol {
  S3,
  NBD,
  SWIFT;

  /**
   * @param apis the {@link String} representing the {@link com.formationds.commons.model.Volume#getApis()}
   *
   * @return Returns {@link List} of {@link com.formationds.commons.model.type.Protocol}
   */
  public static List<Protocol> lookup( final String apis )
  {
    final List<Protocol> protocols = new ArrayList<>( );
    final String[] split = apis.split( "," );
    if( split[ 0 ] != null )
    {
      for( final Protocol protocol : Protocol.values() )
      {
        if( protocol.name().contains( apis ) )
        {
          protocols.add( protocol );
        }
      }
    }

    return protocols;
  }

  public static Protocol findByValue( final int ordinal ) {
    for( final Protocol protocol : Protocol.values() )
    {
      if( protocol.ordinal() == ordinal ) {
        return protocol;
      }
    }

    return null;
  }
}
