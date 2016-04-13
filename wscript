#! /usr/bin/env python
# encoding: utf-8

variants = ['twig', 'branch'] #, 'trunk'] #, 'naive', 'balloon', 'cap']
costs = ['linear'] #, 'affine']
dps = ['dynamic', 'guided']

def suffix(c, d):
	return('_' + c + '_' + d)

def options(opt):
	opt.load('compiler_c')

	opt.add_option('', '--debug',
		action = 'store_true',
		dest = 'debug',
		help = 'debug build')


def configure(conf):
	conf.load('ar')
	conf.load('compiler_c')

	# conf.recurse('util')
	conf.recurse('arch')

	# debug options
	if conf.options.debug is True:
		conf.env.append_value('CFLAGS', '-fmacro-backtrace-limit=0')
		conf.env.append_value('CFLAGS', '-g')
		conf.env.append_value('CFLAGS', '-DDEBUG')
	else:
		# conf.env.append_value('CFLAGS', '-g')
		# conf.env.append_value('CFLAGS', '-DDEBUG')
		conf.env.append_value('CFLAGS', '-O3')

	# conf.env.append_value('CFLAGS', '-DBENCH')
	conf.env.append_value('CFLAGS', '-Wall')
	conf.env.append_value('CFLAGS', '-std=c99')
	# conf.env.append_value('CFLAGS', '-D_POSIX_C_SOURCE=200112L')	# for posix_memalign and clock_gettime
	conf.env.append_value('CFLAGS', '-fPIC')

	if conf.env.CC_NAME == 'icc':
		# FIXME: dirty hack to pass '-diag-disable remark', current waf does not support space-separated options
		# note: https://groups.google.com/forum/#!topic/waf-users/TJKhe04HGQc
		conf.env.append_value('CFLAGS', '-diag-disable')
		conf.env.append_value('CFLAGS', 'remark')
	elif conf.env.CC_NAME == 'gcc':
		conf.env.append_value('CFLAGS', '-Wno-unused-variable')
		conf.env.append_value('CFLAGS', '-Wno-unused-but-set-variable')
		conf.env.append_value('CFLAGS', '-Wno-unused-result')
		conf.env.append_value('CFLAGS', '-Wno-format')				# ignore incompatibility between unsigned long long and int64_t
	elif conf.env.CC_NAME == 'clang':
		conf.env.append_value('CFLAGS', '-Wno-unused-variable')
	else:
		pass


def build(bld):

	# bld.recurse('util')
	bld.recurse('arch')

#	bld.shlib(
#		source = 'gaba.c',
#		target = 'gaba',
#		use = bld.env.OBJ)

	bld.stlib(
		source = 'gaba.c',
		target = 'gaba',
		includes = 'util')

	bld.program(
		source = ['gaba.c', 'arch/io.s'],
		target = 'unittest',
		includes = 'util',
		defines = ['TEST'])

	bld.program(
		source = ['bench.c', 'gaba.c', 'arch/io.s'],
		target = 'bench',
		includes = 'util',
		defines = ['BENCH'])
