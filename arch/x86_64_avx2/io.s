
#
# @file io.s
#
# @brief sequence reader / alignment writer implementation for x86_64.
#

	.text

	.align 16
_bswapdq_idx:
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
	# work:   %ymm0, %ymm1, %ymm2
	.globl _loada_ascii_2bit_fw
	.globl __loada_ascii_2bit_fw
	# ASCII -> 2bit, load to lower 2bit, forward only seq.
_loada_ascii_2bit_fw:
__loada_ascii_2bit_fw:
	cmpq $0, %rsi		# check NULL
	jl _loada_ascii_2bit_fw_null
	movb $0x03, %al
	movq %rax, %xmm4
	vpbroadcastb %xmm4, %ymm4
	cmpq %rcx, %rdx
	jge _loada_ascii_2bit_fw_r
_loada_ascii_2bit_fw_f:
	vmovdqu (%rsi, %rdx), %ymm0
	vpsrlq $1, %ymm0, %ymm1
	vpsrlq $2, %ymm0, %ymm2
	vpxor %ymm1, %ymm2, %ymm2
	vpandn %ymm4, %ymm2, %ymm2
	vmovdqu %ymm2, (%rdi)
	ret
_loada_ascii_2bit_fw_r:
	vbroadcasti128 _bswapdq_idx(%rip), %ymm5
	addq %rcx, %rcx
	subq $32, %r8
	subq %rdx, %rcx
	addq %r8, %rcx
	vmovdqu (%rsi, %rcx), %ymm0
	vpsrlq $1, %ymm0, %ymm1
	vpsrlq $2, %ymm0, %ymm2
	vpxor %ymm1, %ymm2, %ymm2
	vpand %ymm4, %ymm2, %ymm2
	vpshufb %ymm5, %ymm2, %ymm2
	vperm2i128 $1, %ymm2, %ymm2, %ymm2
	vmovdqu %ymm2, (%rdi)
	ret
_loada_ascii_2bit_fw_null:
	movb $0x80, %al
	movq %rax, %xmm4
	vpbroadcastb %xmm4, %ymm4
	vmovdqu %ymm0, (%rdi)
	ret

	.globl _loada_ascii_2bit_fr
	.globl __loada_ascii_2bit_fr
	# ASCII -> 2bit, load to lower 2bit, forward-reverse seq.
_loada_ascii_2bit_fr:
__loada_ascii_2bit_fr:
	cmpq $0, %rsi
	jl _loada_ascii_2bit_fr_null
	movb $0x03, %al
	movq %rax, %xmm4
	vpbroadcastb %xmm4, %ymm4
	vmovdqu (%rsi, %rdx), %ymm0
	vpsrlq $1, %ymm0, %ymm1
	vpsrlq $2, %ymm0, %ymm2
	vpxor %ymm1, %ymm2, %ymm2
	vpandn %ymm4, %ymm2, %ymm2
	vmovdqu %ymm2, (%rdi)
	ret
_loada_ascii_2bit_fr_null:
	movb $0x80, %al
	movq %rax, %xmm0
	vpbroadcastb %xmm0, %ymm0
	vmovdqu %ymm0, (%rdi)
	ret

	.globl _loadb_ascii_2bit_fw
	.globl __loadb_ascii_2bit_fw
	# ASCII -> 2bit, load to upper 2bit, forward only seq.
_loadb_ascii_2bit_fw:
__loadb_ascii_2bit_fw:
	cmpq $0, %rsi		# check NULL
	jl _loadb_ascii_2bit_fw_null
	movb $0x0c, %al
	movq %rax, %xmm4
	vpbroadcastb %xmm4, %ymm4
	cmpq %rcx, %rdx
	jge _loadb_ascii_2bit_fw_r
_loadb_ascii_2bit_fw_f:
	vmovdqu (%rsi, %rdx), %ymm0
	vpsllq $1, %ymm0, %ymm1
	vpxor %ymm0, %ymm1, %ymm2
	vpand %ymm4, %ymm2, %ymm2
	vmovdqu %ymm2, (%rdi)
	ret
_loadb_ascii_2bit_fw_r:
	vbroadcasti128 _bswapdq_idx(%rip), %ymm5
	addq %rcx, %rcx
	subq $32, %r8
	subq %rdx, %rcx
	addq %r8, %rcx
	vmovdqu (%rsi, %rcx), %ymm0
	vpsllq $1, %ymm0, %ymm1
	vpxor %ymm0, %ymm1, %ymm2
	vpandn %ymm4, %ymm2, %ymm2
	vpshufb %ymm5, %ymm2, %ymm2
	vperm2i128 $1, %ymm2, %ymm2, %ymm2
	vmovdqu %ymm2, (%rdi)
	ret
_loadb_ascii_2bit_fw_null:
	movb $0x80, %al
	movq %rax, %xmm0
	vpbroadcastb %xmm0, %ymm0
	vmovdqu %ymm0, (%rdi)
	ret

	.globl _loadb_ascii_2bit_fr
	.globl __loadb_ascii_2bit_fr
	# ASCII -> 2bit, load to upper 2bit, forward-reverse seq.
_loadb_ascii_2bit_fr:
__loadb_ascii_2bit_fr:
	cmpq $0, %rsi
	jl _loadb_ascii_2bit_fr_null
	movb $0x0c, %al
	movq %rax, %xmm4
	vpbroadcastb %xmm4, %ymm4
	vmovdqu (%rdx, %rsi), %ymm0
	vpsllq $1, %ymm0, %ymm1
	vpxor %ymm0, %ymm1, %ymm2
	vpand %ymm4, %ymm2, %ymm2
	vmovdqu %ymm2, (%rdi)
	ret
_loadb_ascii_2bit_fr_null:
	movb $0x80, %al
	movq %rax, %xmm0
	vpbroadcastb %xmm0, %ymm0
	vmovdqu %ymm0, (%rdi)
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
