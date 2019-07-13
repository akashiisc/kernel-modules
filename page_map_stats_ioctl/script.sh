#!/bin/bash
input="/tmp/panda"
while IFS= read -r line
do
  sudo insmod page_map_stats.ko pfn=0x$line
  sudo rmmod page_map_stats
  echo "$line"
done < "$input"
