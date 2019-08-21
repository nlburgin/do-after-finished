# do-after-finished
Linux program to run a command after a process exits

usage: `do-after-finished polldelay afterdelay PID command [arg]...`

polldelay: how often (in seconds) to check if the process is finished. If set to zero, it will poll 10 times per second.

afterdelay: how long (in seconds) between the end of specified process and the running of the specified command. Should be set to a nonzero value if the parent process is expected to need time to clean up after, otherwise a race condition may occur.

PID: PID of process we're waiting for to finish. As long as it wouldn't turn up multiple PIDs, sometimes you can get away with lazily deriving the argument from `pgrep`.

command: command to be run

[arg]... additional arguments are the arguments for the command

It's meant to be used to schedule a command to be run after the completion of a long-running batch process that's already started. 

For example, say you want to leave your computer with an ffmpeg encode already running, then turn it off after the encode finishes. As long as there's only one process named FFMPEG running, you could use the command:

```bash
$ sudo do-after-finished 10 10 `pgrep ffmpeg` systemctl poweroff
```

## How to build

This is a single file C program. The following build command is recommended:

`$ gcc -O3 -fwhole-program -march=native -Wno-error -o do-after-finished do-after-finished.c; strip do-after-finished`

This is tested on GCC/Linux. Clang/Freebsd is not tested and may or may not work.

## Alarm command

Alarm.c is a similar program that simply sounds an alarm when the time is up.

```
usage: alarm pre-delay beep-delay repetitions
zero repetitions means beep infinitely
```
It uses basically the same build command.

It uses the bell character `U+0007` and needs to be run from a terminal that has audible bell support. Depending on the terminal's implementation, the sound may come either by beeping the PC speaker or by playing a chime through the general-purpose sound driver. In the latter case, it of course will require a speaker to be connected which is turned on and not muted.

the beep-delay argument sets the interval in seconds between beeps; it should generally be set to lower values (e.g. between 0.25 and 1) to things that need urgent attention, and higher values (e.g. between 10 and 60) for things that are less urgent.

It may be either used standalone, or combined with `do-after-finish` (typically using a `0` pre-delay argument in that case) to sound the alarm after a process exits.
