/*****************************************
 *          VulcanOS Kernel              *
 * Developed by Marco 'icebit' Cetica    * 
 *            (c) 2019-2021              *
 *       Released under GPLv3            *
 * https://github.com/ice-bit/vulcanos   *
 *****************************************/
#include "drivers/tty.h"
#include "drivers/gdt.h"
#include "drivers/idt.h"
#include "drivers/timer.h"
#include "drivers/keyboard.h"
#include "mem/paging.h"
#include "mem/kheap.h"
#include "mem/multiboot.h"
#include "userspace/shell.h"
#include "libc/stdio.h"
#include "libc/panic.h"

#include <stdint.h>

#define PRTOK printf("\n["); printf_color(" OK ", LIGHT_GREEN, BLACK); printf("]"); // Ugly hack to print "[ OK ]"
#define PRTAT printf("\n["); printf_color(" ** ", LIGHT_BROWN, BLACK); printf("]"); // Ugly hack to print "[ * ]"
#define PRTER printf("\n["); printf_color(" ERR ", LIGHT_RED, BLACK); printf("]"); // Ugly hack to print "[ ER ]"

void kernel_main(uint32_t magic, uint32_t addr) {
    // First of all, check if we're booted by a Multiboot-compliant boot loader
    if(magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        PRTER
        printf("   - Invalid magic number: %x\n", (unsigned)magic);
        PANIC("Invalid multiboot magic number");
    }

    if(addr & 7) {
        PRTER
        printf("   - Unaligned mbi: %x\n", addr);
        PANIC("Unaligned multiboot MBI");
    }

       
    printf("Loading kernel, wait please...");
    
    gdt_setup(); // Setup Global Descriptor Table
    PRTOK
    printf("   - Loaded GDT");

    idt_setup(); // Setup Interrupt Descriptor Table
    PRTOK
    printf("   - Loaded IDT");

    init_timer(1); // Initialize PIT driver
    PRTOK
    printf("   - Loaded PIT");

    init_keyboard(); // Initialize keyboard driver
    PRTOK
    printf("   - Loaded PS/2 driver");

    init_paging(); // Initialize paging
    PRTOK
    printf("   - Loaded Paging");
    
    PRTAT
    printf("   - Testing heap...\t");

    uint32_t x = kmalloc(32), y = kmalloc(32);
    printf("x: %x, y: %x", x, y);
    kfree((void*)y);
    uint32_t z = kmalloc(8);
    printf(", z: %x", z); // If z is equal to y, heap's anti-fragmentation algorithm works
    if(z != y) {
        PRTER
        puts("   - Heap test has failed!");
        PANIC("Different heap pages");
    }

    kfree((void*)z), kfree((void*)x);

    PRTOK
    printf("   - Heap works!");

    init_prompt(); // Initialize frame buffer
}
