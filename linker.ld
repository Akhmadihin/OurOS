ENTRY(_start)

SECTIONS
{
    . = 1M;

    /* Multiboot header должен быть в начале */
    .multiboot : ALIGN(4K)
    {
        *(.multiboot)
    }

    .text : ALIGN(4K)
    {
        *(.text)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    .bss : ALIGN(4K)
    {
        *(.bss)
    }
}
