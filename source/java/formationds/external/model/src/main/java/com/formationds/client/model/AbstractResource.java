/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.model;

import java.util.Arrays;
import java.util.Comparator;
import java.util.Iterator;
import java.util.Objects;

/**
 * Base class for external model resources.  Each resource has a system-generated ID as well as
 * a natural key / name.
 * <p/>
 * Equals method implementation is based on the ID equality.  However, the Comparable implementation is
 * based on name comparison.  That is, they are inconsistent and thus an AbstractResource is not a good
 * candidate for use as a key in Collections.
 * @param <T>
 * @param <I>
 */
public abstract class AbstractResource<T, I extends ID<T,I>> implements Comparable<AbstractResource> {

    /**
     * @return a resource name-based comparator
     */
    public static Comparator<AbstractResource> nameCompare() { return new Comparator<AbstractResource>() {
        @Override
        public int compare( AbstractResource o1, AbstractResource o2 ) {
            return o1.name.compareTo( o2.name );
        }
    };}

    /**
     *
     * @return a resource id-based comparator
     */
    public static Comparator<AbstractResource> idCompare() { return new Comparator<AbstractResource>() {
        @Override
        public int compare( AbstractResource o1, AbstractResource o2 ) {
            return o1.uid.compareTo( o2.uid );
        }
    };}

    private I      uid;
    private final String name;

    /**
     * Create a resource with the specified unique identifier and name
     * @param uid the unique identifier
     * @param name the resource name
     */
    public AbstractResource( I uid, String name ) {
        this.uid = uid;
        this.name = name;
    }

    /**
     * Create a resource with the specified unique identifier and a name constructed from the
     * set of name components, delimited by a '.' character.
     *
     * @param uid the unique identifier
     * @param names the components that make a unique resource name/key
     */
    public AbstractResource( I uid, String... names ) {
        this( uid, '.', names );
    }

    /**
     *
     * @param uid the unique identifier
     * @param delim a delimiter for the name component
     * @param names the components that make a unique resource name/key
     */
    public AbstractResource( I uid, char delim, String... names ) {
        this.uid = uid;

        if ( names.length == 1 ) {
            this.name = names[0];
        } else {
            StringBuilder sb = new StringBuilder();
            Iterator<String> niter = Arrays.asList( names ).iterator();
            while ( niter.hasNext() ) {
                sb.append( niter.next() );
                if ( niter.hasNext() )
                    sb.append( delim );
            }
            this.name = sb.toString();
        }
    }

    void setId(I id) {
        if (this.uid != null) {
            throw new IllegalStateException( "ID is already set." );
        }
        this.uid = id;
    }

    /**
     *
     * @return the unique identifier for this resource
     */
    public T getId() { return uid.getId(); }

    /**
     *
     * @return the unique resource name
     */
    public String getName() { return name; }

    @Override
    public int compareTo( AbstractResource o ) {
        return name.compareTo( o.getName() );
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof AbstractResource) ) { return false; }
        final AbstractResource<?, ?> that = (AbstractResource<?, ?>) o;
        return Objects.equals( uid, that.uid ) &&
               Objects.equals( name, that.name );
    }

    @Override
    public int hashCode() {
        return Objects.hash( uid, name );
    }
}
