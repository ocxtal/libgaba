#! /usr/bin/env python
# encoding: utf-8


variants = ['naive', 'twig', 'branch', 'trunk', 'balloon']


def options(opt):
	opt.load('compiler_c')

def configure(conf):
	conf.load('compiler_c')

	conf.recurse('util')
	conf.recurse('arch')
#	conf.recurse('variant')

	for v in variants:
		conf.env.append_value('OBJ', v)


def build(bld):

	bld.recurse('util')
	bld.recurse('arch')
#	bld.recurse('variant')

	for v in variants:
		bld.objects(
			source = 'dp.c',		# 'dp.c' + 'variant/naive_impl.h'
			target = v,				# 'naive'
			defines = ['BASE=' + v, 'SUFFIX='])


	bld.shlib(
		source = 'sea.c',
		target = 'sea',
		use = bld.env.OBJ)

