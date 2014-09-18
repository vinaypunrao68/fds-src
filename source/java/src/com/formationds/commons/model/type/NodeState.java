/*
 * Copyright (C) 2014, All Rights Reserved, by Formation Data Systems, Inc.
 *
 *  This software is furnished under a license and may be used and copied only
 *  in  accordance  with  the  terms  of such  license and with the inclusion
 *  of the above copyright notice. This software or  any  other copies thereof
 *  may not be provided or otherwise made available to any other person.
 *  No title to and ownership of  the  software  is  hereby transferred.
 *
 *  The information in this software is subject to change without  notice
 *  and  should  not be  construed  as  a commitment by Formation Data Systems.
 *
 *  Formation Data Systems assumes no responsibility for the use or  reliability
 *  of its software on equipment which is not supplied by Formation Date Systems.
 */

package com.formationds.commons.model.type;

/**
 * @author ptinius
 */
public enum NodeState
{
  UP( "FDS_Node_Up" ),
  DOWN( "FDS_Node_Down" ),
  REMOVED( "FDS_Node_Rmvd" ),
  DISCOVERED( "FDS_Node_Discovered" ),
  MIGRATION( "FDS_Node_Migration" ),
  UNKNOWN( "Unknown" );

  private String fdsDefined;

  NodeState( final String fdsDefined )
  {
    this.fdsDefined = fdsDefined;
  }

  /**
   * @return Returns {@link String} representing the fds defined state
   */
  public String getFdsDefined()
  {
    return fdsDefined;
  }

  /**
   * @param fdsState the {@link String} representing the fds defined state
   *
   * @return
   */
  public static NodeState byFdsState( final String fdsState )
  {
    for( final NodeState state : NodeState.values() )
    {
      if( state.getFdsDefined( ).equalsIgnoreCase( fdsState ) )
      {
        return state;
      }
    }

    return UNKNOWN;
  }
}
