package com.formationds.commons.patterns;

public interface Visitor<VisitableT extends Visitable<ThisT, VisitableT, ExT>,
                         ThisT extends Visitor<VisitableT, ThisT, ExT>,
                         ExT extends Exception>
{
    void doVisit(VisitableT visitable) throws ExT;
}
