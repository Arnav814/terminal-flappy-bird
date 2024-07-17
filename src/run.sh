#!/usr/bin/env bash
./fly "$@" 2>errors.log
STAT=$?
if [[ STAT -ne 0 ]]; then
	stty sane
fi
cat errors.log
exit $STAT

