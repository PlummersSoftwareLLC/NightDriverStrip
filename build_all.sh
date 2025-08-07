#!/bin/bash

# Parallel Build Script for NightDriverStrip
# Usage: ./build_all.sh [max_jobs]
# Example: ./build_all.sh 4    # Build with 4 parallel jobs

set -e

# Get max parallel jobs (default to number of CPU cores)
MAX_JOBS=${1:-$(nproc)}

echo "Building all NightDriverStrip environments with $MAX_JOBS parallel jobs..."

# Get list of all available environments
ENV_LIST=$(pio project data | grep -A 1000 "Environments:" | grep "^Name:" | awk '{print $2}' | grep -v "^$")

if [ -z "$ENV_LIST" ]; then
    echo "Error: No environments found. Make sure you're in a PlatformIO project directory."
    exit 1
fi

# Convert to array
readarray -t ENVS <<< "$ENV_LIST"

echo "Found ${#ENVS[@]} environments:"
printf '%s\n' "${ENVS[@]}" | sed 's/^/  - /'
echo

# Function to build a single environment
build_env() {
    local env=$1
    local start_time=$(date +%s)
    
    echo "[$(date '+%H:%M:%S')] Starting build for $env..."
    
    if pio run -e "$env" > "build_${env}.log" 2>&1; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        echo "[$(date '+%H:%M:%S')] ✅ $env completed in ${duration}s"
        echo "$env,SUCCESS,$duration" >> build_results.csv
    else
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        echo "[$(date '+%H:%M:%S')] ❌ $env failed in ${duration}s"
        echo "$env,FAILED,$duration" >> build_results.csv
    fi
}

# Export function for parallel execution
export -f build_env

# Initialize results file
echo "Environment,Status,Duration(s)" > build_results.csv

# Start timing
BUILD_START=$(date +%s)

# Run builds in parallel using xargs
printf '%s\n' "${ENVS[@]}" | xargs -n 1 -P "$MAX_JOBS" -I {} bash -c 'build_env "$@"' _ {}

# Calculate total time
BUILD_END=$(date +%s)
TOTAL_DURATION=$((BUILD_END - BUILD_START))

echo
echo "=== Build Summary ==="
echo "Total time: ${TOTAL_DURATION}s"
echo

# Show results summary
if [ -f build_results.csv ]; then
    echo "Results summary:"
    echo "----------------"
    
    SUCCESS_COUNT=$(grep -c ",SUCCESS," build_results.csv || echo 0)
    FAILED_COUNT=$(grep -c ",FAILED," build_results.csv || echo 0)
    
    echo "✅ Successful builds: $SUCCESS_COUNT"
    echo "❌ Failed builds: $FAILED_COUNT"
    
    if [ $FAILED_COUNT -gt 0 ]; then
        echo
        echo "Failed environments:"
        grep ",FAILED," build_results.csv | cut -d',' -f1 | sed 's/^/  - /'
        echo
        echo "Check individual log files (build_<env>.log) for detailed error information."
    fi
    
    echo
    echo "Detailed results saved to: build_results.csv"
    echo "Individual logs saved to: build_<environment>.log"
else
    echo "Error: Results file not found."
fi

# Cleanup on successful completion
if [ $FAILED_COUNT -eq 0 ]; then
    echo "All builds successful! Cleaning up log files..."
    rm -f build_*.log
fi
