phase 1: entry point and basic feedback
kernel entry: write boot.asm to handle transition from bootloader. set up stack pointer and jump to c/rust code.
early serial output: write primitive uart driver (0x3f8 port). implement basic putc and print to see text in host console.
graphics init: parse framebuffer data structure from bootloader. write pixel-drawing routine and basic bitmap font renderer.

phase 2: core architecture (gdt, idt, memory)
gdt & tss: initialize global descriptor table and task state segment to prepare for privilege rings.
idt & exceptions: set up interrupt descriptor table. write assembly stubs to catch cpu exceptions (page fault, gpf) and log registers via serial.
physical memory manager (pmm): implement page allocator using bitmap. parse memory map to mark free/reserved pages.
virtual memory manager (vmm): write page table manipulation code (pml4/pdpt/pd/pt). identity map low memory and map kernel to higher half.
kernel heap: implement simple buddy allocator or list allocator for internal kernel structures.

phase 3: scheduling and ipc (the microkernel core)
threads & context switch: define thread state structure. write assembly routine to save/restore registers during timer interrupt.
scheduler: implement basic round-robin thread queue.
ipc implementation: write synchronous message passing primitives (send, receive, reply). this is the backbone for all services.

phase 4: system calls and ring 3 transition
syscall setup: configure star and lstar registers to enable syscall / sysret instructions.
core syscall handlers: implement handlers for ipc functions and basic page allocation for user space.
first user task: write a routine that manually creates an isolated address space, loads a simple user function into it, and switches context to ring 3.

phase 5: runtime and user space servers
minimal libc: write standalone string functions (memcpy, memset, strlen, strcmp) and basic printf parser.
vfs server: write an isolated user space process that tracks mount points, files, and anonymous pipes via ipc.
hardware servers: write user space daemons for keyboard input and storage devices (ramdisk or ahci), talking to the kernel via shared memory or specific architecture channels.

phase 6: custom shell (xysh)
process spawning: implement binary loading into new tasks via vfs and process manager server.
shell loop: write the main input-parse-eval loop. use custom system calls to read keyboard data, parse commands, and spawn child processes.
