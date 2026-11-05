CSOPESY MO1 - Process Scheduler
Group 6, S01

Members
  Gonzaga, Rainer
  Gonzales, Aaron James
  Ramos, Richmond Jose
  Yasumuro, Mariel

Entry point
  src/main.cpp (main)

Building
  Open ProcessScheduler.sln in Visual Studio 2022, pick Release / x64, and build.
  Or from a developer prompt:
    msbuild ProcessScheduler.sln /p:Configuration=Release /p:Platform=x64

Running
  build\Release\process-scheduler.exe

  The build copies config.txt next to the exe, and the program reads config.txt
  from the folder it's run in. Edit build\Release\config.txt to change the
  parameters; there's no need to rebuild. Type "initialize" first.

Commands
  initialize             read config.txt and start the CPU cores
  screen -s <name>       create a process and attach to its screen
  screen -r <name>       reattach to a running process
  screen -ls             CPU utilization, running and finished processes
                         (screen-ls works too)
  scheduler-start        generate a process every batch-process-freq x 10 ticks
                         (scheduler-test works too)
  scheduler-stop         stop generating processes
  report-util            save the screen -ls output to csopesy-log.txt
  exit                   quit

  Inside a process screen:
  process-smi            process info and its logs
  exit                   back to the main menu

config.txt
  num-cpu              1 to 128
  scheduler            "fcfs" or "rr"
  quantum-cycles       RR time slice in ticks
  batch-process-freq   ticks between generated processes, in steps of 10 ticks
  min-ins / max-ins    instruction count range per process
  delays-per-exec      busy-wait ticks between instructions
                       (delay-per-exec is accepted too)
  x-counter            optional, 0 by default. 1 gives every process x = 0 and
                       alternating PRINT("Value from: " + x) and ADD(x, x, 1-10).
                       Set it to 1 only for test case 5.
