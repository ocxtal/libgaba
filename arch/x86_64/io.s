	.text
	.align 4

# pop functions

	# ascii
	.globl _pop_ascii
	.globl __pop_ascii
_pop_ascii:
__pop_ascii:
	movb (%rdi, %rsi), %al
	movq %rax, %rsi
	shrq $2, %rax
	shrq $1, %rsi
	xorq %rax, %rsi
	andq $3, %rax
	ret

	# 4bit encoded (1base per byte)
	.globl _pop_4bit
	.globl __pop_4bit
_pop_4bit:
__pop_4bit:
	movb (%rdi, %rsi), %al
	movq %rax, %rsi
	shrq $3, %rax
	shrq $1, %rsi
	andq $1, %rax
	subq %rax, %rsi
	andq $3, %rax
	ret

	# 2bit encoded (1base per byte)
	.globl _pop_2bit
	.globl __pop_2bit
_pop_2bit:
__pop_2bit:
	movb (%rdi, %rsi), %al
	ret

	# 4bit packed
	.globl _pop_4bit8packed
	.globl __pop_4bit8packed
_pop_4bit8packed:
__pop_4bit8packed:
	movq %rsi, %rcx
	shrq $1, %rsi
	movb (%rdi, %rsi), %al
	shlq $2, %rcx
	andq $4, %rcx
	shrq %cl, %rax
	movq %rax, %rsi
	shrq $3, %rax
	shrq $1, %rsi
	andq $1, %rax
	subq %rax, %rsi
	andq $3, %rax	
	ret

	# 2bit packed
	.globl _pop_2bit8packed
	.globl __pop_2bit8packed
_pop_2bit8packed:
__pop_2bit8packed:
	movq %rsi, %rcx
	shrq $2, %rsi
	movb (%rdi, %rsi), %al
	shlq $1, %rcx
	andq $6, %rcx
	shrq %cl, %rax
	andq $3, %rax
	ret

# push functions

	# ascii
	.globl _init_ascii
	.globl __init_ascii
_init_ascii:
__init_ascii:
	addq $-1, %rsi
	movb $0, (%rdi, %rsi)
	movq %rsi, %rax
	ret

	.globl _pushm_ascii
	.globl __pushm_ascii
_pushm_ascii:
__pushm_ascii:
	addq $-1, %rsi
	movb $77, (%rdi, %rsi)
	movq %rsi, %rax
	ret

	.globl _pushx_ascii
	.globl __pushx_ascii
_pushx_ascii:
__pushx_ascii:
	addq $-1, %rsi
	movb $88, (%rdi, %rsi)
	movq %rsi, %rax
	ret

	.globl _pushi_ascii
	.globl __pushi_ascii
_pushi_ascii:
__pushi_ascii:
	addq $-1, %rsi
	movb $73, (%rdi, %rsi)
	movq %rsi, %rax
	ret

	.globl _pushd_ascii
	.globl __pushd_ascii
_pushd_ascii:
__pushd_ascii:
	addq $-1, %rsi
	movb $68, (%rdi, %rsi)
	movq %rsi, %rax
	ret

	.globl _finish_ascii
	.globl __finish_ascii
_finish_ascii:
__finish_ascii:
	movq %rsi, %rax
	ret

	# cigar
	.globl _init_cigar
	.globl __init_cigar
_init_cigar:
__init_cigar:
	addq $-1, %rsi
	movb $0, (%rdi, %rsi)
	addq $-1, %rsi
	movb $77, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-4, %rsi
	movl $0, (%rdi, %rsi)
	ret

	.globl _pushm_cigar
	.globl __pushm_cigar
_pushm_cigar:
__pushm_cigar:
	cmpb $77, (%rdi, %rsi)
	je __pushm_cigar_incr
	movq %rsi, %rdx
	addq $-4, %rdx
	movl (%rdi, %rdx), %eax
	movq $10, %rcx
__pushm_cigar_loop:
	movq $0, %rdx
	divq %rcx
	addq $48, %rdx
	addq $-1, %rsi
	movb %dl, (%rdi, %rsi)
	cmpl $0, %eax
	jne __pushm_cigar_loop
	addq $-1, %rsi
	movb $77, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-4, %rsi
	movl $1, (%rdi, %rsi)
	ret
__pushm_cigar_incr:
	movq %rsi, %rax
	addq $-4, %rsi
	addl $1, (%rdi, %rsi)
	ret

	.globl _pushx_cigar
	.globl __pushx_cigar
_pushx_cigar:
__pushx_cigar:
	cmpb $88, (%rdi, %rsi)
	je __pushx_cigar_incr
	movq %rsi, %rdx
	addq $-4, %rdx
	movl (%rdi, %rdx), %eax
	movq $10, %rcx
__pushx_cigar_loop:
	movq $0, %rdx
	divq %rcx
	addq $48, %rdx
	addq $-1, %rsi
	movb %dl, (%rdi, %rsi)
	cmpl $0, %eax
	jne __pushx_cigar_loop
	addq $-1, %rsi
	movb $88, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-4, %rsi
	movl $1, (%rdi, %rsi)
	ret
__pushx_cigar_incr:
	movq %rsi, %rax
	addq $-4, %rsi
	addq $1, (%rdi, %rsi)
	ret

	.globl _pushi_cigar
	.globl __pushi_cigar
_pushi_cigar:
__pushi_cigar:
	cmpb $73, (%rdi, %rsi)
	je __pushi_cigar_incr
	movq %rsi, %rdx
	addq $-4, %rdx
	movl (%rdi, %rdx), %eax
	movq $10, %rcx
__pushi_cigar_loop:
	movq $0, %rdx
	divq %rcx
	addq $48, %rdx
	addq $-1, %rsi
	movb %dl, (%rdi, %rsi)
	cmpl $0, %eax
	jne __pushi_cigar_loop
	addq $-1, %rsi
	movb $73, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-4, %rsi
	movl $1, (%rdi, %rsi)
	ret
__pushi_cigar_incr:
	movq %rsi, %rax
	addq $-4, %rsi
	addq $1, (%rdi, %rsi)
	ret

	.globl _pushd_cigar
	.globl __pushd_cigar
_pushd_cigar:
__pushd_cigar:
	cmpb $68, (%rdi, %rsi)
	je __pushd_cigar_incr
	movq %rsi, %rdx
	addq $-4, %rdx
	movl (%rdi, %rdx), %eax
	movq $10, %rcx
__pushd_cigar_loop:
	movq $0, %rdx
	divq %rcx
	addq $48, %rdx
	addq $-1, %rsi
	movb %dl, (%rdi, %rsi)
	cmpl $0, %eax
	jne __pushd_cigar_loop
	addq $-1, %rsi
	movb $68, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-4, %rsi
	movl $1, (%rdi, %rsi)
	ret
__pushd_cigar_incr:
	movq %rsi, %rax
	addq $-4, %rsi
	addq $1, (%rdi, %rsi)
	ret

	.globl _finish_cigar
	.globl __finish_cigar
_finish_cigar:
__finish_cigar:
	movq %rsi, %rdx
	addq $-4, %rdx
	movl (%rdi, %rdx), %eax
	movq $10, %rcx
__finish_cigar_loop:
	movq $0, %rdx
	divq %rcx
	addq $48, %rdx
	addq $-1, %rsi
	movb %dl, (%rdi, %rsi)
	cmpl $0, %eax
	jne __finish_cigar_loop
	movq %rsi, %rax
	ret

	# direction string
	.globl _init_dir
	.globl __init_dir
_init_dir:
__init_dir:
	addq $-1, %rsi
	movb $0, (%rdi, %rsi)
	movq %rsi, %rax
	ret

	.globl _pushm_dir
	.globl __pushm_dir
_pushm_dir:
__pushm_dir:
	addq $-2, %rsi
	movw $1, (%rdi, %rsi)
	movq %rsi, %rax
	ret

	.globl _pushx_dir
	.globl __pushx_dir
_pushx_dir:
__pushx_dir:
	addq $-2, %rsi
	movw $1, (%rdi, %rsi)
	movq %rsi, %rax
	ret

	.globl _pushi_dir
	.globl __pushi_dir
_pushi_dir:
__pushi_dir:
	addq $-1, %rsi
	movb $1, (%rdi, %rsi)
	movq %rsi, %rax
	ret

	.globl _pushd_dir
	.globl __pushd_dir
_pushd_dir:
__pushd_dir:
	addq $-1, %rsi
	movb $0, (%rdi, %rsi)
	movq %rsi, %rax
	ret

	.globl _finish_dir
	.globl __finish_dir
_finish_dir:
__finish_dir:
	movq %rsi, %rax
	ret
