#!/bin/sh

############################################
# Delete vcap rule
#

#set -x

PRIORITY=10
VCAM_HANDLE=$1

COMMAND="vcap del $VCAM_HANDLE"

echo "$COMMAND"
if ! $COMMAND; then
   echo "Failed to delete vcap rule"
   exit 1
fi

exit 0
