#!/bin/sh
kill -9 `top | grep -w bbsd | grep RUN | awk '{print $1}'`
