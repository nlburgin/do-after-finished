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
