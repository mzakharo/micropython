/* Memory layout for internal flash storage configuration:

    FLASH_ISR   .isr_vector

    FLASH_TEXT  .text
    FLASH_TEXT  .data

    RAM         .data
    RAM         .bss
    RAM         .heap
    RAM         .stack
*/

ENTRY(Reset_Handler)

/* define output sections */
SECTIONS
{
    /* The startup code goes first into FLASH */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector)) /* Startup code */

        /* This first flash block is 16K annd the isr vectors only take up
           about 400 bytes. So we pull in a couple of object files to pad it
           out. */

        . = ALIGN(4);

        /* NOTE: If you update the list of files contained in .isr_vector,
                 then be sure to also update smhal/Makefile where it forcibly
           builds each of these files with -Os */

        */ff.o(.text*)
        */vfs_fat_*.o(.text*)
        */py/formatfloat.o(.text*)
        */py/parsenum.o(.text*)
        */py/mpprint.o(.text*)

        . = ALIGN(4);
    } >FLASH_ISR

    /* The program code and other data goes into FLASH */
    .text :
    {
        . = ALIGN(4);
        *(.text*)          /* .text* sections (code) */
        *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    /*  *(.glue_7)   */    /* glue arm to thumb code */
    /*  *(.glue_7t)  */    /* glue thumb to arm code */

        . = ALIGN(4);
        _etext = .;        /* define a global symbol at end of code */
    } >FLASH_TEXT

		/* This linker script fragment is intended to be included in SECTIONS. */



		/* Used by the start-up code to initialise data */
		_sidata = LOADADDR(.data);

		/* Main framebuffer memory */
		.sdram_memory (NOLOAD) :
		{
			. = ALIGN(4);
			_sdram_base = .;
			. = . + (16M);
			. = ALIGN(4);
			_hjpgdbuf = .;
			. = . + (3072K);
			. = ALIGN(4);
			_houtbuf = .;
			. = . + (128K);
			. = ALIGN(4);
			_ltdcbuf = .;
			. = . + (5120K);
			. = ALIGN(4);
			_grapbuf = .;
			. = . + (1024K);
			. = ALIGN(4);
			_cachedbuf = .;
			. = . + (16K);
			. = ALIGN(4);
			_sdram_end = .;
		} >SDRAM


		/* Initialised data section, start-up code will copy it from flash to RAM */
		.data :
		{
				. = ALIGN(4);
				_sdata = .;
				*(.data*)
				. = ALIGN(4);
				_edata = .;
		} >RAM AT> FLASH_TEXT

		/* This linker script fragment is intended to be included in SECTIONS. */

		/* Zeroed-out data section */
		.bss :
		{
				. = ALIGN(4);
				_sbss = .;
				*(.bss*)
				*(COMMON)
				. = ALIGN(4);
				_ebss = .;
		} >RAM

		/* This is to define the start of the heap, and make sure there is a minimum size */
		.heap :
		{
				. = ALIGN(4);
				. = . + _minimum_heap_size;
				. = ALIGN(4);
		} >RAM

		/* This checks there is enough RAM for the stack */
		.stack :
		{
				. = ALIGN(4);
				. = . + _minimum_stack_size;
				. = ALIGN(4);
		} >RAM
		
		/* For C++ exception handling */
		.ARM :
		{
				__exidx_start = .;
				*(.ARM.exidx*)
				__exidx_end = .;
		} >FLASH_TEXT
}
