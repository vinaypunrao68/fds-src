package com.formationds.commons.patterns;

public interface Visitable<VisitorT extends Visitor<ThisT, VisitorT, ExT>,
                           ThisT extends Visitable<VisitorT, ThisT, ExT>,
                           ExT extends Exception>
{
    void accept(VisitorT visitor) throws ExT;
}
