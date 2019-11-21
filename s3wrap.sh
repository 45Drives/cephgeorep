#!/usr/bin/env bash

BUCKET=$1 # first argument is s3 bucket
shift

for ARG in "$@"; do
	REL=$(echo "$ARG" | gawk -F '(/\\./)' '{print $2}')
	# echo "s3cmd put $ARG s3://"$BUCKET"/"$REL""
	s3cmd put "$ARG" s3://"$BUCKET"/"$REL"
	EXIT_CODE=$?
	if [ $EXIT_CODE -ne 0 ]; then
		echo s3cmd exited with code $EXIT_CODE
		exit $EXIT_CODE
	fi
done

exit 0
