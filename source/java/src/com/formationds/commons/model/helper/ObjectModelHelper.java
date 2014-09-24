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

import com.google.gson.FieldNamingPolicy;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import org.joda.time.format.ISODateTimeFormat;

import java.lang.reflect.Type;
import java.text.ParseException;
import java.util.Date;

/**
 * @author ptinius
 */
public class ObjectModelHelper
{
  /**
   * @param date the {@link String} representing the RDATE spec format
   *
   * @return Returns {@link Date}
   *
   * @throws ParseException if there is a parse error
   */
  public static Date toiCalFormat( final String date )
    throws ParseException {
    return ISODateTimeFormat.dateOptionalTimeParser()
                            .parseDateTime( date )
                            .toDate();
  }

  /**
   * @param json the {@link String} representing the JSON object
   * @param type the {@link Type} to parse the JSON into
   *
   * @return Returns the
   */
  public static <T> T toObject( final String json, final Type type )
  {
    return new GsonBuilder().create().fromJson( json, type );
  }

  /**
   * @param object the {@link Object} representing the JSON
   *
   * @return Returns the {@link String} representing the JSON
   */
  public static String toJSON( final Object object )
  {
    Gson gson =
      new GsonBuilder().setFieldNamingPolicy(
        FieldNamingPolicy.IDENTITY)
                       .setPrettyPrinting()
                       .create();
    return gson.toJson( object );
  }

  /**
   * default singleton
   */
  private ObjectModelHelper()
  {
    super();
  }
}
