MEMORY
{
  FLASH  (rx)  : ORIGIN = 0x08000000, LENGTH = 1024K
  CCM    (rw)  : ORIGIN = 0x10000000, LENGTH = 64K
  RAM    (rwx) : ORIGIN = 0x20000000, LENGTH = 120K
  DACRAM (rwx) : ORIGIN = 0x2001E000, LENGTH = 8K
}

_estack = ORIGIN(CCM) + LENGTH(CCM);

SECTIONS
{
  .isr_vector :
  {
    KEEP(*(.isr_vector))
  } > FLASH

  .text :
  {
    *(.text*)
  } > FLASH

  .rodata :
  {
    *(.rodata*)
  } >FLASH

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > CCM AT > FLASH
  _sidata = LOADADDR(.data);

  .bss : 
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > CCM
  
  .dma_input (NOLOAD) :
  {
    . = ALIGN(4);
    *(.dma_input*)
  } > RAM

  .dac_table (NOLOAD) :
  {
    . = ALIGN(4);
    *(.dac_table*)
  } > DACRAM
}
