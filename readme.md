## NanoShell - Experimental version
NanoShell is a 32-bit operating system.

#### Be advised that this is experimental software. Usage of it is not advised.

This software is licensed under the GNU General Public License version 3.

## Implemented features
### Basic memory manager
[x] Kernel heap
[x] User heaps, able to be switched to at any time
[x] Allow mapping of any memory anywhere 
[x] Demand paging (use faults to generate pages on the fly rather than generating them instantly)
[x] Copy-on-write when cloning (will be useful for `fork` later)
[ ] Unmapping of memory areas
[ ] Port to the actual NanoShell operating system itself

## Build instructions

To build this experimental fork of NanoShell, simply follow the [NanoShell build guide](https://github.com/iProgramMC/NanoShellOS/blob/master/readme.md). It works the same way.

### Why didn't I just make this a branch into the real NanoShell repository?
First off, there're already plenty of branches in the original NanoShell repository, but secondly, I kind of needed a start-over in order to really get a
grip on what I'm doing. Eventually these changes will be ported to the original NanoShell repository.
