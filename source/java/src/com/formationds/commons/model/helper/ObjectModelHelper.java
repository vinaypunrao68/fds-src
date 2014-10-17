/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.helper;

import com.formationds.commons.model.type.Protocol;
import com.google.gson.FieldNamingPolicy;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import org.joda.time.format.ISODateTimeFormat;

import java.io.Reader;
import java.lang.reflect.Type;
import java.text.ParseException;
import java.util.Date;
import java.util.List;

/**
 * @author ptinius
 */
public class ObjectModelHelper {
  /**
   * @param protocols the {@link List} of {@link com.formationds.commons.model.type.Protocol}
   *
   * @return Returns {@link String} representing a comma separated list of
   * {@link com.formationds.commons.model.type.Protocol}
   */
  public static String toProtocalString( final List<Protocol> protocols ) {
    final StringBuilder sb = new StringBuilder();
    for( final Protocol protocol : protocols ) {
      if( sb.length() != 0 ) {
        sb.append( "," );
      }

      sb.append( protocol.name() );
    }

    return sb.toString();
  }

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
   * @return Returns the {@link Type} represented within the JSON
   */
  public static <T> T toObject( final String json, final Type type ) {
    return new GsonBuilder().create()
                            .fromJson( json, type );
  }

  /**
   * @param reader the {@link java.io.Reader}
   * @param type   the {@link Type} to parse the JSON into
   *
   * @return Returns the {@link Type} represented within the JSON
   */
  public static <T> T toObject( final Reader reader, final Type type ) {
    return new GsonBuilder().create()
                            .fromJson( reader, type );
  }

  /**
   * @param object the {@link Object} representing the JSON
   *
   * @return Returns the {@link String} representing the JSON
   */
  public static String toJSON( final Object object ) {
    Gson gson =
      new GsonBuilder().setFieldNamingPolicy( FieldNamingPolicy.IDENTITY )
                       .setPrettyPrinting()
                       .create();
    return gson.toJson( object );
  }

  /**
   * default singleton
   */
  private ObjectModelHelper() {
    super();
  }
}
