/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

import java.util.Optional;

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

  public static Optional<NodeState> byFdsDefined( final String fdspNodeStateName ) {

    for( final NodeState state : values() ) {

      if( state.getFdsDefined().equalsIgnoreCase( fdspNodeStateName ) ) {

        return Optional.of( state );

      }

    }

    return Optional.empty();
  }

}
