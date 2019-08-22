#!/bin/sh

test_positive() { return `echo "$1 < 0" | bc`; }

now=`date +%s`
then=`date -d "$1" +%s`
diff=`echo "$then - $now" | bc`

if test_positive $diff
then
  echo "alarm clock is going to run '$2' at '$1' ($diff seconds from now)"
  alarm $diff 0.25 10
  eval $2
else
  echo "you can't set an alarm in the past!"
fi
