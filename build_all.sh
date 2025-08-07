#!/bin/bash

# Parallel Build Script for NightDriverStrip
# Usage: ./build_all.sh [max_jobs]
# Example: ./build_all.sh 4    # Build with 4 parallel jobs

set -e

# Cleanup function for graceful exit
cleanup() {
    echo
    echo "Build interrupted. Cleaning up..."
    # Kill any remaining background processes
    jobs -p | xargs -r kill 2>/dev/null || true
    exit 1
}

# Set up signal handlers
trap cleanup SIGINT SIGTERM

# Show help if requested
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    echo "Usage: $0 [max_jobs]"
    echo "Build all PlatformIO environments in parallel"
    echo ""
    echo "Arguments:"
    echo "  max_jobs    Maximum number of parallel build jobs (default: number of CPU cores)"
    echo ""
    echo "Examples:"
    echo "  $0          # Build with all CPU cores"
    echo "  $0 4        # Build with 4 parallel jobs"
    echo "  $0 1        # Build sequentially"
    exit 0
fi

# Get max parallel jobs (default to number of CPU cores)
MAX_JOBS=${1:-$(nproc)}

# Validate MAX_JOBS is a positive integer
if ! [[ "$MAX_JOBS" =~ ^[1-9][0-9]*$ ]]; then
    echo "Error: Invalid number of jobs '$MAX_JOBS'. Must be a positive integer."
    echo "Use '$0 --help' for usage information."
    exit 1
fi

echo "Building all NightDriverStrip environments with $MAX_JOBS parallel jobs..."

# Get list of all available environments
ENV_LIST=$(grep "^\[env:" platformio.ini | sed 's/^\[env:\(.*\)\]$/\1/')

if [ -z "$ENV_LIST" ]; then
    echo "Error: No environments found. Make sure you're in a PlatformIO project directory with platformio.ini."
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
