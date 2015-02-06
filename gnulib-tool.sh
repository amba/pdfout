#!/bin/sh
if test -n "$GNULIB_TOOL"
then
    "$GNULIB_TOOL" --source-base=gl --m4-base=gl/m4 --tests-base=gl/tests \
		   --with-tests "$@"
else
    echo "GNULIB_TOOL not found in environment, exiting."
    exit 1
fi
