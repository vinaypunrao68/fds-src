/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.cache;

/**
 * @author ptinius
 */
class LinkedList {
  private LinkedListNode head = new LinkedListNode( "head", null, null );

  /**
   * Creates a new linked list.
   */
  public LinkedList() {
    head.next = head.previous = head;
  }

  /**
   * @return Returns the first element of the list.
   */
  @SuppressWarnings("UnusedDeclaration")
  public LinkedListNode getFirst() {
    LinkedListNode node = head.next;
    if( node == head ) {
      return null;
    }
    return node;
  }

  /**
   * @return Returns the last element of the list.
   */
  public LinkedListNode getLast() {
    LinkedListNode node = head.previous;
    if( node == head ) {
      return null;
    }
    return node;
  }

  /**
   * @param node the node to add to the beginning of the list.
   *
   * @return Returns the node
   */
  public LinkedListNode addFirst( final LinkedListNode node ) {
    node.next = head.next;
    node.previous = head;
    node.previous.next = node;
    node.next.previous = node;
    return node;
  }

  /**
   * @param object the object to add to the beginning of the list.
   *
   * @return Returns the node created to wrap the object.
   */
  public LinkedListNode addFirst( final Object object ) {
    LinkedListNode node = new LinkedListNode( object, head.next, head );
    node.previous.next = node;
    node.next.previous = node;
    return node;
  }

  /**
   * @param object the object to add to the end of the list.
   *
   * @return Returns the node created to wrap the object.
   */
  @SuppressWarnings("UnusedDeclaration")
  public LinkedListNode addLast( final Object object ) {
    LinkedListNode node = new LinkedListNode( object, head, head.previous );
    node.previous.next = node;
    node.next.previous = node;
    return node;
  }

  /**
   * Erases all elements/re-initializes it.
   */
  public void clear() {
    //Remove all references in the list.
    LinkedListNode node = getLast();
    while( node != null ) {
      node.remove();
      node = getLast();
    }

    //Re-initialize.
    head.next = head.previous = head;
  }

  /**
   * @return Returns a String representation of the LinkedList.
   */
  public String toString() {
    LinkedListNode node = head.next;
    StringBuilder buf = new StringBuilder();
    while( node != head ) {
      buf.append( node.toString() )
         .append( ", " );
      node = node.next;
    }
    return buf.toString();
  }
}