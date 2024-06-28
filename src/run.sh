#/usr/bin/env bash
(./fly 2>errors.log || stty sane);  cat errors.log
