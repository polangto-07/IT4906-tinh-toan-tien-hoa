# Task 4 - Experiment Runner for Robust UAV

This folder is self-contained: it includes the GA source code, the PowerShell runner, and the three Solomon benchmark files used by the experiment.

## Requirements

- Windows PowerShell 5.1 or PowerShell 7+
- `g++` (MinGW-w64) available on `PATH`

## Quick test

Run from this folder:

```powershell
.\run_experiments.ps1 -Seeds 1,2 -Generations 20 -ResultsDir results_smoke
```

The command compiles `genetic_algorithm.cpp`, runs every one of the nine benchmark cases with two reproducible seeds, and writes output to `results_smoke/`.

## Full experiment

```powershell
.\run_experiments.ps1
```

The default configuration runs 30 seeds (`1..30`) for each of the nine benchmark cases. The GA uses population 100 and 1,000 generations.

## Output

- `results/all_runs.csv`: one row per execution, including seed, objective, energy feasibility and runtime.
- `results/experiment_summary.csv`: summary statistics for feasible runs by benchmark case.
- `results/<instance>_<size>/`: convergence, route and run summary files for each seed.

An execution is feasible only if `energy_rate * longest_route_distance <= Emax`.
