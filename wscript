#! /usr/bin/env python
# encoding: utf-8

variants = ['naive', 'twig', 'branch', 'trunk', 'balloon']
cost = ['linear', 'affine']
dp = ['dynamic', 'guided']

cost_flag = {'linear': 'SEA_LINEAR_GAP_COST', 'affine': 'SEA_AFFINE_GAP_COST'}
dp_flag = {'dynamic': 'SEA_DYNAMIC', 'guided': 'SEA_GUIDED'}

def suffix(c, d):
	return('_' + c + '_' + d)

def options(opt):
	opt.load('compiler_c')

def configure(conf):
	conf.load('compiler_c')

	conf.recurse('util')
	conf.recurse('arch')
#	conf.recurse('variant')

	from itertools import product
	for (v, c, d) in product(variants, cost, dp):
		conf.env.append_value('OBJ', v + suffix(c, d))


def build(bld):

	bld.recurse('util')
	bld.recurse('arch')
#	bld.recurse('variant')

	from itertools import product
	for (v, c, d) in product(variants, cost, dp):
		bld.objects(
			source = 'dp.c',			# 'dp.c' + 'variant/naive_impl.h'
			target = v + suffix(c, d),	# 'naive_linear_dynamic'
			defines = [
				'BASE=' + v,
				'COST=' + cost_flag[c],
				'DP=' + dp_flag[d],
				'SUFFIX=' + suffix(c, d)])


	bld.shlib(
		source = 'sea.c',
		target = 'sea',
		use = bld.env.OBJ)

