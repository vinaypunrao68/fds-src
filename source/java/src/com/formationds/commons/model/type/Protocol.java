/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 *
 * This software is furnished under a license and may be used and copied only
 * in  accordance  with  the  terms  of such  license and with the inclusion
 * of the above copyright notice. This software or  any  other copies thereof
 * may not be provided or otherwise made available to any other person.
 * No title to and ownership of  the  software  is  hereby transferred.
 *
 * The information in this software is subject to change without  notice
 * and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 * Formation Data Systems assumes no responsibility for the use or  reliability
 * of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.commons.model.type;

import java.util.ArrayList;
import java.util.List;

/**
 * @author ptinius
 */
public enum Protocol {
  S3,
  SWIFT,
  NBD;

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
}
