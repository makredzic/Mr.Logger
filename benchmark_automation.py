#!/usr/bin/env python3
"""
Benchmark Automation Script for MRLogger

This script automates the benchmarking process by:
1. Building the project from scratch
2. Running each benchmark executable N times
3. Clearing caches between runs
4. Analyzing results and generating plots

Usage: python3 benchmark_automation.py <N>
Where N is the number of times each benchmark should be run.
"""

import argparse
import subprocess
import sys
import os
import json
import glob
import shutil
import pathlib
from typing import List, Dict, Any
import matplotlib.pyplot as plt
import numpy as np

def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Run benchmarks N times and generate statistical plots"
    )
    parser.add_argument(
        "runs", 
        type=int, 
        help="Number of times to run each benchmark"
    )
    return parser.parse_args()

def clean_and_build():
    """Clean build directory and rebuild the project."""
    print("Cleaning build directory...")
    if os.path.exists("build"):
        shutil.rmtree("build")
    
    print("Setting up build with meson...")
    result = subprocess.run(["meson", "setup", "build"], capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Meson setup failed: {result.stderr}")
        sys.exit(1)
    
    print("Compiling project...")
    result = subprocess.run(["meson", "compile", "-C", "build"], capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Compilation failed: {result.stderr}")
        sys.exit(1)
    
    print("Build completed successfully")

def discover_benchmark_executables():
    """Discover all benchmark executables in build/Benchmarks/."""
    benchmarks_dir = "build/Benchmarks"
    if not os.path.exists(benchmarks_dir):
        print(f"Error: {benchmarks_dir} directory not found")
        sys.exit(1)
    
    executables = []
    for item in os.listdir(benchmarks_dir):
        item_path = os.path.join(benchmarks_dir, item)
        # Check if it's an executable file (not a directory)
        if os.path.isfile(item_path) and os.access(item_path, os.X_OK):
            executables.append(item_path)
    
    executables.sort()  # Ensure consistent ordering
    
    if not executables:
        print("Error: No benchmark executables found")
        sys.exit(1)
    
    print(f"Found {len(executables)} benchmark executables:")
    for exe in executables:
        print(f"  - {exe}")
    
    return executables

def cleanup_between_runs():
    """Clean up between benchmark runs to ensure fair testing conditions."""
    # Delete all .log files in build directory
    log_files = glob.glob("build/*.log")
    for log_file in log_files:
        try:
            os.remove(log_file)
            print(f"Deleted {log_file}")
        except OSError as e:
            print(f"Warning: Could not delete {log_file}: {e}")
    
    # Purge OS page cache (requires sudo permissions)
    try:
        subprocess.run(["sync"], check=True)
        # Clear page cache, dentries and inodes  
        subprocess.run(["sudo", "sh", "-c", "echo 3 > /proc/sys/vm/drop_caches"], 
                      check=True, capture_output=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        # Silent failure for cache clearing since it requires sudo
        pass

def run_single_benchmark(executable_path: str, run_number: int):
    """Run a single benchmark executable."""
    print(f"  Run {run_number}: Executing {os.path.basename(executable_path)}")
    
    try:
        result = subprocess.run([executable_path], capture_output=True, text=True, cwd=".")
        if result.returncode != 0:
            print(f"    Error: Benchmark failed with return code {result.returncode}")
            print(f"    Stderr: {result.stderr}")
            return False
        
        print(f"    Completed successfully")
        return True
    except Exception as e:
        print(f"    Error executing benchmark: {e}")
        return False

def run_all_benchmarks(executables: List[str], num_runs: int):
    """Run all benchmark executables N times each with proper cleanup."""
    total_benchmarks = len(executables) * num_runs
    current_benchmark = 0
    
    for executable in executables:
        executable_name = os.path.basename(executable)
        print(f"\nRunning {executable_name} {num_runs} times:")
        
        for run_num in range(1, num_runs + 1):
            current_benchmark += 1
            print(f"Progress: {current_benchmark}/{total_benchmarks}")
            
            # Clean up before each run
            cleanup_between_runs()
            
            # Run the benchmark
            success = run_single_benchmark(executable, run_num)
            if not success:
                print(f"Warning: Run {run_num} of {executable_name} failed")
                continue
        
        print(f"Completed all runs for {executable_name}")

def parse_benchmark_results():
    """Parse all JSON result files and organize by benchmark type."""
    results_dir = "build/BenchmarkResults"
    if not os.path.exists(results_dir):
        print(f"Error: {results_dir} directory not found")
        return {}
    
    # Group results by benchmark name
    benchmark_data = {}
    
    json_files = glob.glob(os.path.join(results_dir, "*.json"))
    
    for json_file in json_files:
        try:
            with open(json_file, 'r') as f:
                data = json.load(f)
            
            benchmark_name = data['benchmark_name']
            if benchmark_name not in benchmark_data:
                benchmark_data[benchmark_name] = []
            
            benchmark_data[benchmark_name].append(data)
            
        except (json.JSONDecodeError, KeyError) as e:
            print(f"Warning: Could not parse {json_file}: {e}")
            continue
    
    print(f"Parsed results for {len(benchmark_data)} benchmark types")
    for name, results in benchmark_data.items():
        print(f"  {name}: {len(results)} runs")
    
    return benchmark_data

def calculate_statistics(benchmark_data: Dict[str, List[Dict[str, Any]]]):
    """Calculate statistical summaries for each benchmark."""
    stats = {}

    for benchmark_name, runs in benchmark_data.items():
        if not runs:
            continue

        durations_ms = [run['end_to_end_time_ms'] for run in runs]
        messages_per_sec = [run['end_to_end_messages_per_second'] for run in runs]
        queue_times_ms = [run['queue_time_ms'] for run in runs]

        stats[benchmark_name] = {
            'duration_ms': {
                'min': min(durations_ms),
                'max': max(durations_ms),
                'avg': sum(durations_ms) / len(durations_ms),
                'median': np.median(durations_ms)
            },
            'messages_per_second': {
                'min': min(messages_per_sec),
                'max': max(messages_per_sec),
                'avg': sum(messages_per_sec) / len(messages_per_sec),
                'median': np.median(messages_per_sec)
            },
            'queue_time_ms': {
                'min': min(queue_times_ms),
                'max': max(queue_times_ms),
                'avg': sum(queue_times_ms) / len(queue_times_ms),
                'median': np.median(queue_times_ms)
            },
            'run_count': len(runs)
        }

    return stats

def separate_benchmarks_by_threading(stats: Dict[str, Dict[str, Dict[str, float]]]):
    """Separate benchmark results into single-threaded and multi-threaded categories."""
    single_threaded = {}
    multi_threaded = {}
    
    for benchmark_name, data in stats.items():
        if 'MultiThread' in benchmark_name:
            multi_threaded[benchmark_name] = data
        else:
            single_threaded[benchmark_name] = data
    
    return single_threaded, multi_threaded

def create_benchmark_plot(stats: Dict[str, Dict[str, Dict[str, float]]], plot_type: str, thread_type: str, plots_dir: str):
    """Create a single benchmark plot for either duration, performance, or queue time."""
    if not stats:
        print(f"No {thread_type} benchmarks found, skipping {plot_type} plot")
        return

    benchmark_names = list(stats.keys())
    metrics = ['min', 'max', 'avg', 'median']
    x_pos = np.arange(len(benchmark_names))
    width = 0.2

    plt.figure(figsize=(14, 8))

    # Set up plot based on type
    if plot_type == 'duration':
        metric_key = 'duration_ms'
        ylabel = 'Duration (ms)'
        title = f'{thread_type} Benchmark Duration Statistics (Less is Better)'
        better_text = 'Less is Better'
    elif plot_type == 'queue_time':
        metric_key = 'queue_time_ms'
        ylabel = 'Queue Time (ms)'
        title = f'{thread_type} Benchmark Queue Time Statistics (Less is Better)'
        better_text = 'Less is Better'
    else:  # performance
        metric_key = 'messages_per_second'
        ylabel = 'Messages per Second'
        title = f'{thread_type} Benchmark Performance Statistics (More is Better)'
        better_text = 'More is Better'

    # Create bars
    for i, metric in enumerate(metrics):
        values = [stats[name][metric_key][metric] for name in benchmark_names]
        plt.bar(x_pos + i * width, values, width, label=f'{metric.capitalize()}', alpha=0.8)

    plt.xlabel('Benchmark Type')
    plt.ylabel(ylabel)
    plt.title(title)
    plt.xticks(x_pos + width * 1.5, benchmark_names, rotation=45, ha='right')
    plt.legend()
    plt.tight_layout()
    plt.grid(True, alpha=0.3)

    # Add "better" text in corner
    plt.text(0.98, 0.02, better_text, transform=plt.gca().transAxes,
             fontsize=12, fontweight='bold', ha='right', va='bottom',
             bbox=dict(boxstyle='round,pad=0.3', facecolor='yellow', alpha=0.7))

    # Save plot
    filename = f'{plot_type}_{thread_type.lower().replace("-", "_")}.png'
    plot_path = os.path.join(plots_dir, filename)
    plt.savefig(plot_path, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"{thread_type} {plot_type} plot saved to {plot_path}")

def create_plots(stats: Dict[str, Dict[str, Dict[str, float]]]):
    """Create and save statistical plots separated by threading model."""
    # Create plots directory
    plots_dir = "build/BenchmarkPlots"
    os.makedirs(plots_dir, exist_ok=True)

    # Separate benchmarks by threading model
    single_threaded, multi_threaded = separate_benchmarks_by_threading(stats)

    # Create single-threaded plots
    create_benchmark_plot(single_threaded, 'duration', 'Single-Threaded', plots_dir)
    create_benchmark_plot(single_threaded, 'queue_time', 'Single-Threaded', plots_dir)
    create_benchmark_plot(single_threaded, 'performance', 'Single-Threaded', plots_dir)

    # Create multi-threaded plots
    create_benchmark_plot(multi_threaded, 'duration', 'Multi-Threaded', plots_dir)
    create_benchmark_plot(multi_threaded, 'queue_time', 'Multi-Threaded', plots_dir)
    create_benchmark_plot(multi_threaded, 'performance', 'Multi-Threaded', plots_dir)

def analyze_and_plot_results():
    """Parse results, calculate statistics, and generate plots."""
    print("\nAnalyzing benchmark results...")
    
    # Parse all JSON results
    benchmark_data = parse_benchmark_results()
    if not benchmark_data:
        print("No benchmark results found to analyze")
        return
    
    # Calculate statistics
    stats = calculate_statistics(benchmark_data)
    
    # Print summary
    print("\nBenchmark Statistics Summary:")
    for benchmark_name, stat in stats.items():
        print(f"\n{benchmark_name} ({stat['run_count']} runs):")
        print(f"  Duration (ms): min={stat['duration_ms']['min']:.2f}, "
              f"max={stat['duration_ms']['max']:.2f}, "
              f"avg={stat['duration_ms']['avg']:.2f}, "
              f"median={stat['duration_ms']['median']:.2f}")
        print(f"  Queue Time (ms): min={stat['queue_time_ms']['min']:.2f}, "
              f"max={stat['queue_time_ms']['max']:.2f}, "
              f"avg={stat['queue_time_ms']['avg']:.2f}, "
              f"median={stat['queue_time_ms']['median']:.2f}")
        print(f"  Messages/sec: min={stat['messages_per_second']['min']:.0f}, "
              f"max={stat['messages_per_second']['max']:.0f}, "
              f"avg={stat['messages_per_second']['avg']:.0f}, "
              f"median={stat['messages_per_second']['median']:.0f}")
    
    # Create plots
    create_plots(stats)

def main():
    """Main execution function."""
    try:
        args = parse_arguments()
        
        if args.runs <= 0:
            print("Error: Number of runs must be positive")
            sys.exit(1)
        
        # Validate we're in the right directory
        if not os.path.exists("meson.build"):
            print("Error: meson.build not found. Please run this script from the project root directory.")
            sys.exit(1)
        
        print(f"Starting benchmark automation with {args.runs} runs per executable")
        
        # Build the project
        clean_and_build()
        
        # Discover benchmark executables
        executables = discover_benchmark_executables()
        
        # Run benchmarks
        run_all_benchmarks(executables, args.runs)
        
        # Analyze results and create plots
        analyze_and_plot_results()
        
        print("Benchmark automation completed successfully")
        
    except KeyboardInterrupt:
        print("\nBenchmark automation interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()