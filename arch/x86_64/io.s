
#
# @file io.s
#
# @brief sequence reader / alignment writer implementation for x86_64.
#

	.text
	.align 4

# pop functions

	# ascii
	# input:  %rdi (pointer)
	#         %rsi (index)    (modified)
	# output: %rax
	# work:   None
	.globl _pop_ascii
	.globl __pop_ascii
_pop_ascii:
__pop_ascii:
	movb (%rdi, %rsi), %al
	movq %rax, %rsi
	shrq $2, %rax
	shrq $1, %rsi
	xorq %rsi, %rax
	andq $3, %rax
	ret

	# 4bit encoded (1base per byte)
	# input:  %rdi (pointer)
	#         %rsi (index)    (modified)
	# output: %rax
	# work:   None
	.globl _pop_4bit
	.globl __pop_4bit
_pop_4bit:
__pop_4bit:
	movb (%rdi, %rsi), %al
	movq %rax, %rsi
	shrq $3, %rax
	shrq $1, %rsi
	andq $1, %rax
	subq %rsi, %rax
	andq $3, %rax
	ret

	# 2bit encoded (1base per byte)
	# input:  %rdi (pointer)
	#         %rsi (index)
	# output: %rax
	# work:   None
	.globl _pop_2bit
	.globl __pop_2bit
_pop_2bit:
__pop_2bit:
	movb (%rdi, %rsi), %al
	ret

	# 4bit packed
	# input:  %rdi (pointer)
	#         %rsi (index)    (modified)
	# output: %rax
	# work:   %rcx
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
	shrq $3, %rsi
	shrq $1, %rax
	andq $1, %rsi
	subq %rsi, %rax
	andq $3, %rax	
	ret

	# 2bit packed
	# input:  %rdi (pointer)
	#         %rsi (index)    (modified)
	# output: %rax
	# work:   %rcx
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

	# ascii forward
	# input:  %rdi (pointer)
	#         %rsi (index)    (modified)
	#         %rdx (index)
	# output: %rax
	# work:   None
	.globl _init_ascii_f
	.globl __init_ascii_f
_init_ascii_f:
__init_ascii_f:
	addq $-1, %rsi
	movb $0, (%rdi, %rsi)
	movq %rsi, %rax
	ret

	# input:  %rdi (pointer)
	#         %rsi (index)    (modified)
	#         %rdx (char)
	# output: %rax
	# work:   None
	.globl _push_ascii_f
	.globl __push_ascii_f
_push_ascii_f:
__push_ascii_f:
	addq $-1, %rsi
	movb %dl, (%rdi, %rsi)
	movq %rsi, %rax
	ret

	# input:  %rdi (pointer)
	#         %rsi (index)
	# output: %rax
	# work:   None
	.globl _finish_ascii_f
	.globl __finish_ascii_f
_finish_ascii_f:
__finish_ascii_f:
	movq %rsi, %rax
	ret

	# ascii reverse
	# input:  %rdi (pointer)
	#         %rsi (index)
	#         %rdx (index)
	# output: %rax
	# work:   None
	.globl _init_ascii_r
	.globl __init_ascii_r
_init_ascii_r:
__init_ascii_r:
	movq %rdx, %rax
	ret

	# input:  %rdi (pointer)
	#         %rsi (index)
	#         %rdx (char)
	# output: %rax
	# work:   None
	.globl _push_ascii_r
	.globl __push_ascii_r
_push_ascii_r:
__push_ascii_r:
	movb %dl, (%rdi, %rsi)
	movq %rsi, %rax
	addq $1, %rax
	ret

	# input:  %rdi (pointer)
	#         %rsi (index)
	# output: %rax
	# work:   None
	.globl _finish_ascii_r
	.globl __finish_ascii_r
_finish_ascii_r:
__finish_ascii_r:
	movb $0, (%rdi, %rsi)
	movq %rsi, %rax
	addq $1, %rax
	ret

	# cigar forward
	# input:  %rdi (pointer)
	#         %rsi (index)    (modified)
	#         %rdx (index)
	# output: %rax
	# work:   None
	.globl _init_cigar_f
	.globl __init_cigar_f
_init_cigar_f:
__init_cigar_f:
	addq $-1, %rsi
	movb $0, (%rdi, %rsi)
	addq $-1, %rsi
	movb $61, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-4, %rsi
	movl $0, (%rdi, %rsi)
	ret

	# input:  %rdi (pointer)
	#         %rsi (index)    (modified)
	#         %rdx (char)     (modified)
	# output: %rax
	# work:   %rcx, %r8, %xmm0
	.globl _push_cigar_f
	.globl __push_cigar_f
_push_cigar_f:
__push_cigar_f:
	cmpb $77, %dl
	jne __push_cigar_f_mov
	movq $61, %rdx
__push_cigar_f_mov:
	cmpb %dl, (%rdi, %rsi)
	je __push_cigar_f_incr
	movb %dl, %r8b
	movq %rsi, %rdx
	addq $-4, %rdx
	movl (%rdi, %rdx), %eax
	movq $10, %rcx
__push_cigar_f_loop:
	movq $0, %rdx
	divq %rcx
	addq $48, %rdx
	addq $-1, %rsi
	movb %dl, (%rdi, %rsi)
	cmpl $0, %eax
	jne __push_cigar_f_loop
	addq $-1, %rsi
	movb %r8b, (%rdi, %rsi)
	movq %rsi, %rax
	addq $-4, %rsi
	movl $1, (%rdi, %rsi)
	ret
__push_cigar_f_incr:
	movq %rsi, %rax
	addq $-4, %rsi
	addl $1, (%rdi, %rsi)
	ret

	# input:  %rdi (pointer)
	#         %rsi (index)    (modified)
	# output: %rax
	# work:   %rdx
	.globl _finish_cigar_f
	.globl __finish_cigar_f
_finish_cigar_f:
__finish_cigar_f:
	movq %rsi, %rdx
	addq $-4, %rdx
	movl (%rdi, %rdx), %eax
	movq $10, %rcx
__finish_cigar_f_loop:
	movq $0, %rdx
	divq %rcx
	addq $48, %rdx
	addq $-1, %rsi
	movb %dl, (%rdi, %rsi)
	cmpl $0, %eax
	jne __finish_cigar_f_loop
	movq %rsi, %rax
	ret

	# cigar reverse
	# input:  %rdi (pointer)
	#         %rsi (index)
	#         %rdx (index)    (modified)
	# output: %rax
	# work:   None
	.globl _init_cigar_r
	.globl __init_cigar_r
_init_cigar_r:
__init_cigar_r:
	movb $61, (%rdi, %rdx)
	movq %rdx, %rax
	addq $1, %rdx
	movl $0, (%rdi, %rdx)
	ret

	# input:  %rdi (pointer)
	#         %rsi (index)    (modified)
	#         %rdx (char)     (modified)
	# output: %rax
	# work:   %rcx, %r8, %xmm0
	.globl _push_cigar_r
	.globl __push_cigar_r
_push_cigar_r:
__push_cigar_r:
	cmpb $77, %dl
	jne __push_cigar_r_mov
	movq $61, %rdx
__push_cigar_r_mov:
	movb (%rdi, %rsi), %r8b
	cmpb %dl, %r8b
	je __push_cigar_r_incr
	pxor %xmm0, %xmm0
	# pxor %xmm1, %xmm1
	shlq $8, %rdx
	orq %rdx, %r8
	movd %r8d, %xmm0
	movq $1, %r8		# %r8 holds the length of array in %xmm0
	movq %rsi, %rdx
	addq $1, %rdx
	movl (%rdi, %rdx), %eax
	movq $10, %rcx
__push_cigar_r_loop:
	movq $0, %rdx
	divq %rcx
	addq $48, %rdx
	# movd %edx, %xmm1
	pslldq $1, %xmm0
	# por %xmm1, %xmm0
	pinsrb $0, %edx, %xmm0
	addq $1, %r8
	cmpl $0, %eax
	jne __push_cigar_r_loop
	movdqu %xmm0, (%rdi, %rsi)
	addq %r8, %rsi
	# movb $61, (%rdi, %rsi)
	movq %rsi, %rax
	addq $1, %rsi
	movl $1, (%rdi, %rsi)
	ret
__push_cigar_r_incr:
	movq %rsi, %rax
	addq $1, %rsi
	addl $1, (%rdi, %rsi)
	ret

	# input:  %rdi (pointer)
	#         %rsi (index)    (modified)
	# output: %rax
	# work:   %rdx, %rcx, %r8, %xmm0
	.globl _finish_cigar_r
	.globl __finish_cigar_r
_finish_cigar_r:
__finish_cigar_r:
	movb (%rdi, %rsi), %r8b
	pxor %xmm0, %xmm0
	# pxor %xmm1, %xmm1
	movd %r8d, %xmm0
	movq $1, %r8		# %r8 holds the length of array in %xmm0
	movq %rsi, %rdx
	addq $1, %rdx
	movl (%rdi, %rdx), %eax
	movq $10, %rcx
__finish_cigar_r_loop:
	movq $0, %rdx
	divq %rcx
	addq $48, %rdx
	# movd %edx, %xmm1
	pslldq $1, %xmm0
	# por %xmm1, %xmm0
	pinsrb $0, %edx, %xmm0
	addq $1, %r8
	cmpl $0, %eax
	jne __finish_cigar_r_loop
	movdqu %xmm0, (%rdi, %rsi)
	addq %r8, %rsi
	movb $0, (%rdi, %rsi)
	movq %rsi, %rax
	addq $1, %rax
	ret

	# direction string forward
	# input:  %rdi (pointer)
	#         %rsi (index)    (modified)
	#         %rdx (index)
	# output: %rax
	# work:   None
	.globl _init_dir_f
	.globl __init_dir_f
_init_dir_f:
__init_dir_f:
	addq $-1, %rsi
	movb $0, (%rdi, %rsi)
	movq %rsi, %rax
	ret

	# input:  %rdi (pointer)
	#         %rsi (index)    (modified)
	#         %rdx (char)
	# output: %rax
	# work:   %rcx, %r8
	.globl _push_dir_f
	.globl __push_dir_f
_push_dir_f:
__push_dir_f:
	movq %rdx, %r8
	movq %rdx, %rcx
	shrq $2, %rcx
	xorq $1, %r8
	andq %rcx, %r8
	andq $1, %r8
	xorq %rcx, %rdx
	addq $3, %r8
	andq $1, %rdx
	andq $3, %r8
	addq $-2, %rsi
	shlq $8, %r8
	movq %rsi, %rax
	movw %r8w, (%rdi, %rsi)
	addq %rdx, %rax
	ret

	# input:  %rdi (pointer)
	#         %rsi (index)
	# output: %rax
	# work:   None
	.globl _finish_dir_f
	.globl __finish_dir_f
_finish_dir_f:
__finish_dir_f:
	movq %rsi, %rax
	ret

	# direction string reverse
	# input:  %rdi (pointer)
	#         %rsi (index)
	#         %rdx (index)
	# output: %rax
	# work:   None
	.globl _init_dir_r
	.globl __init_dir_r
_init_dir_r:
__init_dir_r:
	movq %rdx, %rax
	ret

	# input:  %rdi (pointer)
	#         %rsi (index)
	#         %rdx (char)
	# output: %rax
	# work:   %rcx, %r8
	.globl _push_dir_r
	.globl __push_dir_r
_push_dir_r:
__push_dir_r:
	movq %rdx, %r8
	movq %rdx, %rcx
	shrq $2, %rcx
	xorq $1, %r8
	andq %rcx, %r8
	andq $1, %r8
	xorq %rcx, %rdx
	addq $3, %r8
	andq $1, %rdx
	andq $3, %r8
	movw %r8w, (%rdi, %rsi)
	movq %rsi, %rax
	addq $2, %rax
	subq %rdx, %rax
	ret

	# input:  %rdi (pointer)
	#         %rsi (index)
	# output: %rax
	# work:   None
	.globl _finish_dir_r
	.globl __finish_dir_r
_finish_dir_r:
__finish_dir_r:
	movb $0, (%rdi, %rsi)
	movq %rsi, %rax
	addq $1, %rax
	ret

#
# end of io.s
#
