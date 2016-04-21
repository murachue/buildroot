	.set reorder
	.set at

	.text

.type start, %function
.globl start
start:
	/* init stack to memsize-0x10, at kseg0 */
	lw $t0, 0x80000318 /* memsize, if CIC_6105 this is 0x800003F0? */
	li $t1, 0x80000000 - 0x10
	addu $sp, $t0, $t1

	/* magic n64 hardware init */
	li $t0, 8
	sw $t0, 0xBFC007FC

	/* reloc this code */
	li $t0, 0x803FF000 /* near 4MB */
	la $t1, start
	la $t2, end
1:
	lw $t3, ($t1)
	addiu $t1, 4
	sw $t3, ($t0)
	addiu $t0, 4
	blt $t1, $t2, 1b

	/* j 0x803FF000 + 1f - start;; Error: unsupported constant in relocation */
	/* li $t0, 0x803FF000 + (1f - start);; this goes wrong... why? */
	/* la $t0, 0x803FF000 + (1f - start);; Error: expression too complex */
	li $t0, 0x803FF000
	addiu $t0, 1f - start
	jr $t0
1:

	/* load vmlinux */
	/* we don't wait DMA, we assume DMA is ready. */
	la $t1, 0xA4600000 /* PI base */
	la $t0, 0x00000000 /* phys-ram addr */
	sw $t0, 0x00($t1) /* PI_DRAM_ADDR_REG */
	la $t0, vmlinux_start - 0x80000000/*phys-addr-ize*/ + 0x10000000/*cart-base*/
	sw $t0, 0x04($t1) /* PI_CART_ADDR_REG */
	la $t0, vmlinux_size - 1 /* note: Length must be >0. At most 7 bytes are xferred more. */
	sw $t0, 0x0C($t1) /* PI_WR_LEN_REG, start DMA cart->dram */
1:
	lw $t0, 0x10($t1) /* PI_STATUS_REG */
	andi $t0, 1 /* DMA busy */
	bnez $t0, 1b

	/* flush I/D cache */
	/*oops, could not invalidate D, it must be write-backed??*/
	/*cache */

	/* jump to kernel_start */
	j 0x80000400
end:

	.data
vmlinux_start:
	.incbin "vmlinux.bin"
	.set vmlinux_size, (. - vmlinux_start)
