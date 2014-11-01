/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.helper;

import com.formationds.commons.model.type.Protocol;
import com.google.gson.FieldNamingPolicy;
import com.google.gson.GsonBuilder;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.Reader;
import java.lang.reflect.Type;
import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;

/**
 * @author ptinius
 */
public class ObjectModelHelper {

  private static final Logger logger =
      LoggerFactory.getLogger( ObjectModelHelper.class );

  private static final ModelObjectExclusionStrategy exclusion =
      new ModelObjectExclusionStrategy();

  private static final DateFormat ISO_DATE =
    new SimpleDateFormat( "yyyyMMdd'T'HHmmss" );

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
    return fromISODateTime( date );
  }

  /**
   * @param date the {@link Date} representing the date
   *
   * @return Returns {@link String} representing the ISO Date format of
   * "yyyyMMddTHHmmssZ"
   */
  public static String isoDateTimeUTC( final Date date ) {
    return ISO_DATE.format( date );
  }

  /**
   * @param date the {@link String} representing the ISO Date format of
   *             "yyyyMMddThhmmss"
   *
   * @return Date Returns {@link Date} representing the {@code date}
   */
  public static Date fromISODateTime( final String date )
    throws ParseException {
    return ISO_DATE.parse( date );
  }

  /**
   * @param fullQualifiedFieldName the {@link String} representing the fully
   *                               qualified field name
   */
  public static void excludeField( final String fullQualifiedFieldName ) {
      try {
          exclusion.excludeField( fullQualifiedFieldName );
      } catch( ClassNotFoundException e ) {
          logger.warn( "could not find class or field {}, skipping!",
                       fullQualifiedFieldName );
      }
  }

  /**
   * @param clazz the {@link Class} representing the class to exclude
   */
  public static void excludeClass( final Class<?> clazz ) {
      exclusion.excludeClass( clazz );
  }
  /**
   * @param json the {@link String} representing the JSON object
   * @param type the {@link Type} to parse the JSON into
   *
   * @return Returns the {@link Type} represented within the JSON
   */
  public static <T> T toObject( final String json, final Type type ) {
    return new GsonBuilder().setExclusionStrategies( exclusion )
                            .create()
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
   return new GsonBuilder().setFieldNamingPolicy( FieldNamingPolicy.IDENTITY )
                           .setPrettyPrinting()
                           .create()
                           .toJson( object );
  }

  /**
   * default singleton
   */
  private ObjectModelHelper() {
    super();
  }
}
