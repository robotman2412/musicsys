#!/bin/bash

run() {
	# Clear latest log
	echo "Log started on $(date)" > data/latest.log
	
	./musicsys > >(tee -a data/latest.log) 2>&1
	
	ec=$?
	echo "Program exited with code $ec" | tee -a data/latest.log
	if [[ $ec -ne 0 ]]; then
		echo "Aborted; saving error log"
		cp data/latest.log "data/crashes/crash_$(date | sed -e 's/\W/_/g').log"
		return 1
	fi
	
	return 0
}

# While it crashes, restart it
run
while [[ $? -ne 0 ]]; do
	run
done
