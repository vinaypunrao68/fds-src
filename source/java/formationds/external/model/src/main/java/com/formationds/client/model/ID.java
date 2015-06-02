/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.model;

import java.util.Objects;
import java.util.UUID;

/**
 * The ID represents a globally unique identifier for an entity in the Formation system.
 * An ID is considered equivalent with another if either:
 *
 * <code>
 *      id.equals(otherId)
 * </code>
 *
 * or
 *
 * <code>
 *      id.compareTo(otherId) == 0.
 * </code>
 *
 * By "Globally Unique" we specifically mean that it is unique within an FDS Global Domain, which is to say
 * one FDS installation.  It does not require specification of a UUID/GUID (https://www.ietf.org/rfc/rfc4122.txt)
 * but it does not prevent use of a UUID/GUID either.
 * <p/>
 * Each ID implementation is required to have a constructor that takes a String in the format
 * produced by the #toString method and is capable of parsing that string to generate and ID
 * object.  A second constructor taking the specified generic type T is also expected to be
 * present.
 */
// At this time the only constraint I am putting on the contents of the UniqueId is that
// it must have a string representation that is unique across and FDS Global Domain.  We may at some
// point decide to put additional constraints on the id interface, for example, we may want to define
// it as a "composite key" such as local-domain.tenantId.volume-name
public interface ID<T,U extends ID<T,U>> extends Comparable<U> {

    /**
     * Implementation of an ID as a single long value consistent with current implementation of
     * FDS thrift SvcUUID and similar in the C++ code.
     */
    class FdsUID implements ID<Long, FdsUID> {

        public static FdsUID id(Long id) { return new FdsUID( id ); }
        private final Long id;

        /**
         * @param id
         */
        public FdsUID( Long id ) {
            if (id == null)
                throw new NullPointerException( "ID must not be null" );
            this.id = id;
        }

        /**
         * Create an FdsUUID from the specified string.  This supports a string
         * in either decimaal or hex format.  Leading/trailing whitespace is automatically trimmed
         * and it handles a uuid prepended with '0x' or containing '-' chars.
         * @param id the uuid string.
         * @throws IllegalArgumentException if the id is null or not a valid decimal or hex representation of a UUID.
         */
        public FdsUID( String id ) throws IllegalArgumentException {
            long x;
            String s = id.trim().replaceAll( "-", "" );
            try {
                // try hex first
                if (id.startsWith( "0x" )) {
                    id = id.substring( 2 );
                    x = Long.valueOf( id, 16 );
                }
                else {
                    try {
                        // not hex; try decimal.
                        x = Long.valueOf( id );
                    } catch (NumberFormatException nfe2) {
                        throw new IllegalArgumentException( "id " + id + " must be a valid hex or decimal Long value", nfe2 );
                    }
                }
            } catch (NumberFormatException nfe) {
                throw new IllegalArgumentException( "id " + id + " must be a valid hex or decimal Long value", nfe );
            }
            this.id = x;
        }
        public Long getId() { return this.id; }

        @Override
        public String toString() {
            String h = Long.toHexString( id ).toUpperCase();
            if ( h.length() % 2 != 0 ) {
                h = '0' + h;
            }
            return "0x" + h;
        }

        @Override
        public int compareTo( FdsUID o ) {
            return this.id.compareTo( o.id );
        }

        @Override
        public boolean equals( Object o ) {
            if ( this == o ) { return true; }
            if ( !(o instanceof FdsUID) ) { return false; }
            final FdsUID fdsUID = (FdsUID) o;
            return Objects.equals( id, fdsUID.id );
        }

        @Override
        public int hashCode() {
            return Objects.hash( id );
        }
    }

    /**
     * ID implementation based on a java.util.UUID supporting RFC 4122 style Universally-Unique Identifiers.
     */
    class RFC4122Uuid implements ID<UUID, RFC4122Uuid> {

        /**
         *
         * @param uuid the uuid
         * @return a new RFC4122Uuid based on the specified uuid
         */
        public static RFC4122Uuid id(UUID uuid) { return new RFC4122Uuid(uuid); }

        private final UUID uuid;

        /**
         * @param uuid the uuid
         */
        public RFC4122Uuid(UUID uuid) {
            if (uuid == null) throw new NullPointerException( "ID must not be null" );
            this.uuid = uuid;
        }

        /**
         * Leading/trailing whitespace is automatically trimmed
         * and it handles a uuid prepended with '0x' or containing '-' chars.
         *
         * @param uuid the uuid
         *
         * @throws NullPointerException if uuid string is null
         * @throws IllegalArgumentException if uuid string can not be converted into a valid UUID
         */
        public RFC4122Uuid(String uuid) {
            if (uuid == null) throw new NullPointerException( "UUID string must not be null." );
            long msb = 0;
            long lsb = 0;
            byte[] data = hexToUUIDBytes( uuid );
            for (int i=0; i<8; i++) {
                msb = (msb << 8) | (data[i] & 0xff);
            }
            for (int i=8; i<16; i++) {
                lsb = (lsb << 8) | (data[i] & 0xff);
            }

            this.uuid = new UUID( msb, lsb );
        }

        public static byte[] hexToUUIDBytes( String uuid ) {
            String s = uuid.trim().replaceAll( "-", "" ).replaceFirst( "0x" , "" );
            if (s.length() != 32)
                throw new IllegalArgumentException( "uuid string " + uuid + " is not a valid RFC 4122 UUID" );
            byte[] data = new byte[16];
            for (int i = 0; i < 16; i += 2) {
                data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
                                      + Character.digit(s.charAt(i+1), 16));
            }
            return data;
        }

        public UUID getId() { return uuid; }

        public String toString() {
            return getId().toString();
        }

        @Override
        public int compareTo( RFC4122Uuid o ) {
            return uuid.compareTo( o.uuid );
        }

        @Override
        public boolean equals( Object o ) {
            if ( this == o ) { return true; }
            if ( !(o instanceof RFC4122Uuid) ) { return false; }
            final RFC4122Uuid that = (RFC4122Uuid) o;
            return Objects.equals( uuid, that.uuid );
        }

        @Override
        public int hashCode() {
            return Objects.hash( uuid );
        }
    }

    /**
     *
     * @return the id
     */
    public T getId();
}

