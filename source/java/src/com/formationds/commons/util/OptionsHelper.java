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

package com.formationds.commons.util;

import com.formationds.commons.util.i18n.CommonsUtilResource;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * @author ptinius
 */
public class OptionsHelper
{
  private static final String DUPLICATE_KEY =
    CommonsUtilResource.getString( "duplicate.key" );

  private Map<String, String> optionsMap = new HashMap<String, String>();

  /**
   * Default constructor
   */
  public OptionsHelper()
  {
    super();
  }

  /**
   * Parses options from the given list of strings.
   *
   * @param input string array containing options. It is expected
   * to be of the format:
   * input[x] == "--optName",
   * input[x + 1] == "optValue"
   */
  public void load( String[] input )
    throws Exception
  {
    List<String> options = Arrays.asList( input );

    int len = options.size();
    int i = 0;

    if( len == 0 )
    {
      return;
    }

    while( i < options.size() )
    {
      String val = "";
      String opt = options.get( i );

      if( opt.startsWith( "--" ) &&
          optionsMap.containsKey( opt.substring( 2 ) ) )
      {
        throw new IllegalArgumentException( DUPLICATE_KEY + ": " +
                                            opt.substring( 2 ) );
      }

      if( options.get( i ).startsWith( "--" ) )
      {
        if( options.size() > i + 1 )
        {
          if( !options.get( i + 1 ).startsWith( "--" ) )
          {
            val = options.get( i + 1 );
            optionsMap.put( opt.substring( 2 ), val );
          }
          else
          {
            optionsMap.put( opt.substring( 2 ), null );
          }
        }
        else
        {
          optionsMap.put( opt.substring( 2 ), null );
        }
      }

      i++;
    }
  }

  /**
   * Retrieves option value corresponding to given key.
   *
   * @param optionKey name of the option
   *
   * @return String option value
   */
  public String get( String optionKey )
  {
    return optionsMap.get( optionKey );
  }

  /**
   * Verifies if an option by the given name is present.
   *
   * @param optionName name of the options
   *
   * @return boolean
   */
  public boolean isSet( String optionName )
  {
    return optionsMap.containsKey( optionName );
  }

  /**
   * @return a string representation of the object.
   */
  @Override
  public String toString()
  {
    final StringBuilder sb = new StringBuilder();

    sb.append( "OptionsHelper" )
      .append( "[ " )
      .append( "optionsMap=" )
      .append( optionsMap )
      .append( " ]" );

    return sb.toString();
  }
}
