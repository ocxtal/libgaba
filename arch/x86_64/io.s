	.text
	.align 4

# pop functions

	# ascii
	.globl __pop_ascii
__pop_ascii:
	movb (%rdi, %rsi), %al
	ret

	# 4bit encoded (1base per byte)
	.globl __pop_4bit
__pop_4bit:
	movb (%rdi, %rsi), %al
	ret

	# 2bit encoded (1base per byte)
	.globl __pop_2bit
__pop_2bit:
	movb (%rdi, %rsi), %al
	ret

	# 4bit packed
	.globl __pop_4bit8packed
__pop_4bit8packed:
	movb (%rdi, %rsi), %al
	ret

	# 2bit packed
	.globl __pop_2bit8packed
__pop_2bit8packed:
	movb (%rdi, %rsi), %al
	ret

# push functions

	# ascii
	.globl __pushm_ascii
__pushm_ascii:
	movb $77, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-1, %rax
	ret

	.globl __pushx_ascii
__pushx_ascii:
	movb $88, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-1, %rax
	ret

	.globl __pushi_ascii
__pushi_ascii:
	movb $73, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-1, %rax
	ret

	.globl __pushd_ascii
__pushd_ascii:
	movb $68, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-1, %rax
	ret

	# cigar
	.globl __pushm_cigar
__pushm_cigar:
	movb $77, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-1, %rax
	ret

	.globl __pushx_cigar
__pushx_cigar:
	movb $88, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-1, %rax
	ret

	.globl __pushi_cigar
__pushi_cigar:
	movb $73, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-1, %rax
	ret

	.globl __pushd_cigar
__pushd_cigar:
	movb $68, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-1, %rax
	ret

	# direction string
	.globl __pushm_dir
__pushm_dir:
	movb $77, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-1, %rax
	ret

	.globl __pushx_dir
__pushx_dir:
	movb $88, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-1, %rax
	ret

	.globl __pushi_dir
__pushi_dir:
	movb $73, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-1, %rax
	ret

	.globl __pushd_dir
__pushd_dir:
	movb $68, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-1, %rax
	ret
