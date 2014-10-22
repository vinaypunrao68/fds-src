/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.cache;

/**
 * @author ptinius
 */
class LinkedListNode {
  public LinkedListNode previous;
  public LinkedListNode next;
  public Object object;

  /**
   * The creation timestamp is used in the case that the cache has a maximum
   * lifetime set. In that case, when [current time] - [creation time] > [max
   * lifetime], the object will be deleted from cache.
   */
  public long timestamp;

  /**
   * @param object   the Object that the node represents.
   * @param next     a reference to the next LinkedListNode in the list.
   * @param previous a reference to the previous LinkedListNode in the list.
   */
  public LinkedListNode( final Object object,
                         final LinkedListNode next,
                         final LinkedListNode previous ) {
    this.object = object;
    this.next = next;
    this.previous = previous;
  }

  /**
   * Removes this node from the linked list that it is a part of.
   */
  public void remove() {
    previous.next = next;
    next.previous = previous;
  }

  /**
   * @return Returns a String representation of the LinkedListNode.
   */
  public String toString() {
    return object.toString();
  }
}
