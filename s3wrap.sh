#!/usr/bin/env bash

#    Copyright (C) 2019-2021 Joshua Boudreau <jboudreau@45drives.com>
# 
#    This file is part of cephgeorep.
# 
#    cephgeorep is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 2 of the License, or
#    (at your option) any later version.
# 
#    cephgeorep is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
# 
#    You should have received a copy of the GNU General Public License
#    along with cephgeorep.  If not, see <https://www.gnu.org/licenses/>.

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
