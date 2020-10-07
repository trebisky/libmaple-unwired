This is another (and important) STM32F103 project.

It began with the fine libmaple project.
This was done by Leaf Labs and is very good work.

I however did not at all like dealing with C++.
The project was already partitioned with a low level "libmaple"
part, which looked like original code, and was entirely in C.
I have left this untouched.

The top part is what was originally called "wirish".
This was almost entirely in C++.  I have ported and partly rewritten
this to be entirely in C.  I have not yet done all of it, but I
am working on it.  I am calling my rewrite of this section
"unwired".

I took the approach in "unwired" of having a single directory with
both .c and .h files.  I find this far more convenient than running
around in circles chasing include file paths.

I have also abandoned the multi-board support they were working on
and name board specific files stm32f1-xxxx.c and such, placing
them in the same directory rather than a board specific directory
hidden away someplace.  I am lazy and like things simple.

This may seem cruel and brutal, but it is a hard world.
It suits my purposes, and the orignal libmaple is still out there
as well as STM32duino which is derived from it.
I hope nobody from Leaf Labs that finds this will feel insulted.

-----

One thing I have particularly enjoyed is that I fixed an issue with the
original libmaple.  I use an STlink SWD dongle to load code into my
STM32F103 and am used to simply typing "make burn" to load new code without
having to touch jumpers.  This would not work with the executables I
made using the original Libmaple.  This was at first confusing, and then
simply irritating.  This seems to be due to some fiddling done when the
USB driver is set up that I now bypass (it was making preparations to
allow the USB driver to handle reset, probably expecting some kind of USB
based boot loader to be used.)
