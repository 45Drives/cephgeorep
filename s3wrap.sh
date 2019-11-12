#!/usr/bin/env bash

BUCKET=$1 # first argument is s3 bucket
shift

for ARG in "$@"; do
	if [ ! -z "$ARG" ]; then
		REL=$(echo $ARG | awk -F '/./' '{print $2}')
		# echo "s3cmd put $ARG s3://"$BUCKET"/"$REL""
		s3cmd put $ARG s3://"$BUCKET"/"$REL"
	fi
done

exit 0
