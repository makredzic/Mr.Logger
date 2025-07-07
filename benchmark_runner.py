#!/usr/bin/env python3
"""
Automated benchmark runner for C++20 Logger
Handles dependency installation, compilation, benchmark execution, and result visualization
"""

import subprocess
import json
import os
import sys
import time
import statistics
from pathlib import Path
from datetime import datetime
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from typing import List, Dict, Any

class BenchmarkRunner:
    def __init__(self, repo_path: str, num_runs: int = 10):
        self.repo_path = Path(repo_path)
        self.num_runs = num_runs
        self.results_dir = self.repo_path / "BenchmarkResults"
        self.plots_dir = self.repo_path / "BenchmarkPlots"
        self.build_dir = self.repo_path / "build"
        
        # Ensure directories exist
        self.results_dir.mkdir(exist_ok=True)
        self.plots_dir.mkdir(exist_ok=True)
        
        # Benchmark executable names from meson.build
        self.benchmarks = [
            "BenchmarkDefault",
            "BenchmarkSmall", 
            "BenchmarkLarge",
            "BenchmarkDefaultMultithread",
            "BenchmarkSmallMultithread",
            "BenchmarkLargeMultithread"
        ]
        
    def install_dependencies(self):
        """Install system dependencies for building C++20 code"""
        print("Installing system dependencies...")
        
        # Update package list
        subprocess.run(["sudo", "apt", "update"], check=True)
        
        # Install build dependencies
        dependencies = [
            "build-essential",
            "gcc-11",
            "g++-11",
            "meson",
            "ninja-build",
            "python3-pip",
            "git"
        ]
        
        subprocess.run(["sudo", "apt", "install", "-y"] + dependencies, check=True)
        
        # Set GCC 11 as default for C++20 support
        subprocess.run(["sudo", "update-alternatives", "--install", "/usr/bin/gcc", "gcc", "/usr/bin/gcc-11", "100"], check=True)
        subprocess.run(["sudo", "update-alternatives", "--install", "/usr/bin/g++", "g++", "/usr/bin/g++-11", "100"], check=True)
        
        print("Dependencies installed successfully!")
        
    def setup_meson_build(self):
        """Configure meson build system"""
        print("Setting up meson build...")
        
        # Clean previous build if exists
        if self.build_dir.exists():
            subprocess.run(["rm", "-rf", str(self.build_dir)], check=True)
        
        # Configure meson with release optimizations (without LTO to avoid archive issues)
        subprocess.run([
            "meson", "setup", "build",
            "--buildtype=release",
            "-Db_ndebug=true"
        ], cwd=self.repo_path, check=True)
        
        print("Meson build configured!")
        
    def compile_benchmarks(self):
        """Compile all benchmark executables"""
        print("Compiling benchmarks...")
        
        # Compile using ninja with proper environment
        env = os.environ.copy()
        env['NINJA_STATUS'] = '[%f/%t] '
        
        try:
            result = subprocess.run(["ninja", "-C", "build"], cwd=self.repo_path, env=env, 
                                  capture_output=True, text=True, check=True)
        except subprocess.CalledProcessError as e:
            # If compilation fails due to archive index issues, try to fix them
            error_output = e.stdout + e.stderr if e.stderr else e.stdout
            if "archive has no index" in error_output or "ranlib" in error_output:
                print("Detected archive index issue, attempting to fix with ranlib...")
                self._fix_archive_indices()
                # Retry compilation
                result = subprocess.run(["ninja", "-C", "build"], cwd=self.repo_path, env=env, 
                                      capture_output=True, text=True, check=True)
            else:
                print(f"Compilation failed with error: {error_output}")
                raise
        
        print("Benchmarks compiled successfully!")
    
    def _fix_archive_indices(self):
        """Fix archive index issues by running ranlib on static libraries"""
        print("Fixing archive indices...")
        
        # Find all .a files in the build directory
        build_path = self.repo_path / "build"
        archive_files = list(build_path.rglob("*.a"))
        
        for archive_file in archive_files:
            try:
                print(f"  Running ranlib on {archive_file}")
                subprocess.run(["ranlib", str(archive_file)], check=True)
            except subprocess.CalledProcessError as e:
                print(f"  Warning: Could not run ranlib on {archive_file}: {e}")
                continue
        
    def run_single_benchmark(self, benchmark_name: str, run_number: int) -> Dict[str, Any]:
        """Run a single benchmark and return the result"""
        print(f"  Run {run_number + 1}/{self.num_runs} of {benchmark_name}")
        
        # Run the benchmark executable directly
        benchmark_executable = self.build_dir / "Benchmarks" / benchmark_name
        start_time = time.time()
        try:
            subprocess.run([str(benchmark_executable)], cwd=self.repo_path, 
                          capture_output=True, text=True, check=True)
        except subprocess.CalledProcessError as e:
            error_output = e.stdout + e.stderr if e.stderr else e.stdout
            print(f"  Benchmark execution failed with error: {error_output}")
            raise
        end_time = time.time()
        
        # Find the latest JSON result file
        json_files = list(self.results_dir.glob("*.json"))
        if not json_files:
            raise FileNotFoundError(f"No JSON result files found in {self.results_dir}. The benchmark may not have produced output.")
        latest_json = max(json_files, key=lambda p: p.stat().st_mtime)
        
        # Read the benchmark result
        with open(latest_json, 'r') as f:
            result = json.load(f)
        
        # Add metadata
        result['run_number'] = run_number
        result['wall_time'] = end_time - start_time
        result['timestamp'] = datetime.now().isoformat()
        
        # Move/rename the file to avoid conflicts
        new_name = f"{benchmark_name}_run{run_number}_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        latest_json.rename(self.results_dir / new_name)
        
        return result
        
    def run_all_benchmarks(self) -> Dict[str, List[Dict[str, Any]]]:
        """Run all benchmarks multiple times"""
        all_results = {}
        
        for benchmark in self.benchmarks:
            print(f"\nRunning benchmark: {benchmark}")
            benchmark_results = []
            
            for run in range(self.num_runs):
                try:
                    result = self.run_single_benchmark(benchmark, run)
                    benchmark_results.append(result)
                    
                    # Small delay between runs to let system stabilize
                    time.sleep(2)
                    
                except Exception as e:
                    print(f"  Error in run {run + 1}: {e}")
                    continue
            
            all_results[benchmark] = benchmark_results
            
        return all_results
        
    def analyze_results(self, results: Dict[str, List[Dict[str, Any]]]) -> pd.DataFrame:
        """Analyze benchmark results and create summary statistics"""
        summary_data = []
        
        for benchmark_name, runs in results.items():
            if not runs:
                continue
                
            # Extract metrics from all runs
            messages_per_sec = [r['messages_per_second'] for r in runs]
            duration_ms = [r['duration_ms'] for r in runs]
            
            # Calculate statistics
            summary = {
                'benchmark': benchmark_name,
                'runs': len(runs),
                'avg_messages_per_sec': statistics.mean(messages_per_sec),
                'std_messages_per_sec': statistics.stdev(messages_per_sec) if len(messages_per_sec) > 1 else 0,
                'min_messages_per_sec': min(messages_per_sec),
                'max_messages_per_sec': max(messages_per_sec),
                'median_messages_per_sec': statistics.median(messages_per_sec),
                'avg_duration_ms': statistics.mean(duration_ms),
                'threads': runs[0].get('threads', 1),
                'messages_logged': runs[0]['messages_logged']
            }
            
            summary_data.append(summary)
            
        return pd.DataFrame(summary_data)
        
    def create_plots(self, results: Dict[str, List[Dict[str, Any]]], summary_df: pd.DataFrame):
        """Create visualization plots for benchmark results"""
        
        # Set style
        plt.style.use('seaborn-v0_8-darkgrid')
        
        # 1. Messages per second comparison
        fig, ax = plt.subplots(figsize=(12, 8))
        
        benchmarks = []
        avg_performance = []
        std_performance = []
        
        for _, row in summary_df.iterrows():
            benchmarks.append(row['benchmark'].replace('Benchmark', ''))
            avg_performance.append(row['avg_messages_per_sec'])
            std_performance.append(row['std_messages_per_sec'])
        
        x = np.arange(len(benchmarks))
        bars = ax.bar(x, avg_performance, yerr=std_performance, capsize=10, alpha=0.8)
        
        # Color single-threaded vs multi-threaded differently
        for i, bar in enumerate(bars):
            if 'Multithread' in benchmarks[i] or 'multi' in benchmarks[i]:
                bar.set_color('darkblue')
            else:
                bar.set_color('lightblue')
        
        ax.set_xlabel('Benchmark Type', fontsize=12)
        ax.set_ylabel('Messages per Second', fontsize=12)
        ax.set_title('Logger Performance Comparison (Average with Std Dev)', fontsize=14)
        ax.set_xticks(x)
        ax.set_xticklabels(benchmarks, rotation=45, ha='right')
        
        # Add value labels on bars
        for i, (bar, avg) in enumerate(zip(bars, avg_performance)):
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2., height,
                   f'{avg:.2e}', ha='center', va='bottom', fontsize=10)
        
        plt.tight_layout()
        plt.savefig(self.plots_dir / 'performance_comparison.png', dpi=300)
        plt.close()
        
        # 2. Box plot showing distribution across runs
        fig, ax = plt.subplots(figsize=(12, 8))
        
        box_data = []
        box_labels = []
        
        for benchmark_name, runs in results.items():
            if runs:
                box_data.append([r['messages_per_second'] for r in runs])
                box_labels.append(benchmark_name.replace('Benchmark', ''))
        
        if box_data:  # Only create boxplot if there's data
            box_plot = ax.boxplot(box_data, tick_labels=box_labels, patch_artist=True)
            
            # Color boxes
            for i, box in enumerate(box_plot['boxes']):
                if 'Multithread' in box_labels[i] or 'multi' in box_labels[i]:
                    box.set_facecolor('darkblue')
                else:
                    box.set_facecolor('lightblue')
        else:
            ax.text(0.5, 0.5, 'No benchmark data available', 
                   horizontalalignment='center', verticalalignment='center', 
                   transform=ax.transAxes, fontsize=14)
        
        ax.set_xlabel('Benchmark Type', fontsize=12)
        ax.set_ylabel('Messages per Second', fontsize=12)
        ax.set_title('Performance Distribution Across Runs', fontsize=14)
        plt.xticks(rotation=45, ha='right')
        
        plt.tight_layout()
        plt.savefig(self.plots_dir / 'performance_distribution.png', dpi=300)
        plt.close()
        
        # 3. Speedup chart (multi-threaded vs single-threaded)
        fig, ax = plt.subplots(figsize=(10, 6))
        
        configs = ['default', 'small', 'large']
        speedups = []
        
        for config in configs:
            single = summary_df[summary_df['benchmark'] == f'Benchmark{config.capitalize()}']['avg_messages_per_sec'].values
            multi = summary_df[summary_df['benchmark'] == f'Benchmark{config.capitalize()}Multithread']['avg_messages_per_sec'].values
            
            if len(single) > 0 and len(multi) > 0:
                speedup = multi[0] / single[0]
                speedups.append(speedup)
            else:
                speedups.append(0)
        
        bars = ax.bar(configs, speedups, alpha=0.8, color='green')
        
        # Add ideal speedup line (assuming 10 threads)
        threads = summary_df[summary_df['benchmark'].str.contains('Multithread')]['threads'].iloc[0]
        ax.axhline(y=threads, color='red', linestyle='--', label=f'Ideal speedup ({threads}x)')
        
        ax.set_xlabel('Configuration', fontsize=12)
        ax.set_ylabel('Speedup Factor', fontsize=12)
        ax.set_title('Multi-threaded Speedup vs Single-threaded', fontsize=14)
        ax.legend()
        
        # Add value labels
        for bar, speedup in zip(bars, speedups):
            if speedup > 0:
                ax.text(bar.get_x() + bar.get_width()/2., bar.get_height(),
                       f'{speedup:.2f}x', ha='center', va='bottom')
        
        plt.tight_layout()
        plt.savefig(self.plots_dir / 'speedup_comparison.png', dpi=300)
        plt.close()
        
        print(f"\nPlots saved to: {self.plots_dir}")
        
    def save_summary_report(self, summary_df: pd.DataFrame):
        """Save a comprehensive summary report"""
        report_path = self.plots_dir / 'benchmark_report.txt'
        
        with open(report_path, 'w') as f:
            f.write("C++20 Logger Benchmark Report\n")
            f.write("=" * 50 + "\n\n")
            f.write(f"Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            f.write(f"Number of runs per benchmark: {self.num_runs}\n\n")
            
            f.write("Summary Statistics:\n")
            f.write("-" * 50 + "\n")
            f.write(summary_df.to_string(index=False))
            f.write("\n\n")
            
            # Calculate and report efficiency
            f.write("Multi-threading Efficiency:\n")
            f.write("-" * 50 + "\n")
            
            for config in ['default', 'small', 'large']:
                single = summary_df[summary_df['benchmark'] == f'Benchmark{config.capitalize()}']['avg_messages_per_sec'].values
                multi = summary_df[summary_df['benchmark'] == f'Benchmark{config.capitalize()}Multithread']['avg_messages_per_sec'].values
                
                if len(single) > 0 and len(multi) > 0:
                    threads = summary_df[summary_df['benchmark'] == f'Benchmark{config.capitalize()}Multithread']['threads'].values[0]
                    speedup = multi[0] / single[0]
                    efficiency = (speedup / threads) * 100
                    
                    f.write(f"{config.capitalize()} configuration:\n")
                    f.write(f"  Single-threaded: {single[0]:.2e} msg/s\n")
                    f.write(f"  Multi-threaded ({threads} threads): {multi[0]:.2e} msg/s\n")
                    f.write(f"  Speedup: {speedup:.2f}x\n")
                    f.write(f"  Efficiency: {efficiency:.1f}%\n\n")
        
        print(f"Report saved to: {report_path}")
        
    def run(self):
        """Main execution flow"""
        print("Starting C++20 Logger Benchmark Suite")
        print("=" * 50)
        
        # Install dependencies
        # self.install_dependencies()
        
        # Setup and compile
        self.setup_meson_build()
        self.compile_benchmarks()
        
        # Run benchmarks
        print("\nRunning benchmarks...")
        results = self.run_all_benchmarks()
        
        # Analyze results
        print("\nAnalyzing results...")
        summary_df = self.analyze_results(results)
        
        # Create visualizations
        print("\nCreating plots...")
        self.create_plots(results, summary_df)
        
        # Save summary report
        self.save_summary_report(summary_df)
        
        print("\nBenchmarking complete!")
        print(f"Results saved to: {self.results_dir}")
        print(f"Plots saved to: {self.plots_dir}")
        
        # Save raw results for future analysis
        raw_results_path = self.results_dir / f"all_results_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        with open(raw_results_path, 'w') as f:
            json.dump(results, f, indent=2)
        

if __name__ == "__main__":
    # Get repository path (current directory by default)
    repo_path = sys.argv[1] if len(sys.argv) > 1 else "."
    
    # Number of runs per benchmark (default 10)
    num_runs = int(sys.argv[2]) if len(sys.argv) > 2 else 10
    
    # Create and run benchmark suite
    runner = BenchmarkRunner(repo_path, num_runs)
    runner.run()
