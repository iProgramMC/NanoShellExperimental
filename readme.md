## NanoShell - Experimental version
NanoShell is a 32-bit operating system.

#### Be advised that this is experimental software. Usage of it is not advised.

This software is licensed under the GNU General Public License version 3.

## Implemented features

### Basic memory manager

- [x] Kernel heap
- [x] User heaps, able to be switched to at any time
- [x] Allow mapping of any memory anywhere 
- [x] Demand paging (use faults to generate pages on the fly rather than generating them instantly)
- [x] Copy-on-write when cloning (will be useful for `fork` later)
- [x] Unmapping of memory areas
- [x] **Port to the actual NanoShell operating system itself**

### File System Code
- [X] A really basic shell which allows for interaction with the OS and potential file systems
- [ ] The VFS skeleton
- [ ] Init ram disk (use Tar like NanoShell proper)
- [ ] Read the EXT2 super block
- [ ] EXT2 read files from root.
- [ ] EXT2 read files inside subdirectories.
- [ ] EXT2 write support
- [ ] EXT2 make dir support
- [ ] EXT2 unlink support
- [ ] EXT2 hardlink support
- [ ] FAT32 Skeleton
- [ ] FAT32 read files from root.
- [ ] FAT32 read files inside subdirectories.
- [ ] FAT32 write support
- [ ] FAT32 make dir support
- [ ] FAT32 unlink support
- [ ] FAT32 hardlink support
- [ ] Port to NanoShell proper

## Build instructions

To build this experimental fork of NanoShell, simply follow the [NanoShell build guide](https://github.com/iProgramMC/NanoShellOS/blob/master/readme.md). It works the same way.

### Why didn't I just make this a branch into the real NanoShell repository?
First off, there're already plenty of branches in the original NanoShell repository, but secondly, I kind of needed a start-over in order to really get a
grip on what I'm doing. Eventually these changes will be ported to the original NanoShell repository.
