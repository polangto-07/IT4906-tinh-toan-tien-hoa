param(
    [int[]]$Seeds = @(1..30),
    [int]$Population = 100,
    [int]$Generations = 1000,
    [string]$ResultsDir = "results"
)

$ErrorActionPreference = 'Stop'
$source = Join-Path $PSScriptRoot 'genetic_algorithm.cpp'
$binary = Join-Path $PSScriptRoot 'ga_experiment.exe'
$benchmarkDir = Join-Path $PSScriptRoot 'data'
$resultsPath = Join-Path $PSScriptRoot $ResultsDir

if (-not (Get-Command g++ -ErrorAction SilentlyContinue)) {
    throw 'Không tìm thấy g++. Hãy cài MinGW-w64 hoặc thêm g++ vào PATH.'
}

& g++ -std=c++17 -O2 $source -o $binary
if ($LASTEXITCODE -ne 0) { throw 'Biên dịch genetic_algorithm.cpp thất bại.' }

# K/Emax lấy từ mô tả benchmark Pha 1.
$cases = @(
    @{ Instance='C101';  Customers=10; UAVs=2; Emax=50.17 },
    @{ Instance='C101';  Customers=20; UAVs=3; Emax=70.32 },
    @{ Instance='C101';  Customers=50; UAVs=5; Emax=84.73 },
    @{ Instance='R101';  Customers=10; UAVs=2; Emax=138.60 },
    @{ Instance='R101';  Customers=20; UAVs=3; Emax=146.28 },
    @{ Instance='R101';  Customers=50; UAVs=5; Emax=166.15 },
    @{ Instance='RC101'; Customers=10; UAVs=2; Emax=107.84 },
    @{ Instance='RC101'; Customers=20; UAVs=3; Emax=115.93 },
    @{ Instance='RC101'; Customers=50; UAVs=5; Emax=117.14 }
)

New-Item -ItemType Directory -Force $resultsPath | Out-Null
$allRows = @()
foreach ($case in $cases) {
    $instancePath = Join-Path $benchmarkDir ($case.Instance + '.txt')
    if (-not (Test-Path -LiteralPath $instancePath)) { throw "Không tìm thấy benchmark: $instancePath" }
    $caseDir = Join-Path $resultsPath ($case.Instance + '_' + $case.Customers)
    New-Item -ItemType Directory -Force $caseDir | Out-Null
    foreach ($seed in $Seeds) {
        Write-Host "Running $($case.Instance)_$($case.Customers), seed $seed"
        & $binary --instance $instancePath --customers $case.Customers --uavs $case.UAVs --emax $case.Emax --seed $seed --population $Population --generations $Generations --output-dir $caseDir
        if ($LASTEXITCODE -ne 0) { throw "GA thất bại: $($case.Instance)_$($case.Customers), seed $seed" }
        $allRows += Import-Csv (Join-Path $caseDir "run_seed_${seed}_summary.csv")
    }
}

$allRows | Export-Csv (Join-Path $resultsPath 'all_runs.csv') -NoTypeInformation
$allRows | Group-Object instance, customers | ForEach-Object {
    $rows = $_.Group
    $feasible = @($rows | Where-Object { $_.feasible -eq '1' })
    $stats = if ($feasible.Count) { $feasible | Measure-Object best_objective -Minimum -Maximum -Average }
    [PSCustomObject]@{
        instance = $rows[0].instance
        customers = [int]$rows[0].customers
        runs = $rows.Count
        feasible_runs = $feasible.Count
        feasible_objective_min = if ($stats) { $stats.Minimum } else { $null }
        feasible_objective_mean = if ($stats) { $stats.Average } else { $null }
        feasible_objective_max = if ($stats) { $stats.Maximum } else { $null }
        runtime_mean_seconds = ($rows | Measure-Object runtime_seconds -Average).Average
    }
} | Sort-Object instance, customers | Export-Csv (Join-Path $resultsPath 'experiment_summary.csv') -NoTypeInformation

Write-Host "Hoàn tất. Kết quả: $resultsPath"
