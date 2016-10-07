	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 11
	.globl	_print_usage
	.align	4, 0x90
_print_usage:                           ## @print_usage
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp0:
	.cfi_def_cfa_offset 16
Ltmp1:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp2:
	.cfi_def_cfa_register %rbp
	movq	___stderrp@GOTPCREL(%rip), %rax
	movq	(%rax), %rcx
	leaq	L_.str(%rip), %rdi
	movl	$66, %esi
	movl	$1, %edx
	popq	%rbp
	jmp	_fwrite                 ## TAILCALL
	.cfi_endproc

	.globl	_random_base
	.align	4, 0x90
_random_base:                           ## @random_base
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp3:
	.cfi_def_cfa_offset 16
Ltmp4:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp5:
	.cfi_def_cfa_register %rbp
	callq	_rand
	movl	%eax, %ecx
	sarl	$31, %ecx
	shrl	$30, %ecx
	addl	%eax, %ecx
	andl	$-4, %ecx
	subl	%ecx, %eax
	cltq
	leaq	_random_base.table(%rip), %rcx
	movsbl	(%rax,%rcx), %eax
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_generate_random_sequence
	.align	4, 0x90
_generate_random_sequence:              ## @generate_random_sequence
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp6:
	.cfi_def_cfa_offset 16
Ltmp7:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp8:
	.cfi_def_cfa_register %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	pushq	%rax
Ltmp9:
	.cfi_offset %rbx, -56
Ltmp10:
	.cfi_offset %r12, -48
Ltmp11:
	.cfi_offset %r13, -40
Ltmp12:
	.cfi_offset %r14, -32
Ltmp13:
	.cfi_offset %r15, -24
	movl	%edi, %r14d
	leal	33(%r14), %eax
	movslq	%eax, %rdi
	callq	_malloc
	movq	%rax, %r15
	xorl	%eax, %eax
	testq	%r15, %r15
	je	LBB2_5
## BB#1:                                ## %.preheader
	testl	%r14d, %r14d
	jle	LBB2_4
## BB#2:
	leaq	_random_base.table(%rip), %r12
	movl	%r14d, %r13d
	movq	%r15, %rbx
	.align	4, 0x90
LBB2_3:                                 ## %.lr.ph
                                        ## =>This Inner Loop Header: Depth=1
	callq	_rand
	movl	%eax, %ecx
	sarl	$31, %ecx
	shrl	$30, %ecx
	addl	%eax, %ecx
	andl	$-4, %ecx
	subl	%ecx, %eax
	cltq
	movb	(%rax,%r12), %al
	movb	%al, (%rbx)
	incq	%rbx
	decl	%r13d
	jne	LBB2_3
LBB2_4:                                 ## %._crit_edge
	movslq	%r14d, %rax
	movb	$0, (%r15,%rax)
	movq	%r15, %rax
LBB2_5:
	addq	$8, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
	.cfi_endproc

	.section	__TEXT,__literal8,8byte_literals
	.align	3
LCPI3_0:
	.quad	4746794007244308480     ## double 2147483647
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_generate_mutated_sequence
	.align	4, 0x90
_generate_mutated_sequence:             ## @generate_mutated_sequence
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp14:
	.cfi_def_cfa_offset 16
Ltmp15:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp16:
	.cfi_def_cfa_register %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$56, %rsp
Ltmp17:
	.cfi_offset %rbx, -56
Ltmp18:
	.cfi_offset %r12, -48
Ltmp19:
	.cfi_offset %r13, -40
Ltmp20:
	.cfi_offset %r14, -32
Ltmp21:
	.cfi_offset %r15, -24
	movl	%edx, %r14d
	movsd	%xmm1, -64(%rbp)        ## 8-byte Spill
	movsd	%xmm0, -56(%rbp)        ## 8-byte Spill
	movl	%esi, %ebx
	movq	%rdi, -80(%rbp)         ## 8-byte Spill
	xorl	%eax, %eax
	testq	%rdi, %rdi
	movl	%ebx, %r13d
	je	LBB3_21
## BB#1:
	leal	33(%rbx), %eax
	movslq	%eax, %rdi
	callq	_malloc
	movq	%rax, %r15
	xorl	%eax, %eax
	testq	%r15, %r15
	je	LBB3_21
## BB#2:                                ## %.preheader
	testl	%ebx, %ebx
	movq	%rbx, -72(%rbp)         ## 8-byte Spill
	jle	LBB3_20
## BB#3:                                ## %.lr.ph
	movl	$1, %eax
	subl	%r14d, %eax
	movl	%eax, -84(%rbp)         ## 4-byte Spill
	addl	$-2, %r14d
	movl	%r14d, -88(%rbp)        ## 4-byte Spill
	xorl	%r14d, %r14d
	xorl	%ebx, %ebx
	xorl	%r12d, %r12d
	.align	4, 0x90
LBB3_4:                                 ## =>This Inner Loop Header: Depth=1
	callq	_rand
	cvtsi2sdl	%eax, %xmm1
	movsd	LCPI3_0(%rip), %xmm0    ## xmm0 = mem[0],zero
	divsd	%xmm0, %xmm1
	movsd	%xmm1, -48(%rbp)        ## 8-byte Spill
	callq	_rand
	movsd	-56(%rbp), %xmm0        ## 8-byte Reload
                                        ## xmm0 = mem[0],zero
	ucomisd	-48(%rbp), %xmm0        ## 8-byte Folded Reload
	jbe	LBB3_6
## BB#5:                                ##   in Loop: Header=BB3_4 Depth=1
	movl	%eax, %ecx
	sarl	$31, %ecx
	shrl	$30, %ecx
	addl	%eax, %ecx
	andl	$-4, %ecx
	subl	%ecx, %eax
	cltq
	leaq	_random_base.table(%rip), %rcx
	movb	(%rax,%rcx), %al
	movb	%al, (%r15,%r14)
	incl	%r12d
	jmp	LBB3_19
	.align	4, 0x90
LBB3_6:                                 ##   in Loop: Header=BB3_4 Depth=1
	xorps	%xmm0, %xmm0
	cvtsi2sdl	%eax, %xmm0
	divsd	LCPI3_0(%rip), %xmm0
	movsd	-64(%rbp), %xmm1        ## 8-byte Reload
                                        ## xmm1 = mem[0],zero
	ucomisd	%xmm0, %xmm1
	jbe	LBB3_15
## BB#7:                                ##   in Loop: Header=BB3_4 Depth=1
	callq	_rand
	cmpl	-84(%rbp), %ebx         ## 4-byte Folded Reload
	jle	LBB3_13
## BB#8:                                ##   in Loop: Header=BB3_4 Depth=1
	andl	$1, %eax
	je	LBB3_13
## BB#9:                                ##   in Loop: Header=BB3_4 Depth=1
	cmpl	%r13d, %r12d
	jge	LBB3_11
## BB#10:                               ##   in Loop: Header=BB3_4 Depth=1
	movslq	%r12d, %rax
	incl	%r12d
	addq	-80(%rbp), %rax         ## 8-byte Folded Reload
	jmp	LBB3_12
LBB3_13:                                ##   in Loop: Header=BB3_4 Depth=1
	cmpl	-88(%rbp), %ebx         ## 4-byte Folded Reload
	jge	LBB3_15
## BB#14:                               ##   in Loop: Header=BB3_4 Depth=1
	callq	_rand
	movl	%eax, %ecx
	sarl	$31, %ecx
	shrl	$30, %ecx
	addl	%eax, %ecx
	andl	$-4, %ecx
	subl	%ecx, %eax
	cltq
	leaq	_random_base.table(%rip), %rcx
	movb	(%rax,%rcx), %al
	movb	%al, (%r15,%r14)
	incl	%ebx
	jmp	LBB3_19
	.align	4, 0x90
LBB3_15:                                ##   in Loop: Header=BB3_4 Depth=1
	movq	-72(%rbp), %rax         ## 8-byte Reload
	cmpl	%eax, %r12d
	jge	LBB3_17
## BB#16:                               ##   in Loop: Header=BB3_4 Depth=1
	movslq	%r12d, %rax
	incl	%r12d
	addq	-80(%rbp), %rax         ## 8-byte Folded Reload
	jmp	LBB3_18
LBB3_17:                                ##   in Loop: Header=BB3_4 Depth=1
	callq	_rand
	movl	%eax, %ecx
	sarl	$31, %ecx
	shrl	$30, %ecx
	addl	%eax, %ecx
	andl	$-4, %ecx
	subl	%ecx, %eax
	cltq
	leaq	_random_base.table(%rip), %rcx
	addq	%rcx, %rax
LBB3_18:                                ##   in Loop: Header=BB3_4 Depth=1
	movb	(%rax), %al
	movb	%al, (%r15,%r14)
	jmp	LBB3_19
LBB3_11:                                ##   in Loop: Header=BB3_4 Depth=1
	callq	_rand
	movl	%eax, %ecx
	sarl	$31, %ecx
	shrl	$30, %ecx
	addl	%eax, %ecx
	andl	$-4, %ecx
	subl	%ecx, %eax
	cltq
	leaq	_random_base.table(%rip), %rcx
	addq	%rcx, %rax
LBB3_12:                                ##   in Loop: Header=BB3_4 Depth=1
	movb	(%rax), %al
	movb	%al, (%r15,%r14)
	incl	%r12d
	decl	%ebx
	.align	4, 0x90
LBB3_19:                                ##   in Loop: Header=BB3_4 Depth=1
	incq	%r14
	cmpl	%r14d, %r13d
	jne	LBB3_4
LBB3_20:                                ## %._crit_edge
	movq	-72(%rbp), %rax         ## 8-byte Reload
	cltq
	movb	$0, (%r15,%rax)
	movq	%r15, %rax
LBB3_21:
	addq	$56, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
	.cfi_endproc

	.globl	_parse_args
	.align	4, 0x90
_parse_args:                            ## @parse_args
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp22:
	.cfi_def_cfa_offset 16
Ltmp23:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp24:
	.cfi_def_cfa_register %rbp
	pushq	%rbx
	pushq	%rax
Ltmp25:
	.cfi_offset %rbx, -24
	movq	%rdi, %rbx
	cmpl	$99, %esi
	jle	LBB4_1
## BB#4:
	cmpl	$100, %esi
	je	LBB4_9
## BB#5:
	cmpl	$120, %esi
	je	LBB4_8
## BB#6:
	cmpl	$108, %esi
	jne	LBB4_11
## BB#7:
	movq	%rdx, %rdi
	callq	_atoi
	cltq
	movq	%rax, (%rbx)
	xorl	%eax, %eax
	jmp	LBB4_12
LBB4_1:
	cmpl	$97, %esi
	je	LBB4_10
## BB#2:
	cmpl	$99, %esi
	jne	LBB4_11
## BB#3:
	movq	%rdx, %rdi
	callq	_atoi
	cltq
	movq	%rax, 8(%rbx)
	xorl	%eax, %eax
	jmp	LBB4_12
LBB4_9:
	movq	%rdx, %rdi
	callq	_atof
	movsd	%xmm0, 24(%rbx)
	xorl	%eax, %eax
	jmp	LBB4_12
LBB4_8:
	movq	%rdx, %rdi
	callq	_atof
	movsd	%xmm0, 16(%rbx)
	xorl	%eax, %eax
	jmp	LBB4_12
LBB4_10:
	movq	%rdx, %rdi
	callq	_puts
	xorl	%eax, %eax
	jmp	LBB4_12
LBB4_11:
	movq	___stderrp@GOTPCREL(%rip), %rax
	movq	(%rax), %rcx
	leaq	L_.str(%rip), %rdi
	movl	$66, %esi
	movl	$1, %edx
	callq	_fwrite
	movl	$-1, %eax
LBB4_12:
	addq	$8, %rsp
	popq	%rbx
	popq	%rbp
	retq
	.cfi_endproc

	.section	__TEXT,__literal8,8byte_literals
	.align	3
LCPI5_0:
	.quad	4591870180066957722     ## double 0.10000000000000001
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_main
	.align	4, 0x90
_main:                                  ## @main
	.cfi_startproc
## BB#0:
	pushq	%rbp
Ltmp26:
	.cfi_def_cfa_offset 16
Ltmp27:
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
Ltmp28:
	.cfi_def_cfa_register %rbp
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	subq	$152, %rsp
Ltmp29:
	.cfi_offset %rbx, -56
Ltmp30:
	.cfi_offset %r12, -48
Ltmp31:
	.cfi_offset %r13, -40
Ltmp32:
	.cfi_offset %r14, -32
Ltmp33:
	.cfi_offset %r15, -24
	movq	%rsi, %r15
	movl	%edi, %r12d
	leaq	L_.str.2(%rip), %rdx
	callq	_getopt
	movsd	LCPI5_0(%rip), %xmm0    ## xmm0 = mem[0],zero
	movl	$10000, %r14d           ## imm = 0x2710
	cmpl	$-1, %eax
	je	LBB5_1
## BB#11:
	leaq	L_.str.2(%rip), %rbx
	movl	$10000, %r13d           ## imm = 0x2710
	movsd	%xmm0, -168(%rbp)       ## 8-byte Spill
	movsd	%xmm0, -160(%rbp)       ## 8-byte Spill
	.align	4, 0x90
LBB5_12:                                ## =>This Inner Loop Header: Depth=1
	movq	_optarg@GOTPCREL(%rip), %rcx
	movq	(%rcx), %rdi
	cmpl	$99, %eax
	jle	LBB5_13
## BB#16:                               ##   in Loop: Header=BB5_12 Depth=1
	cmpl	$100, %eax
	je	LBB5_24
## BB#17:                               ##   in Loop: Header=BB5_12 Depth=1
	cmpl	$120, %eax
	jne	LBB5_18
## BB#20:                               ## %parse_args.exit.thread.outer3
                                        ##   in Loop: Header=BB5_12 Depth=1
	callq	_atof
	movsd	%xmm0, -160(%rbp)       ## 8-byte Spill
	jmp	LBB5_22
	.align	4, 0x90
LBB5_13:                                ##   in Loop: Header=BB5_12 Depth=1
	cmpl	$97, %eax
	jne	LBB5_14
## BB#21:                               ## %parse_args.exit.thread
                                        ##   in Loop: Header=BB5_12 Depth=1
	callq	_puts
	jmp	LBB5_22
	.align	4, 0x90
LBB5_24:                                ## %parse_args.exit.thread.outer
                                        ##   in Loop: Header=BB5_12 Depth=1
	callq	_atof
	movsd	%xmm0, -168(%rbp)       ## 8-byte Spill
	jmp	LBB5_22
	.align	4, 0x90
LBB5_18:                                ##   in Loop: Header=BB5_12 Depth=1
	cmpl	$108, %eax
	jne	LBB5_23
## BB#19:                               ## %parse_args.exit.thread.outer10
                                        ##   in Loop: Header=BB5_12 Depth=1
	callq	_atoi
	movslq	%eax, %r14
	jmp	LBB5_22
	.align	4, 0x90
LBB5_14:                                ##   in Loop: Header=BB5_12 Depth=1
	cmpl	$99, %eax
	jne	LBB5_23
## BB#15:                               ## %parse_args.exit.thread.outer7
                                        ##   in Loop: Header=BB5_12 Depth=1
	callq	_atoi
	movslq	%eax, %r13
	.align	4, 0x90
LBB5_22:                                ## %parse_args.exit.thread
                                        ##   in Loop: Header=BB5_12 Depth=1
	movl	%r12d, %edi
	movq	%r15, %rsi
	movq	%rbx, %rdx
	callq	_getopt
	cmpl	$-1, %eax
	jne	LBB5_12
	jmp	LBB5_2
LBB5_1:
	movsd	%xmm0, -168(%rbp)       ## 8-byte Spill
	movsd	%xmm0, -160(%rbp)       ## 8-byte Spill
	movl	$10000, %r13d           ## imm = 0x2710
LBB5_2:                                 ## %parse_args.exit.thread.outer10._crit_edge
	movq	___stderrp@GOTPCREL(%rip), %rax
	movq	(%rax), %rdi
	leaq	L_.str.3(%rip), %rsi
	movb	$2, %al
	movq	%r14, %rdx
	movsd	-160(%rbp), %xmm0       ## 8-byte Reload
                                        ## xmm0 = mem[0],zero
	movsd	-168(%rbp), %xmm1       ## 8-byte Reload
                                        ## xmm1 = mem[0],zero
	movq	%r13, %rcx
	callq	_fprintf
	movq	%r14, %r12
	shlq	$32, %r12
	movabsq	$141733920768, %rdi     ## imm = 0x2100000000
	addq	%r12, %rdi
	sarq	$32, %rdi
	callq	_malloc
	movq	%rax, -176(%rbp)        ## 8-byte Spill
	xorl	%ebx, %ebx
	testq	%rax, %rax
	je	LBB5_7
## BB#3:                                ## %.preheader.i
	testl	%r14d, %r14d
	movq	%r14, -184(%rbp)        ## 8-byte Spill
	jle	LBB5_6
## BB#4:                                ## %.lr.ph.i.preheader
	leaq	_random_base.table(%rip), %rbx
	movq	-176(%rbp), %r15        ## 8-byte Reload
	movq	-184(%rbp), %rax        ## 8-byte Reload
	movl	%eax, %r14d
	.align	4, 0x90
LBB5_5:                                 ## %.lr.ph.i
                                        ## =>This Inner Loop Header: Depth=1
	callq	_rand
	movl	%eax, %ecx
	sarl	$31, %ecx
	shrl	$30, %ecx
	addl	%eax, %ecx
	andl	$-4, %ecx
	subl	%ecx, %eax
	cltq
	movb	(%rax,%rbx), %al
	movb	%al, (%r15)
	incq	%r15
	decl	%r14d
	jne	LBB5_5
LBB5_6:                                 ## %._crit_edge.i
	sarq	$32, %r12
	movq	-176(%rbp), %rax        ## 8-byte Reload
	movb	$0, (%rax,%r12)
	movq	%rax, %rbx
	movq	-184(%rbp), %r14        ## 8-byte Reload
LBB5_7:                                 ## %generate_random_sequence.exit
	movq	%rbx, -176(%rbp)        ## 8-byte Spill
	movl	$8, %edx
	movq	%rbx, %rdi
	movl	%r14d, %esi
	movsd	-160(%rbp), %xmm0       ## 8-byte Reload
                                        ## xmm0 = mem[0],zero
	movsd	-168(%rbp), %xmm1       ## 8-byte Reload
                                        ## xmm1 = mem[0],zero
	callq	_generate_mutated_sequence
	movq	%rax, %r14
	movq	%r14, -168(%rbp)        ## 8-byte Spill
	movw	$0, -80(%rbp)
	movw	$0, -78(%rbp)
	movw	$0, -76(%rbp)
	movw	$100, -74(%rbp)
	movb	$2, -104(%rbp)
	movl	$-33686019, -103(%rbp)  ## imm = 0xFFFFFFFFFDFDFDFD
	movb	$2, -99(%rbp)
	movl	$-33686019, -98(%rbp)   ## imm = 0xFFFFFFFFFDFDFDFD
	movb	$2, -94(%rbp)
	movl	$-33686019, -93(%rbp)   ## imm = 0xFFFFFFFFFDFDFDFD
	movb	$2, -89(%rbp)
	movb	$5, -88(%rbp)
	movb	$1, -87(%rbp)
	movb	$5, -86(%rbp)
	movb	$1, -85(%rbp)
	leaq	-104(%rbp), %rax
	movq	%rax, -72(%rbp)
	leaq	-80(%rbp), %rdi
	callq	_gaba_init
	movq	%rax, %r12
	movq	%r12, -160(%rbp)        ## 8-byte Spill
	movl	$0, -120(%rbp)
	movq	%rbx, %rdi
	callq	_strlen
	movl	%eax, -116(%rbp)
	movq	%rbx, -112(%rbp)
	movl	$2, -136(%rbp)
	movq	%r14, %rdi
	callq	_strlen
	movl	%eax, -132(%rbp)
	movq	%r14, -128(%rbp)
	xorpd	%xmm0, %xmm0
	movapd	%xmm0, -64(%rbp)
	movq	$0, -48(%rbp)
	xorl	%r15d, %r15d
	testq	%r13, %r13
	movl	$0, %ebx
	jle	LBB5_10
## BB#8:                                ## %.lr.ph
	movabsq	$140737488355328, %r14  ## imm = 0x800000000000
	xorl	%ebx, %ebx
	.align	4, 0x90
LBB5_9:                                 ## =>This Inner Loop Header: Depth=1
	movq	%r12, %rdi
	movq	%r14, %rsi
	movq	%r14, %rdx
	callq	_gaba_dp_init
	movq	%rax, %r12
	xorl	%esi, %esi
	leaq	-64(%rbp), %rdi
	callq	_gettimeofday
	xorl	%edx, %edx
	xorl	%r8d, %r8d
	movq	%r12, %rdi
	leaq	-120(%rbp), %rsi
	leaq	-136(%rbp), %rcx
	callq	_gaba_dp_fill_root
	addq	16(%rax), %rbx
	xorl	%esi, %esi
	leaq	-152(%rbp), %rdi
	callq	_gettimeofday
	movq	-152(%rbp), %rax
	subq	-64(%rbp), %rax
	imulq	$1000000000, %rax, %r15 ## imm = 0x3B9ACA00
	movslq	-144(%rbp), %rax
	movslq	-56(%rbp), %rcx
	subq	%rcx, %rax
	imulq	$1000, %rax, %rax       ## imm = 0x3E8
	addq	-48(%rbp), %r15
	addq	%rax, %r15
	movq	%r15, -48(%rbp)
	movq	%r12, %rdi
	movq	-160(%rbp), %r12        ## 8-byte Reload
	callq	_gaba_dp_clean
	decq	%r13
	jne	LBB5_9
LBB5_10:                                ## %._crit_edge
	leaq	L_.str.4(%rip), %rdi
	xorl	%eax, %eax
	movq	%r15, %rsi
	movq	%rbx, %rdx
	callq	_printf
	movq	-176(%rbp), %rdi        ## 8-byte Reload
	callq	_free
	movq	-168(%rbp), %rdi        ## 8-byte Reload
	callq	_free
	movq	%r12, %rdi
	callq	_gaba_clean
	xorl	%eax, %eax
	addq	$152, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
LBB5_23:
	movq	___stderrp@GOTPCREL(%rip), %rax
	movq	(%rax), %rcx
	leaq	L_.str(%rip), %rdi
	movl	$66, %esi
	movl	$1, %edx
	callq	_fwrite
	movl	$1, %edi
	callq	_exit
	.cfi_endproc

	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"usage: bench -l <len> -c <cnt> -x <mismatch rate> -d <indel rate>\n"

	.section	__TEXT,__const
_random_base.table:                     ## @random_base.table
	.ascii	"\001\002\004\b"

	.section	__TEXT,__cstring,cstring_literals
L_.str.2:                               ## @.str.2
	.asciz	"q:t:o:l:x:d:c:a:seb:h"

L_.str.3:                               ## @.str.3
	.asciz	"len\t%lld\ncnt\t%lld\nx\t%f\nd\t%f\n"

L_.str.4:                               ## @.str.4
	.asciz	"%lld\t%lld\n"


.subsections_via_symbols
