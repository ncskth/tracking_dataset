#!/bin/bash

if [ $# -eq 0 ]; then
  echo "Usage: <path-to-recordings-root>"
  exit -1
fi

MAX_PROCESSES=8
current_processes=0
file_pattern="*.raw"
total_files=$(find $1 -type f -name "$file_pattern" | wc -l)
processed_files=0
log_file="processing_log.txt"
pids=()

# Clear the log file
> "$log_file"

cleanup() {
    echo "Received Ctrl+C. Cleaning up..."
    for pid in "${pids[@]}"; do
        kill -9 "$pid"
    done
    wait "${pids[@]}"  # Wait for all processes to be terminated
    exit 1
}

# Set up trap for Ctrl+C
trap cleanup SIGINT
find "$(realpath $1)" -type f -name "$file_pattern" | while read -r in_file; do
    # Generate full path for output directory based on input file name, excluding extension
    out_dir="$(dirname "$in_file")/$(basename "$in_file" .raw)_processed"
    echo "Processing file: $in_file at $(date +"%Y-%m-%d %H:%M:%S")" | tee -a "$log_file"

    # Run data_converter in the background with stdbuf, redirecting both stdout and stderr to the log file, appending the output
    stdbuf -oL -eL ../recorder/build/data_converter "$in_file" "$out_dir" &>> "$log_file" &

    # Store the PID of the background process
    pids+=($!)

    # Increment the count of running processes and processed files
    ((current_processes++))
    ((processed_files++))

    # Check if the maximum number of processes has been reached
    if [ "$current_processes" -ge "$MAX_PROCESSES" ]; then
        # Wait for background processes to finish
        wait "${pids[@]}"
        # Reset the count of running processes
        current_processes=0
        # Clear the array of PIDs
        pids=()
    fi

    echo "Progress at $(date +"%Y-%m-%d %H:%M:%S"): $processed_files / $total_files files processed" | tee -a "$log_file"
done

# Wait for any remaining background processes to finish
wait "${pids[@]}"
