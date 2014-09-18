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

package com.formationds.commons.model.helper;

import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;

import java.io.IOException;
import java.io.InputStream;

/**
 * @author ptinius
 */
public class ObjectModelHelper
{
  private static final ObjectMapper mapper = new ObjectMapper();

  private static ObjectMapper getMapper()
  {
    return mapper;
  }

  /**
   * @param json the {@code json} {@link String}
   * @param type the a native {@code type} representation.
   *
   * @return Returns De-serialized native instance
   *
   * @throws IOException
   * @throws JsonMappingException
   * @throws JsonParseException
   */
  public static <T> T toObject( final String json,
                                final TypeReference<T> type )
    throws IOException
  {
    try
    {
      return getMapper().readValue( json, type );
    }
    catch( Exception e )
    {
      throw new RuntimeException( e );
    }
  }

  /**
   * @param inputStream the {@link java.io.InputStream} representing the json
   * @param type the a native {@code type} representation
   *
   * @return Returns De-serialized native instance
   */
  public static <T> T toObject( final InputStream inputStream,
                                final TypeReference<T> type )
    throws IOException
  {
    try
    {
      return getMapper().readValue( inputStream, type );
    }
    catch( Exception e )
    {
      throw new RuntimeException( e );
    }
  }

  /**
   * @param object the {@link Object} to represent as JSON
   *
   * @return Returns the {@link String} representing the JSON
   */
  public static String toJSON( final Object object )
  {
    try
    {
      return getMapper().writeValueAsString( object );
    }
    catch( Exception e )
    {
      throw new RuntimeException( e );
    }
  }

  /**
   * default singleton
   */
  private ObjectModelHelper()
  {
    super();
  }
}
