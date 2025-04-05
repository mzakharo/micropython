/*
    GNU linker script for tkm32
*/

/* Specify the memory areas */
MEMORY
{
    FLASH (rx)      : ORIGIN = 0x70008000, LENGTH = 0x200000 /* 2M */ /* entire flash */
    FLASH_ISR (rx)  : ORIGIN = 0x70008000, LENGTH = 0x4000 /*16K */ /* sector 0 */
    FLASH_TEXT (rx) : ORIGIN = 0x7000C000, LENGTH = 0x1E0000/*1920K*/ /*  */
		FLASH_FS (rx)   : ORIGIN = 0x701EC000, LENGTH = 0x1C000 /*112K*/ /*  */
    RAM (xrw)       : ORIGIN = 0x701EDC00, LENGTH = 0x500000
}

/* produce a link error if there is not this amount of RAM for these sections */
_minimum_stack_size = 2K;
_minimum_heap_size = 16K;

/* Define the stack.  The stack is full descending so begins just above last byte
   of RAM.  Note that EABI requires the stack to be 8-byte aligned for a call. */
_estack = ORIGIN(RAM) + LENGTH(RAM) - _estack_reserve;
_sstack = _estack - 16K; /* tunable */

/* RAM extents for the garbage collector */
_ram_start = ORIGIN(RAM);
_ram_end = ORIGIN(RAM) + LENGTH(RAM);
_heap_start = _ebss; /* heap starts just after statically allocated memory */
_heap_end = _sstack;
