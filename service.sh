#!/bin/bash

# Determine script path.
rel_path="$(dirname -- "${BASH_SOURCE[0]}")"
abs_path="$(cd -- "$rel_path" && pwd)" 

run() {
	# Make sure there is a data directory.
	mkdir -p data
	mkdir -p data/crashes
	
	# Clear latest log
	echo "Log started on $(date)" > data/latest.log
	echo "User: $USER, Home: $HOME" >> data/latest.log
	echo "Working directory: $(pwd)" >> data/latest.log
	echo "Starting '$abs_path/musicsys'" >> data/latest.log
	echo >> data/latest.log
	
	# Start program, while saving a log
	"$abs_path/musicsys" 1>>data/latest.log 2>&1
	
	# Report exit status
	ec=$?
	echo >> data/latest.log
	echo "Program exited with code $ec" >> data/latest.log
	
	# If nonzero, save log
	cp data/latest.log "data/crashes/crash_$(date | sed -e 's/\W/_/g').log"
	
	return $ec
}

# Check for path parameter
if [[ "$1" != "" ]]; then
	cd "$1"
fi

# Run the server once
run
