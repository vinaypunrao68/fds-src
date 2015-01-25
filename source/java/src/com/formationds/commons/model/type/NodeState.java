/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

/**
 * @author ptinius
 */
public enum NodeState {
  UP( "FDS_Node_Up" ),
  DOWN( "FDS_Node_Down" ),
  REMOVED( "FDS_Node_Rmvd" ),
  DISCOVERED( "FDS_Node_Discovered" ),
  MIGRATION( "FDS_Node_Migration" ),
  UNKNOWN( "Unknown" );

  private String fdsDefined;

  NodeState( final String fdsDefined ) {
    this.fdsDefined = fdsDefined;
  }

  /**
   * @return Returns {@link String} representing the fds defined state
   */
  public String getFdsDefined() {
    return fdsDefined;
  }

  public static NodeState byFdsState( final String fdsState ) {
    for( final NodeState state : NodeState.values() ) {
      if( state.getFdsDefined()
               .equalsIgnoreCase( fdsState ) ) {
        return state;
      }
    }

    return UNKNOWN;
  }

  public static NodeState byFdsState( final int fdsState ) {

    for( final NodeState state : NodeState.values() ) {

      if( state.ordinal() == fdsState ) {

        return state;

      }
    }

    return UNKNOWN;
  }
}
