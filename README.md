# uMT
uMT - a preemptive, soft real-time (not deterministic) multitasker for the ARDUINO AVR/SAM boards

My name is Antonio Pastore (Torino, Italy) and I am dealing with real-time operating systems since late 80'. Recently I purchased an Arduino starter kit for my young son to let him understand in which area he wants to develop his professional carreer.
As a net effect, I got really fond of the environment and I replaced my useless evening in front of the TV with some Arduino programming. As a first "serious" project I decided to port a real-time kernel I developed almost 30 years ago for the Intel 8086 platform to Arduino. The task indeed proved to be a little bit too complex due to the very limited memory resources of the Arduino Uno board I was using. As a consequence I decided to rewrite it from scratch with the specific objective to make as tiny as possible to fit in the Arduino Uno board. I then purchased a Mega2560 in which uMT can nicely run thanks the 8KB RAM availability. Very recently I also purchased a Arduino Due (SAM based) board and I maneged to port this kernel also on this board (although it proved to be quite challenging...). An Arduino Zero (SAMD based) will likely to appear in the next future (I am missing the board...).


In the uMT library one can find a Getting Started document describing how to use uMT and a preliminary reference manual describing the uMT calls/primitives. The Arduino library also contains a large set of working examples to show how to use the uMT functionalities.

This is an initial version of uMT and, as such, no extensive testing has been performed. As a consequence, users are warned about the potential instability of the uMT software.

The current version runs, full functionalities, on the Arduino Uno, Mega2560 and Due.

Please note that this an EDUCATIONAL tool, not designed for industrial or state-of-the-art application (nor life or mission critical application!!!) and not fully optimized.



Main functionalities

uMT offers a rich programming environment with over 30 calls:

•	Task management: creation and deletion of independent, priority based tasks with a start-up parameter. Moreover, preemption and timesharing can be enabled/disabled at run time.

•	Semaphore management: counting semaphores with optional timeout (in the simplest form they can be used as mutual exclusion guards).

•	Event management: a configurable number of events per task (16 or 32 events depending on the AVR/SAM architecture) can be used for inter task synchronization, with optional timeout and ALL/ANY optional logic (number of events can be extended to 32/64 by reconfiguration of uMT source code).

•	Timers management: task's timers (timeouts) and agent timers (Event generation in future time) are available.

•	Support Functionalities: system tick, fatal error, rebooting, etc.


Bugs, questions and suggestions can be sent to the author at "go01260@alice.it"

Enjoy!!!


