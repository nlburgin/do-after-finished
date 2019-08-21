#!/bin/sh

test_positive() { return `echo "$1 < 0" | bc`; }

now=`date +%s`
then=`date -d "$1" +%s`
diff=`echo "$then - $now" | bc`

if test_positive $diff
then
  echo "alarm set for '$1' ($diff seconds from now)"
  exec alarm $diff 1 0
else
  echo "you can't set an alarm in the past!"
fi

