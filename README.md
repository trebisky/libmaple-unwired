This is another (and important) STM32F103 project.

It began with the fine libmaple project.
This was done by Leaf Labs and is very good work.

https://github.com/leaflabs/libmaple

I however did not at all like dealing with C++.
The project was already partitioned with a low level "libmaple"
part, which looked like original code, and was entirely in C.
I have left this untouched.

The top part is what was originally called "wirish".
This was almost entirely in C++.
I have ported and recoded much of this to be entirely in C.
I am doing more all the time and will soon have the C++ scourge eradicated.
I am calling my rewrite of this section "unwired".

I took the approach in "unwired" of having a single directory with
both .c and .h files.  I find this far more convenient than running
around in circles chasing include file paths.

I have also abandoned the multi-board support they were working on
and name board specific files stm32f1-xxxx.c and such, placing
them in the same directory rather than a board specific directory
hidden away someplace.  I am lazy and I like things simple.

This may seem cruel and brutal, but it is a hard world.
It suits my purposes, and the orignal libmaple is still out there
as well as STM32duino which is derived from it.
I hope nobody from Leaf Labs that finds this will feel insulted.

-----

One thing I have particularly enjoyed is that I fixed an issue with the
original libmaple.  I use an STlink SWD dongle to load code into my
STM32F103 and am used to simply typing "make burn" to load new code without
having to touch jumpers.  This would not work with the executables I
made using the original Libmaple.  This was at first confusing,
and then became simply irritating.
It turns out that it was the call to disableDebugPorts() that was
the cause of this.  This simply calls afio_cfg_debug_ports(AFIO_DEBUG_NONE).
What this does is to switch the pins dedicated to SWD from being SWD (aka
"debug" pins) to being plain old GPIO pins.  All fine and good if you aren't
using SWD and want a couple of extra GPIO pins.
These are A13 and A14.  On the original maple_mini these were just pins on
the edge of the board, not in a handy SWD connector like on the blue pill.
