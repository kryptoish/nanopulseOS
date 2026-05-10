# Multiboot2 header with framebuffer request.
.set MB2_MAGIC,    0xe85250d6
.set MB2_ARCH,     0
.set MB2_LENGTH,   (mb2_end - mb2_start)
.set MB2_CHECKSUM, -(MB2_MAGIC + MB2_ARCH + MB2_LENGTH)

.section .multiboot
.align 8
mb2_start:
.long MB2_MAGIC
.long MB2_ARCH
.long MB2_LENGTH
.long MB2_CHECKSUM

# Framebuffer tag - request a 32-bit linear framebuffer (any resolution).
.align 8
.short 5
.short 0
.long  20
.long  0
.long  0
.long  32

# End tag.
.align 8
.short 0
.short 0
.long  8
mb2_end:

# Reserve a stack for the initial thread.
.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

# The kernel entry point.
.section .text
.global _start
.type _start, @function
_start:
	movl $stack_top, %esp

	# Save multiboot2 info pointer (EBX) and magic (EAX) for kernel_main.
	pushl %ebx
	pushl %eax

	call _init
	call kernel_main

	cli
1:	hlt
	jmp 1b
.size _start, . - _start
