package com.formationds.commons.patterns;

/**
 * A visitable object from the Visitor pattern that also takes a single argument when visited.
 * 
 * @param <VisitorT> The type of visitor accepted.
 * @param <ThisT> The implementing class.
 * @param <ArgumentT> The type of argument accepted.
 * @param <ExT> The type of exception thrown.
 */
// @eclipseFormat:off
public interface VisitableWithArg<
        VisitorT extends VisitorWithArg<? super ThisT, VisitorT, ? extends ArgumentT, ? super ExT>,
        ThisT extends VisitableWithArg<VisitorT, ThisT, ArgumentT, ExT>,
        ArgumentT,
        ExT extends Exception>
{
    /**
     * Accept a visitor.
     * 
     * @param visitor The visitor.
     * @param argument The argument.
     * 
     * @throws ExT when necessary.
     */
    void accept(VisitorT visitor, ArgumentT argument) throws ExT;
}
