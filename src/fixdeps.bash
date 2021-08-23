#!/bin/bash

# the point of this is to add the appropriate folder prefix to object flies
# maybe there's a better way to do this but idk

for fn in "$1"/*.d; do
	# temporary file to hold new lines
	newf="$fn-tmp"
	touch "$newf"
	while IFS="" read -r p || [ -n "$p" ]; do
        # if it's an object make rule that doesn't already have the fix
		if [[ $p == *".o"* && $p != "$2"* ]]; then
			# add directory prefix to an object file line
			p=$(echo "$2/$p")
		fi
		echo "$p" >>"$newf"
	done <"$fn"
	mv "$newf" "$fn"
done
