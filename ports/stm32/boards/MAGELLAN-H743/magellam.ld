/*
    GNU linker script for STM32H743
*/

/* Specify the memory areas */
MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 2048K
    FLASH_ISR (rx)  : ORIGIN = 0x08000000, LENGTH = 128K    /* sector 0, 128K */
    FLASH_FS (r)    : ORIGIN = 0x08020000, LENGTH = 128K    /* sector 1, 128K */
    FLASH_TEXT (rx) : ORIGIN = 0x08040000, LENGTH = 1792K   /* sectors 6*128 + 8*128 */
    DTCM (xrw)      : ORIGIN = 0x20000000, LENGTH = 128K    /* Used for storage cache */
    RAM (xrw)       : ORIGIN = 0x24000000, LENGTH = 512K    /* AXI SRAM */    
    RAM_D2 (xrw)    : ORIGIN = 0x30000000, LENGTH = 288K
	SDRAM (xrw)		: ORIGIN = 0xD0000000, LENGTH = 32M
}

/* produce a link error if there is not this amount of RAM for these sections */
_minimum_stack_size = 20K;
_minimum_heap_size = 160K;

/* Define the stack.  The stack is full descending so begins just above last byte
   of RAM.  Note that EABI requires the stack to be 8-byte aligned for a call. */
_estack = ORIGIN(RAM) + LENGTH(RAM) - _estack_reserve;
_sstack = _estack - 16K; /* tunable */

/* RAM extents for the garbage collector */
_ram_start = ORIGIN(RAM);
_ram_end = ORIGIN(RAM) + LENGTH(RAM);

_heap_start = _ebss; /* heap starts just after statically allocated memory */
_heap_end = _sstack;

/* Define output sections */
SECTIONS
{		

	.hjpgd_buffers (NOLOAD) : {
	. = ABSOLUTE(0x20010000);
	*hjpgd.o*(.bss.hdma_buf)
    } >DTCM
		
	.video_buffers (NOLOAD) : {
	. = ABSOLUTE(0x30028000); 	/*32k*/
	*video.o*(.bss.video_buf)
    } >RAM_D2
		
	.wm8978_buffers (NOLOAD) : {
	. = ABSOLUTE(0x30030000);
	*wm8978.o*(.bss.i2s_rbuf)
    } >RAM_D2
		
    .eth_buffers (NOLOAD) : {
	. = ABSOLUTE(0x30040000); /*32K*/
	*eth.o*(.bss.eth_dma)
    } >RAM_D2
}
