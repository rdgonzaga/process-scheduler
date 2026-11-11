# CSOPESY MO1: Process Scheduler

A multi-core CPU scheduler emulator with a command-line shell. It generates
processes with small instruction programs and runs them on 1 to 128 simulated
cores using FCFS or Round Robin, while you attach to processes and watch them run.

## Members

- Gonzaga, Rainer
- Gonzales, Aaron James
- Ramos, Richmond Jose
- Yasumuro, Mariel

## Entry point

`src/main.cpp`

## Running

**Visual Studio 2022:** open `ProcessScheduler.sln`, pick Release / x64, press Run.

**Developer prompt:**

```
msbuild ProcessScheduler.sln /p:Configuration=Release /p:Platform=x64
build\Release\process-scheduler.exe
```

The build copies `config.txt` next to the exe, and the program reads it from the
folder it runs in, so edit `build\Release\config.txt` to change parameters
without rebuilding. Type `initialize` first.

## Commands

| Command | Description |
| --- | --- |
| `initialize` | reads `config.txt` and starts the CPU cores |
| `screen -s <name>` | creates a process and attaches to its screen |
| `screen -r <name>` | reattaches to a running process |
| `screen -ls` | CPU utilization, running and finished processes |
| `scheduler-start` | generates a process every `batch-process-freq` scheduler ticks |
| `scheduler-stop` | stops generating processes |
| `report-util` | saves the `screen -ls` output to `csopesy-log.txt` |
| `exit` | quits |

Inside a process screen, `process-smi` shows the process info and its logs, and
`exit` goes back to the main menu.

## config.txt

| Key | What it does |
| --- | --- |
| `num-cpu` | number of cores, 1 to 128 |
| `scheduler` | `"fcfs"` or `"rr"` |
| `quantum-cycles` | Round Robin time slice in ticks |
| `batch-process-freq` | scheduler ticks between generated processes (one scheduler tick is 10 CPU ticks) |
| `min-ins` / `max-ins` | instruction count range per process |
| `delays-per-exec` | busy-wait ticks between instructions |
| `x-counter` | optional, 0 by default. 1 gives every process `x = 0` and alternating `PRINT` / `ADD(x, x, 1-10)` |

## How it works

One clock thread plus one thread per core, kept in lockstep with a
`std::barrier`:

| Thread | Job |
| --- | --- |
| Clock | advances the tick, generates batch processes, wakes sleepers, hands ready processes to idle cores |
| Core (one per CPU) | runs one instruction tick of its process, then waits on the barrier |
| Shell (main) | reads commands and prints screens |

Processes run a small instruction set (`PRINT`, `DECLARE`, `ADD`, `SUBTRACT`,
`SLEEP`) over 8 variables. Instructions are generated in chunks as the program
counter moves on, so thousands of queued processes stay cheap in memory. Round
Robin preempts a process after `quantum-cycles` ticks and puts it at the back of
the ready queue; `SLEEP` parks it until its wake tick. One mutex guards the
scheduler state and every process reachable from it.

## Files

```
src/main.cpp            shell loop
src/Config.h            shared constants
src/core/Scheduler.*    clock and core threads, ready queue, FCFS / RR
src/core/Process.*      process state, instruction set, logs
src/core/Commands.*     the shell commands
src/core/SystemConfig.* config.txt parsing
src/ui/Console.*        Windows console setup and writes
src/art/AsciiArt.*      header art and group info
```
