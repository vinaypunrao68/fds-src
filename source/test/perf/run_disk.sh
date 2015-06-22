#!/bin/bash
name="new"
cols="w_s wkb_s r_s rkb_s await_r await_w util"
for c in $cols
do
    ./disk2.py $1 $c $name
done
