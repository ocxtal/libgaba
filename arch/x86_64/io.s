
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
	movq %rax, %rs2i
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

# bulk read functions

	# ascii
	# input:  %rdi (dst pointer (8bit array))
	#         %rsi (src pointer (ascii array))
	#         %rdx (index)
	#         %rcx (len)
	# output: %rax
	# work:   %r8, %xmm0, %xmm1, %xmm2
	.globl _load_ascii
	.globl __load_ascii
_load_ascii:
__load_ascii:
	movl $0x03030303, %eax
	movq %rax, %xmm2
	pshufd $0, %xmm2, %xmm2
_load_ascii_bulk_loop:
	cmpq $0, %rcx
	jle _load_ascii_ret
	movdqu (%rsi, %rdx), %xmm0
	movdqa %xmm0, %xmm1
	psrlq $2, %xmm0
	psrlq $1, %xmm1
	pxor %xmm1, %xmm0
	pand %xmm2, %xmm0
	movdqu %xmm0, (%rdi)
	addq $16, %rdi
	addq $16, %rdx
	subq $16, %rcx
	jmp _load_ascii_bulk_loop
_load_ascii_ret:
	ret

	.align 16
_load_ascii_rev:
	.long 0x0c0d0e0f
	.long 0x08090a0b
	.long 0x04050607
	.long 0x00010203

	.text
	.align 4
	# ascii
	# input:  %rdi (dst pointer (8bit array))
	#         %rsi (src pointer (ascii array))
	#         %rdx (index)
	#         %rcx (src length)
	#         %r8  (copy length)
	# output: %rax
	# work:   %xmm0, %xmm1, %xmm2
	.globl _load_ascii_fw
	.globl __load_ascii_fw
_load_ascii_fw:
__load_ascii_fw:
	cmpq $0, %rdx
	jl _load_ascii_fw_inv
	movl $0x03030303, %eax
	movq %rax, %xmm4
	pshufd $0, %xmm4, %xmm4
	cmpq %rcx, %rdx
	jge _load_ascii_fw_r
_load_ascii_fw_f:
	movdqu (%rsi, %rdx), %xmm0
	movdqu 16(%rsi, %rdx), %xmm1
	movdqa %xmm0, %xmm2
	movdqa %xmm1, %xmm3
	psrlq $1, %xmm0
	psrlq $1, %xmm1
	psrlq $2, %xmm2
	psrlq $2, %xmm3
	pxor %xmm0, %xmm2
	pxor %xmm1, %xmm3
	pand %xmm4, %xmm2
	pand %xmm4, %xmm3
	movdqu %xmm2, (%rdi)
	movdqu %xmm3, 16(%rdi)
	ret
_load_ascii_fw_r:
	addq %rcx, %rcx
	subq %rdx, %rcx
	movdqa _load_ascii_rev(%rip), %xmm5
	movdqu (%rsi, %rcx), %xmm0
	movdqu 16(%rsi, %rcx), %xmm1
	movdqa %xmm0, %xmm2
	movdqa %xmm1, %xmm3
	psrlq $1, %xmm0
	psrlq $1, %xmm1
	psrlq $2, %xmm2
	psrlq $2, %xmm3
	pxor %xmm0, %xmm2
	pxor %xmm1, %xmm3
	pand %xmm4, %xmm2
	pand %xmm4, %xmm3
	pshufb %xmm5, %xmm2
	pshufb %xmm5, %xmm3
	movdqu %xmm2, 16(%rdi)
	movdqu %xmm3, (%rdi)
	ret
_load_ascii_fw_inv:
	shrq $16, %rdx
	movq %rdx, %xmm0
	pshufd $0, %xmm0, %xmm0
	movdqu %xmm0, (%rdi)
	movdqu %xmm0, 16(%rdi)
	ret

	.globl _load_ascii_fr
	.globl __load_ascii_fr
_load_ascii_fr:
__load_ascii_fr:
	cmpq $0, %rdx
	jl _load_ascii_fr_inv
	movl $0x03030303, %eax
	movq %rax, %xmm4
	pshufd $0, %xmm4, %xmm4
	cmpq %rcx, %rdx
	jge _load_ascii_fr_r
_load_ascii_fr_f:
	movdqu (%rdx, %rsi), %xmm0
	movdqu 16(%rdx, %rsi), %xmm1
	movdqa %xmm0, %xmm2
	movdqa %xmm1, %xmm3
	psrlq $1, %xmm0
	psrlq $1, %xmm1
	psrlq $2, %xmm2
	psrlq $2, %xmm3
	pxor %xmm0, %xmm2
	pxor %xmm1, %xmm3
	pand %xmm4, %xmm2
	pand %xmm4, %xmm3
	movdqu %xmm2, (%rdi)
	movdqu %xmm3, 16(%rdi)
	ret
_load_ascii_fr_r:
	movdqa _load_ascii_rev(%rip), %xmm5
	movdqu (%rdx, %rsi), %xmm0
	movdqu 16(%rdx, %rsi), %xmm1
	movdqa %xmm0, %xmm2
	movdqa %xmm1, %xmm3
	psrlq $1, %xmm0
	psrlq $1, %xmm1
	psrlq $2, %xmm2
	psrlq $2, %xmm3
	pxor %xmm0, %xmm2
	pxor %xmm1, %xmm3
	pand %xmm4, %xmm2
	pand %xmm4, %xmm3
	pxor %xmm4, %xmm2
	pxor %xmm4, %xmm3
	pshufb %xmm5, %xmm2
	pshufb %xmm5, %xmm3
	movdqu %xmm3, (%rdi)
	movdqu %xmm2, 16(%rdi)
	ret
_load_ascii_fr_inv:
	shrq $16, %rdx
	movq %rdx, %xmm0
	pshufd $0, %xmm0, %xmm0
	movdqu %xmm0, (%rdi)
	movdqu %xmm0, 16(%rdi)
	ret

	# 4bit
	# input:  %rdi (dst pointer (8bit array))
	#         %rsi (src pointer (4bit unpacked array))
	#         %rdx (index)
	#         %rcx (len)
	# output: %rax
	# work:   %r8, %xmm0, %xmm1, %xmm2
	.globl _load_4bit
	.globl __load_4bit
_load_4bit:
__load_4bit:
	movl $0x03030303, %eax
	movq %rax, %xmm2
	pshufd $0, %xmm2, %xmm2
_load_4bit_bulk_loop:
	cmpq $0, %rcx
	jle _load_4bit_ret
	movdqu (%rsi, %rdx), %xmm0
	movdqa %xmm0, %xmm1
	psrlq $3, %xmm0
	psrlq $1, %xmm1
	pand %xmm2, %xmm0
	psubb %xmm0, %xmm1
	pand %xmm2, %xmm1
	movdqu %xmm1, (%rdi)
	addq $16, %rdi
	addq $16, %rdx
	subq $16, %rcx
	jmp _load_4bit_bulk_loop
_load_4bit_ret:
	ret

	# 2bit
	# input:  %rdi (dst pointer (8bit array))
	#         %rsi (src pointer (2bit unpacked array))
	#         %rdx (index)
	#         %rcx (len)
	# output: %rax
	# work:   %r8, %xmm0, %xmm1, %xmm2
	.globl _load_2bit
	.globl __load_2bit
_load_2bit:
__load_2bit:
	movl $0x03030303, %eax
	movq %rax, %xmm2
	pshufd $0, %xmm2, %xmm2
_load_2bit_bulk_loop:
	cmpq $0, %rcx
	jle _load_2bit_end
	movdqu (%rsi, %rdx), %xmm0
	pand %xmm2, %xmm0
	movdqu %xmm0, (%rdi)
	addq $16, %rdi
	addq $16, %rdx
	subq $16, %rcx
	jmp _load_2bit_bulk_loop
_load_2bit_end:
	ret

	# 4bit8packed
	# input:  %rdi (dst pointer (8bit array))
	#         %rsi (src pointer (4bit8packed unpacked array))
	#         %rdx (index)
	#         %rcx (len)
	# output: %rax
	# work:   %r8, %xmm0, %xmm1, %xmm2
	.globl _load_4bit8packed
	.globl __load_4bit8packed
_load_4bit8packed:
__load_4bit8packed:
	movq %rcx, %rax
	movl $0x03030303, %r8d
	movq %r8, %xmm2
	pshufd $0, %xmm2, %xmm2
	movq %rdx, %rcx
	shrq $1, %rdx
	shlq $2, %rcx
	andq $4, %rcx
_load_4bit8packed_bulk_loop:
	cmpq $0, %rax
	jle _load_4bit8packed_ret
	# xorq %r8, %r8
	pxor %xmm0, %xmm0
	movq %rdi, %xmm1
	movq (%rsi, %rdx), %r8
	movq 8(%rsi, %rdx), %rdi
	shrdq %cl, %rdi, %r8
	# shrq %cl, %r8
	movq %r8, %xmm0
	movq %xmm1, %rdi
	pmovzxbw %xmm0, %xmm1
	psrlq $4, %xmm0
	pmovzxbw %xmm0, %xmm0
	pslldq $1, %xmm0
	por %xmm0, %xmm1
	movdqa %xmm1, %xmm0
	psrlq $2, %xmm1
	pand %xmm2, %xmm1
	psrlq $1, %xmm0
	psrlq $1, %xmm1
	psubb %xmm1, %xmm0
	pand %xmm2, %xmm0
	movdqu %xmm0, (%rdi)
	# shrq $2, %rcx
	addq $16, %rdi
	# subq %rcx, %rdi
	addq $8, %rdx
	subq $16, %rax
	# addq %rcx, %rax
	# xorq %rcx, %rcx
	jmp _load_4bit8packed_bulk_loop
_load_4bit8packed_ret:
	ret

	# 2bit8packed
	# input:  %rdi (dst pointer (8bit array))
	#         %rsi (src pointer (2bit8packed unpacked array))
	#         %rdx (index)
	#         %rcx (len)
	# output: %rax
	# work:   %r8, %xmm0, %xmm1, %xmm2
	.globl _load_2bit8packed
	.globl __load_2bit8packed
_load_2bit8packed:
__load_2bit8packed:
	movq %rcx, %rax
	movl $0x03030303, %r8d
	movq %r8, %xmm2
	pshufd $0, %xmm2, %xmm2
	movq %rdx, %rcx
	shrq $2, %rdx
	shlq $1, %rcx
	andq $6, %rcx
_load_2bit8packed_bulk_loop:
	cmpq $0, %rax
	jle _load_2bit8packed_ret
	# xorq %r8, %r8
	pxor %xmm0, %xmm0
	movq (%rsi, %rdx), %r8
	shrq %cl, %r8
	movq %r8, %xmm0
	pmovzxbd %xmm0, %xmm0
	movdqa %xmm0, %xmm1
	pslldq $1, %xmm0
	psrlq $2, %xmm0
	por %xmm0, %xmm1
	movdqa %xmm1, %xmm0
	pslldq $2, %xmm1
	psrlq $4, %xmm1
	por %xmm1, %xmm0
	pand %xmm2, %xmm0
	movdqu %xmm0, (%rdi)
	# shrq $1, %rcx
	addq $16, %rdi
	# subq %rcx, %rdi
	addq $4, %rdx
	subq $16, %rax
	# addq %rcx, %rax
	# xorq %rcx, %rcx
	jmp _load_2bit8packed_bulk_loop
_load_2bit8packed_ret:
	ret


# push functions

	# ascii reverse
	# input:  %rdi (pointer)
	#         %rsi (dst index)
	#         %rdx (src index)
	#         %rcx (char)
	# output: %rax
	# work:   None
	.globl _push_ascii_r
	.globl __push_ascii_r
_push_ascii_r:
__push_ascii_r:
	movl $0x4449584d, %edx	# 'D', 'I', 'X', 'M'
	shlq $3, %rcx
	shrq %cl, %rdx
	movb %dl, -1(%rdi, %rsi)
	movq %rsi, %rax
	addq $-1, %rax
	ret

	# ascii forward
	# input:  %rdi (pointer)
	#         %rsi (dst index)
	#         %rdx (src index)
	#         %rcx (char)
	# output: %rax
	# work:   None
	.globl _push_ascii_f
	.globl __push_ascii_f
_push_ascii_f:
__push_ascii_f:
	movl $0x4449584d, %edx	# 'D', 'I', 'X', 'M'
	shlq $3, %rcx
	shrq %cl, %rdx
	movb %dl, (%rdi, %rsi)
	movq %rsi, %rax
	addq $1, %rax
	ret

	# cigar reverse
	# input:  %rdi (pointer)
	#         %rsi (dst index)    (modified)
	#         %rdx (src index)    (modified)
	#         %rcx (char)     (modified)
	# output: %rax
	# work:   %r8
	.globl _push_cigar_r
	.globl __push_cigar_r
_push_cigar_r:
__push_cigar_r:
	shrq $2, %rdx		# make 4-byte aligned
	shlq $2, %rdx
	movq %rcx, %r8
	movl -4(%rdi, %rdx), %ecx
	shrq $29, %rcx
	cmpb %cl, %r8b
	jne __push_cigar_r_conv
	addl $1, -4(%rdi, %rdx)
	movq %rsi, %rax
	ret
__push_cigar_r_conv:
	movl $0x4449583d, %eax	# 'D', 'I', 'X', '='
	shlq $3, %rcx
	shrq %cl, %rax
	addq $-1, %rsi
	movb %al, (%rdi, %rsi)
	movl -4(%rdi, %rdx), %eax
	andq $0x1fffffff, %rax
	movq $10, %rcx
__push_cigar_r_loop:
	movq $0, %rdx
	divq %rcx
	addq $48, %rdx
	addq $-1, %rsi
	movb %dl, (%rdi, %rsi)
	cmpl $0, %eax
	jne __push_cigar_r_loop
	shlq $29, %r8
	addq $1, %r8
	movq %rsi, %rax
	shrq $2, %rsi
	shlq $2, %rsi
	movl %r8d, -4(%rdi, %rsi)
	ret

	# cigar forward
	# input:  %rdi (pointer)
	#         %rsi (dst index)    (modified)
	#         %rdx (src index)
	#         %rcx (char)
	# output: %rax
	.globl _push_cigar_f
	.globl __push_cigar_f
_push_cigar_f:
__push_cigar_f:
	movl -4(%rdi, %rsi), %r8d
#	movq %r8, %rdx
	shrq $29, %r8
	cmpb %r8b, %cl
	jne __push_cigar_f_next
	addq $1, -4(%rdi, %rsi)
	movq %rsi, %rax
	ret
__push_cigar_f_next:
	shlq $29, %rcx
	movq %rcx, %rdx
	addq $4, %rsi
	movl %edx, -4(%rdi, %rsi)
	movq %rsi, %rax
	ret

	# direction string reverse
	# input:  %rdi (pointer)
	#         %rsi (dst index)
	#         %rdx (src index)
	#         %rcx (char)
	# output: %rax
	# work:   %r8
	.globl _push_dir_r
	.globl __push_dir_r
_push_dir_r:
__push_dir_r:
	movq $0x0000010100010001, %r8
	shlq $4, %rcx
	shrq %cl, %r8
	movw %r8w, -2(%rdi, %rsi)
	shrq $5, %rcx
	leaq -2(%rsi, %rcx), %rax
	ret

	# direction string forward
	# input:  %rdi (pointer)
	#         %rsi (dst index)
	#         %rdx (src index)
	#         %rcx (char)
	# output: %rax
	# work:   %r8
	.globl _push_dir_f
	.globl __push_dir_f
_push_dir_f:
__push_dir_f:
	movq $0x0000010100010001, %r8
	shlq $4, %rcx
	shrq %cl, %r8
	movw %r8w, (%rdi, %rsi)
	shrq $5, %rcx
	negq %rcx
	leaq 2(%rsi, %rcx), %rax
	ret

#
# end of io.s
#
