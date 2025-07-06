#!/bin/bash
# Cloud deployment script for C++20 Logger benchmarking
# This script sets up the environment, clones the repo, and runs benchmarks

set -e  # Exit on error

# Configuration
REPO_URL="${1:-}"  # Pass your git repo URL as first argument
NUM_RUNS="${2:-10}"  # Number of benchmark runs (default: 10)

if [ -z "$REPO_URL" ]; then
    echo "Usage: $0 <git_repo_url> [num_runs]"
    echo "Example: $0 https://github.com/yourusername/your-logger-repo.git 10"
    exit 1
fi

echo "==================================="
echo "C++20 Logger Benchmark Deployment"
echo "==================================="
echo "Repository: $REPO_URL"
echo "Number of runs: $NUM_RUNS"
echo ""

# Update system
echo "Updating system packages..."
sudo apt update && sudo apt upgrade -y

# Install Python and pip if not present
echo "Installing Python..."
sudo apt install -y python3 python3-pip python3-venv

# Create working directory
WORK_DIR="$HOME/logger-benchmark"
mkdir -p "$WORK_DIR"
cd "$WORK_DIR"

# Clone repository
echo "Cloning repository..."
if [ -d "repo" ]; then
    echo "Repository already exists, pulling latest changes..."
    cd repo
    git pull
    cd ..
else
    git clone "$REPO_URL" repo
fi

# Copy benchmark runner script
echo "Setting up benchmark runner..."
cp repo/benchmark_runner.py . 2>/dev/null || {
    echo "benchmark_runner.py not found in repo, creating it..."
    # The Python script content would be written here if not in repo
    # For now, we assume it's been added to the repo
}

# Create virtual environment for Python dependencies
echo "Setting up Python environment..."
python3 -m venv venv
source venv/bin/activate

# Install Python dependencies
pip install --upgrade pip
pip install -r requirements.txt

# Make the benchmark runner executable
chmod +x benchmark_runner.py

# Run the benchmarks
echo "Starting benchmark suite..."
cd repo
python3 ../benchmark_runner.py . "$NUM_RUNS"

# Create archive of results
echo "Creating results archive..."
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
ARCHIVE_NAME="benchmark_results_${TIMESTAMP}.tar.gz"
tar -czf "../$ARCHIVE_NAME" BenchmarkResults/ BenchmarkPlots/

echo ""
echo "==================================="
echo "Benchmarking Complete!"
echo "==================================="
echo "Results archive: $WORK_DIR/$ARCHIVE_NAME"
echo ""
echo "To download results to your local machine:"
echo "scp root@<server-ip>:$WORK_DIR/$ARCHIVE_NAME ."
echo ""
echo "Results directories:"
echo "- Raw results: $WORK_DIR/repo/BenchmarkResults/"
echo "- Plots: $WORK_DIR/repo/BenchmarkPlots/"
