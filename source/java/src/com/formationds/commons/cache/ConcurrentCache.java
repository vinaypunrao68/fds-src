/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.cache;

import com.formationds.commons.cache.abs.AbstractMapEntry;
import com.formationds.commons.cache.i18n.CacheResource;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

/**
 * @author ptinius
 */
@SuppressWarnings("UnusedDeclaration")
public class ConcurrentCache<K, V>
  implements Map<K, V> {
  private static final Logger logger =
    LoggerFactory.getLogger( ConcurrentCache.class );

  protected ConcurrentMap<K, CacheObject<V>> map;
  protected LinkedList lastAccessedList;
  protected LinkedList ageList;
  protected int maxCacheSize;
  protected long maxLifetime;
  protected long cacheHits = 0L;
  protected long cacheMisses = 0L;

  /**
   * bottomless cache, items never expire.
   */
  public ConcurrentCache() {
    this( -1, -1 );
  }

  /**
   * bottomless cache, but age out old entries
   *
   * @param maxLifetime the maximum amount of time (in ms) objects can exist in
   *                    cache before being deleted.
   */
  public ConcurrentCache( final long maxLifetime ) {
    this( -1, maxLifetime );
  }

  /**
   * @param maxSize     the maximum number of objects the cache will hold. -1
   *                    means the cache has no max size.
   * @param maxLifetime the maximum amount of time (in ms) objects can exist in
   *                    cache before being deleted. -1 means objects never
   *                    expire.
   */
  public ConcurrentCache( final int maxSize, final long maxLifetime ) {
    if( maxSize == 0 ) {
      throw new IllegalArgumentException(
        CacheResource.getString( "invalid.max.cache" ) );
    }
    this.maxCacheSize = maxSize;
    this.maxLifetime = maxLifetime;

    // Our primary data structure is a hash map. The default capacity of 11
    // is too small in almost all cases, so we set it bigger.
    map = new ConcurrentHashMap<>( 103 );

    lastAccessedList = new LinkedList();
    ageList = new LinkedList();
  }

  /**
   * @param key   the key
   * @param value the value
   *
   * @return Returns the value
   */
  @Override
  public V put( K key, V value ) {
    V oldValue = null;
    // Delete an old entry if it exists.
    if( map.containsKey( key ) ) {
      oldValue = remove( key, true );
    }

    CacheObject<V> cacheObject = new CacheObject<>( value );
    map.put( key, cacheObject );
    // Make an entry into the cache order list.
    // Store the cache order list entry so that we can get back to it
    // during later lookups.
    cacheObject.lastAccessedListNode = lastAccessedList.addFirst( key );
    // Add the object to the age list
    LinkedListNode ageNode = ageList.addFirst( key );
    ageNode.timestamp = System.currentTimeMillis();
    cacheObject.ageListNode = ageNode;

    // If cache is too full, remove least used cache entries until it is not too full.
    cullCache();

    return oldValue;
  }

  /**
   * @param key the key
   *
   * @return Returns the value associated with the key
   */
  @Override
  public V get( Object key ) {
    // First, clear all entries that have been in cache longer than the
    // maximum defined age.
    deleteExpiredEntries();

    CacheObject<V> cacheObject = map.get( key );
    if( cacheObject == null ) {
      // The object didn't exist in cache, so increment cache misses.
      cacheMisses++;
      return null;
    }
    // Remove the object from it's current place in the cache order list,
    // and re-insert it at the front of the list.
    cacheObject.lastAccessedListNode.remove();
    lastAccessedList.addFirst( cacheObject.lastAccessedListNode );

    // The object exists in cache, so increment cache hits. Also, increment
    // the object's read count.
    cacheHits++;
    cacheObject.readCount++;

    return cacheObject.object;
  }

  /**
   * @param key key whose mapping is to be removed from the map
   *
   * @return Returns the previous value associated with <tt>key</tt>, or
   * <tt>null</tt> if there was no mapping for <tt>key</tt>.
   */
  @Override
  public V remove( Object key ) {
    return remove( key, false );
  }

  /**
   * @param key      key whose mapping is to be removed from the map
   * @param internal the {@code boolean} flag representing internal
   *
   * @return Returns the previous value associated with <tt>key</tt>, or
   * <tt>null</tt> if there was no mapping for <tt>key</tt>.
   */
  @SuppressWarnings("UnusedParameters")
  public V remove( Object key, boolean internal ) {
    //noinspection SuspiciousMethodCalls
    CacheObject<V> cacheObject = map.remove( key );
    // If the object is not in cache, stop trying to remove it.
    if( cacheObject == null ) {
      return null;
    }
    // Remove from the cache order list
    cacheObject.lastAccessedListNode.remove();
    cacheObject.ageListNode.remove();
    // Remove references to linked list nodes
    cacheObject.ageListNode = null;
    cacheObject.lastAccessedListNode = null;

    return cacheObject.object;
  }

  /**
   * clear the cache
   */
  @Override
  public void clear() {
    Object[] keys = map.keySet()
                       .toArray();
    for( Object key : keys ) {
      remove( key );
    }

    // Now, reset all containers.
    map.clear();
    lastAccessedList.clear();
    ageList.clear();

    cacheHits = 0;
    cacheMisses = 0;
  }

  /**
   * @return Returns the number of key-value mappings in this map
   */
  @Override
  public int size() {
    // First, clear all entries that have been in cache longer than the
    // maximum defined age.
    deleteExpiredEntries();

    return map.size();
  }

  /**
   * @return Returns {@code true} if and only if this map contains no key-value
   * mappings
   */
  @Override
  public boolean isEmpty() {
    // First, clear all entries that have been in cache longer than the
    // maximum defined age.
    deleteExpiredEntries();

    return map.isEmpty();
  }

  /**
   * @return Returns a collection view of the values contained in this map
   */
  @SuppressWarnings("NullableProblems")
  @Override
  public Collection<V> values() {
    // First, clear all entries that have been in cache longer than the
    // maximum defined age.
    deleteExpiredEntries();

    return Collections.unmodifiableCollection( new AbstractCollection<V>() {
      Collection<CacheObject<V>> values = map.values();

      public Iterator<V> iterator() {
        return new Iterator<V>() {
          Iterator<CacheObject<V>> it = values.iterator();

          /**
           * @return Returns {@code true} if the iteration has more elements
           */
          @Override
          public boolean hasNext() {
            return it.hasNext();
          }

          /**
           * @return Returns the next element in the iteration
           * @throws NoSuchElementException if the iteration has no more elements
           */
          @Override
          public V next() {
            return it.next().object;
          }

          /**
           * @throws UnsupportedOperationException if the {@code remove}
           *         operation is not supported by this iterator
           *
           * @throws IllegalStateException if the {@code next} method has not
           *         yet been called, or the {@code remove} method has already
           *         been called after the last call to the {@code next}
           *         method
           */
          @Override
          public void remove() {
            it.remove();
          }
        };
      }

      /**
       * @return Returns the size
       */
      @Override
      public int size() {
        return values.size();
      }
    } );
  }

  /**
   * @param key key whose presence in this map is to be tested
   *
   * @return Returns {@code true} if and only if this map contains a mapping
   * for the specified key
   */
  @Override
  public boolean containsKey( final Object key ) {
    // First, clear all entries that have been in cache longer than the
    // maximum defined age.
    deleteExpiredEntries();

    return map.containsKey( key );
  }

  /**
   * @param map mappings to be stored in this map
   */
  @SuppressWarnings("NullableProblems")
  @Override
  public void putAll( final Map<? extends K, ? extends V> map ) {
    for( Entry<? extends K, ? extends V> entry : map.entrySet() ) {
      V value = entry.getValue();
      // If the map is another DefaultCache instance than the
      // entry values will be CacheObject instances that need
      // to be converted to the normal object form.
      if( value instanceof CacheObject ) {
        //noinspection unchecked
        value = ( ( CacheObject<V> ) value ).object;
      }
      put( entry.getKey(), value );
    }
  }

  /**
   * @param value value whose presence in this map is to be tested
   *
   * @return Returns {@code true} if and only if this map maps one or more keys
   * to the specified value
   */
  @Override
  public boolean containsValue( final Object value ) {
    // First, clear all entries that have been in cache longer than the
    // maximum defined age.
    deleteExpiredEntries();

    //noinspection unchecked
    CacheObject<V> cacheObject = new CacheObject<>( ( V ) value );

    return map.containsValue( cacheObject );
  }

  /**
   * @return Returns the entry set
   */
  @SuppressWarnings("NullableProblems")
  @Override
  public Set<Entry<K, V>> entrySet() {
    // Warning -- this method returns CacheObject instances and not Objects
    // in the same form they were put into cache.

    // First, clear all entries that have been in cache longer than the
    // maximum defined age.
    deleteExpiredEntries();

    return new AbstractSet<Entry<K, V>>() {
      private final Set<Entry<K, CacheObject<V>>> set = map.entrySet();

      public Iterator<Entry<K, V>> iterator() {
        return new Iterator<Entry<K, V>>() {
          private final Iterator<Entry<K, CacheObject<V>>> it = set.iterator();

          public boolean hasNext() {
            return it.hasNext();
          }

          public Entry<K, V> next() {
            Entry<K, CacheObject<V>> entry = it.next();
            return new AbstractMapEntry<K, V>( entry.getKey(),
                                               entry.getValue().object ) {
              @Override
              public V setValue( V value ) {
                throw new UnsupportedOperationException(
                  CacheResource.getString( "cannot.set" ) );
              }
            };
          }

          public void remove() {
            it.remove();
          }
        };

      }

      public int size() {
        return set.size();
      }
    };
  }

  /**
   * @return Returns the key set
   */
  @SuppressWarnings("NullableProblems")
  @Override
  public Set<K> keySet() {
    // First, clear all entries that have been in cache longer than the
    // maximum defined age.
    deleteExpiredEntries();

    return Collections.unmodifiableSet( map.keySet() );
  }

  /**
   * @return Returns the cache hit count
   */
  @SuppressWarnings("UnusedDeclaration")
  public long getCacheHits() {
    return cacheHits;
  }

  /**
   * @return Returns the cache misses
   */
  @SuppressWarnings("UnusedDeclaration")
  public long getCacheMisses() {
    return cacheMisses;
  }

  /**
   * @return Returns teh cache maximum size
   */
  @SuppressWarnings("UnusedDeclaration")
  public int getMaxCacheSize() {
    return maxCacheSize;
  }

  /**
   * @param maxCacheSize the cache maximum cache size
   */
  @SuppressWarnings("UnusedDeclaration")
  public void setMaxCacheSize( final int maxCacheSize ) {
    this.maxCacheSize = maxCacheSize;
    // It's possible that the new max size is smaller than our current cache
    // size. If so, we need to delete infrequently used items.
    cullCache();
  }

  /**
   * @return Returns the maximum life time
   */
  @SuppressWarnings("UnusedDeclaration")
  public long getMaxLifetime() {
    return maxLifetime;
  }

  /**
   * @param maxLifetime the maximum life time
   */
  @SuppressWarnings("UnusedDeclaration")
  public void setMaxLifetime( final long maxLifetime ) {
    this.maxLifetime = maxLifetime;
  }

  /**
   * Clears all entries out of cache where the entries are older than the
   * maximum defined age.
   */
  protected void deleteExpiredEntries() {
    // Check if expiration is turned on.
    if( maxLifetime <= 0 ) {
      return;
    }

    // Remove all old entries. To do this, we remove objects from the end
    // of the linked list until they are no longer too old. We get to avoid
    // any hash lookups or looking at any more objects than is strictly
    // necessary.
    LinkedListNode node = ageList.getLast();
    // If there are no entries in the age list, return.
    if( node == null ) {
      return;
    }

    // Determine the expireTime, which is the moment in time that elements
    // should expire from cache. Then, we can do an easy check to see
    // if the expire time is greater than the expire time.
    long expireTime = System.currentTimeMillis() - maxLifetime;

    while( expireTime > node.timestamp ) {
      if( remove( node.object, true ) == null ) {
        logger.error(
          String.format( CacheResource.getString( "error.attempting.remove",
                                                  node.object.toString() ) ) );
        // remove from the ageList
        node.remove();
      }

      // Get the next node.
      node = ageList.getLast();
      // If there are no more entries in the age list, return.
      if( node == null ) {
        return;
      }
    }
  }

  /**
   * Removes the least recently used elements if the cache size is greater than
   * or equal to the maximum allowed size until the cache is at least 10%
   * empty.
   */
  protected void cullCache() {
    // Check if a max cache size is defined.
    if( maxCacheSize < 0 ) {
      return;
    }

    // See if the cache is too big. If so, clean out cache until it's 10% free.
    if( map.size() > maxCacheSize ) {
      // First, delete any old entries to see how much memory that frees.
      deleteExpiredEntries();

      // Next, delete the least recently used elements until 10% of the cache
      // has been freed.
      int desiredSize = ( int ) ( maxCacheSize * .90 );
      for( int i = map.size();
           i > desiredSize;
           i-- ) {
        if( lastAccessedList.getLast() != null ) {
          // Get the key and invoke the remove method on it.
          if( remove( lastAccessedList.getLast().object, true ) == null ) {
            logger.error(
              String.format(
                CacheResource.getString( "error.attempting.remove.cullCache" ),
                lastAccessedList.getLast().object.toString() ) );

            lastAccessedList.getLast()
                            .remove();
          }
        }
      }
    }
  }
}
