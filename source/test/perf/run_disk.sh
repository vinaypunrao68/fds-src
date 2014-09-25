#!/bin/bash
name="new"
cols="w_s wkb_s avgrqsz avgqusz await"
for c in $cols
do
    ./disk2.py ~/new.iostats $c $name
done
