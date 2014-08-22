#!/bin/sh
ps aux | awk '$3 > 30 {print $2}' | xargs kill -9
# ¬å±¼¦Y¸ê·½ > 30 ªº process
