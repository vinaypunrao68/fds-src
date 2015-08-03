package com.formationds.commons.libconfig;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.antlr.runtime.ANTLRInputStream;
import org.antlr.runtime.ANTLRStringStream;
import org.antlr.runtime.CharStream;
import org.antlr.runtime.CommonTokenStream;
import org.antlr.runtime.RecognitionException;
import org.antlr.runtime.tree.CommonTree;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Paths;
import java.text.ParseException;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;

public class ParsedConfig {

    private static final String DEF_KEYMANAGER_PASSWD =
        "etc/ssl/dev/keystore";

    private static final Logger logger =
        LoggerFactory.getLogger( ParsedConfig.class );

    private Map<String, Node> map;

    public ParsedConfig( final String input )
        throws ParseException {

        this( new ANTLRStringStream( input ) );

    }

    public ParsedConfig( final InputStream inputStream )
        throws IOException, ParseException {

        this( new ANTLRInputStream( inputStream ) );

    }

    public ParsedConfig( final CharStream charStream )
        throws ParseException {

        LibConfigParser parser =
            new LibConfigParser(
                new CommonTokenStream(
                    new LibConfigLexer( charStream ) ) );

        LibConfigParser.namespace_return v;
        try {

            v = parser.namespace( );

        } catch( RecognitionException e ) {

            throw new ParseException(
                String.format( "Parse error on '%s'", e.token ), e.index );

        }

        map = new HashMap<>( );
        Node node = doParse( ( CommonTree ) v.getTree( ) );
        map.put( node.getName( ), node );

    }

    private Node doParse( final CommonTree commonTree ) {

        switch( commonTree.getType( ) ) {

            case LibConfigParser.ID:
                Namespace namespace = new Namespace( commonTree.getText( ) );
                if( commonTree.getChildren( ) != null ) {

                    commonTree.getChildren( )
                              .stream( )
                              .map( ( t ) -> ( CommonTree ) t )
                              .forEach( ( c ) -> namespace.addChild(
                                  doParse( c ) ) );
                }

                return namespace;

            case LibConfigParser.EQUALS:
                if( commonTree.getChildren() != null ) {

                    CommonTree[] children = commonTree.getChildren( )
                                                      .stream( )
                                                      .map(
                                                          ( t ) -> ( CommonTree ) t )
                                                      .toArray(
                                                          CommonTree[]::new );
                    return new Assignment(
                        children[ 0 ].getText(),
                        parseValue( children[ 1 ] ) );
                }

                return new Assignment( commonTree.getText(),
                                       Optional.empty() );

            default:
                final String s = "Unrecognized token type, '" +
                    commonTree.getType() + "'";
                logger.warn( s );
                throw new RuntimeException( s );
        }
    }

    private Optional parseValue( final CommonTree child ) {

        switch (child.getType()) {

            case LibConfigParser.INT:
                return Optional.of( Integer.valueOf( child.getText() ) );

            case LibConfigParser.FLOAT:
                return Optional.of( Float.valueOf( child.getText() ) );

            case LibConfigParser.STRING:
                return Optional.of( child.getText()
                                         .replaceAll( "\"", "" ) );

            case LibConfigParser.BOOLEAN:
                return Optional.of( Boolean.valueOf( child.getText() ) );

            default:
                logger.warn(
                    "The specified type of '{}' is not supported.",
                    child.getType() );

                return Optional.empty();
        }
    }

    public Assignment lookup( final String path ) {

        String[] parts = path.split( "\\." );
        Map<String, Node> current = map;

        for( int i = 0; i < parts.length; i++ ) {

            String part = parts[ i ];
            if( !current.containsKey( part ) ) {

                return new Assignment( path, Optional.empty() );

            }

            Node node = current.get( part );
            if( i == parts.length - 1 ) {

                if( ( node instanceof Assignment ) ) {

                    return ( Assignment ) node;

                }

                return new Assignment( part, Optional.empty() );

            } else {

                if( !( node instanceof Namespace ) ) {

                    return new Assignment( part, Optional.empty() );

                }
                current = ( ( Namespace ) node ).children();

            }
        }

        return new Assignment( path, Optional.empty() );
    }

    public int defaultInt( final String key, final int defaultValue ) {

        final Assignment assignment = lookup( key );
        if( assignment.getValue()
                      .isPresent() ) {

            return ( Integer ) assignment.getValue()
                                         .get();

        }

        return defaultValue;
    }

    public boolean defaultBoolean( final String key,
                                   final boolean defaultValue ) {

        final Assignment assignment = lookup( key );
        if( assignment.getValue()
                      .isPresent() ) {

            return ( Boolean ) assignment.getValue()
                                         .get();

        }

        logger.warn(
            "The specified path '{}' was not found, using default of '{}.",
            key, defaultValue );
        return defaultValue;

    }

    public String defaultString( final String key, final String defaultValue ) {

        final Assignment assignment = lookup( key );
        if( assignment.getValue()
                      .isPresent() ) {

            return ( String ) assignment.getValue()
                                        .get();

        }

        logger.warn(
            "The specified path '{}' was not found, using default of '{}.",
            key, defaultValue );
        return defaultValue;

    }

    public File getPath( final String key, final String relativeTo ) {

        final Assignment assignment = lookup( key );
        if( assignment.getValue()
                      .isPresent() ) {

            return Paths.get( relativeTo,
                              ( String ) assignment.getValue()
                                                   .get() )
                        .toFile();
        }

        final File defaultFile = Paths.get( relativeTo, DEF_KEYMANAGER_PASSWD )
                                      .toFile();
        logger.warn(
            "The specified path '{}' was not found, using default of '{}.",
            key, defaultFile );

        return defaultFile;

    }
}
