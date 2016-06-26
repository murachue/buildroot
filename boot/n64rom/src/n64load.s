/* N64LOAD */
/* This loader loads vmlinux.bin at 0x80000000, and call 0x80000400(kernel_start) with $a0=memsize. */
/* note: This loader does not init sp, because loader does not use stack, and kernel inits sp itself. */

	.set reorder
	.set at

	.text

.type __start, %function
.globl __start
__start:
	/* magic n64 hardware init */
	li $t0, 8
	sw $t0, 0xBFC007FC

	b skip_params

vmlinux_bin_start: .long 0xEEEEeeee /* must be filled with PI_CART_ADDR, means phys-addr with 0x10000000(cart-base). */
vmlinux_bin_size: .long 0xDDDDdddd /* must be filled with PI_WR_LEN. note: Length must be >0. At most 7 bytes are xferred more. */

skip_params:
	lw $a0, 0x80000318 /* memsize. if CIC_6105 this is 0x800003F0? */
	lw $a1, vmlinux_bin_start
	lw $a2, vmlinux_bin_size

	/* reloc this code */
	li $t0, 0x803FF000 /* near 4MB */
	la $t1, __start
	la $t2, end
1:
	lw $t3, ($t1)
	addiu $t1, 4
	sw $t3, ($t0)
	addiu $t0, 4
	blt $t1, $t2, 1b

	/* jump to relocated code */
	/* j 0x803FF000 + 1f - __start;; Error: unsupported constant in relocation */
	/* li $t0, 0x803FF000 + (1f - __start);; this goes wrong... why? */
	/* la $t0, 0x803FF000 + (1f - __start);; Error: expression too complex */
	li $t0, 0x803FF000
	addiu $t0, 1f - __start
	jr $t0
1:

	/* flush I/D cache */
	/* do this before DMA from cart, because could not just invalidate but write-back-invalidate Dcache. */
	/* TODO dynamically fetch cache line size from CP0 reg?? */
	/* memo: from libdragon's "header" (0x03C8-0x0458), it seems I=16k/32 D=8k/16 size/line. */
	/* memo: mtc0 $zero,TagLo;mtc0 $zero,TagHi;cache 8/9 is good? */
	li $t0, 0
	li $t1, 0
	li $t2, 16*1024 /* 16K Icache 8K Dcache */
1:
	cache 0, 0($t0) /* I index inval */
	cache 0, 16($t0) /* I index inval */
	cache 1, 0($t1) /* D index wback inval */
	addiu $t0, 32
	addiu $t1, 16
	blt $t0, $t2, 1b

	/* load vmlinux */
	/* we don't wait previous DMA, we assume DMA is ready. */
	li $t0, 0xA4600000 /* PI base */
	/*li $t1, 0x00000000 /* phys-ram addr */
	/*sw $t1, 0x00($t0) /* PI_DRAM_ADDR_REG */
	sw $zero, 0x00($t0) /* PI_DRAM_ADDR_REG. currently, it is zero, use $zero. */
	sw $a1, 0x04($t0) /* PI_CART_ADDR_REG */
	sw $a2, 0x0C($t0) /* PI_WR_LEN_REG, start DMA cart->dram */

	.set push
	.set noreorder
	lw $t1, 0x10($t0) /* PI_STATUS_REG */
1:
	andi $t1, 1 /* DMA busy */
	bnez $t1, 1b
	lw $t1, 0x10($t0) /* PI_STATUS_REG, delay-slot */
	.set pop

	/* jump to kernel_start */
	j 0x80000400
end:
