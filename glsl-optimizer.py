# glsl-optimizer.py
# http://github.com/aras-p/glsl-optimizer
# Copyright (c) 2010 Sunside Inc., All Rights Reserved
# Author: Joe Riedel
# See Radiance/LICENSE for licensing terms.

Import('radvars')
Import('variant_dir')
(build, source) = radvars

version = 'v1'
path = './Extern/glsl-optimizer/' + version

x = build.libBuilder('glsl-optimizer', build, source, path, variant_dir)

build.backend.addIncludePath(x.source, 
	[
		build.absPath(path + '/include'),
		build.absPath(path + '/include/c99'),
		build.absPath(path + '/src/mesa'),
		build.absPath(path + '/src/mapi'),
		build.absPath(path + '/src/talloc')
	])

if build.win():
	build.backend.addDefine(x.source, ['NOMINMAX', 'snprintf=_snprintf'])
	build.backend.addCCFlag(x.source, ['/wd4267', '/wd4065', '/wd4244', '/wd4005', '/wd4090', '/wd4996', '/wd4101', '/wd4291', '/wd4018', '/wd4800', '/wd4099', '/wd4146'])
	build.backend.addCCFlag(x.source, ['/wd4311', '/wd4312']) # these disable warnings about casts to larger sizes, this is a 64bit portability warning.
	
x.add('src/talloc/talloc.c')
x.add('src/mesa/program/hash_table.c')
x.add('src/mesa/program/symbol_table.c')
x.add('src/glsl/glcpp/glcpp-lex.c')
x.add('src/glsl/glcpp/glcpp-parse.c')
x.add('src/glsl/glcpp/pp.c')
x.add('src/glsl/ast_expr.cpp')
x.add('src/glsl/ast_function.cpp')
x.add('src/glsl/ast_to_hir.cpp')
x.add('src/glsl/ast_type.cpp')
x.add('src/glsl/builtin_function.cpp')
x.add('src/glsl/glsl_lexer.cpp')
x.add('src/glsl/glsl_optimizer.cpp')
x.add('src/glsl/glsl_parser.cpp')
x.add('src/glsl/glsl_parser_extras.cpp')
x.add('src/glsl/glsl_symbol_table.cpp')
x.add('src/glsl/glsl_types.cpp')
x.add('src/glsl/hir_field_selection.cpp')
x.add('src/glsl/ir.cpp')
x.add('src/glsl/ir_algebraic.cpp')
x.add('src/glsl/ir_basic_block.cpp')
x.add('src/glsl/ir_clone.cpp')
x.add('src/glsl/ir_constant_expression.cpp')
x.add('src/glsl/ir_constant_folding.cpp')
x.add('src/glsl/ir_constant_propagation.cpp')
x.add('src/glsl/ir_constant_variable.cpp')
x.add('src/glsl/ir_copy_propagation.cpp')
x.add('src/glsl/ir_dead_code.cpp')
x.add('src/glsl/ir_dead_code_local.cpp')
x.add('src/glsl/ir_dead_functions.cpp')
x.add('src/glsl/ir_div_to_mul_rcp.cpp')
x.add('src/glsl/ir_expression_flattening.cpp')
x.add('src/glsl/ir_function.cpp')
x.add('src/glsl/ir_function_can_inline.cpp')
x.add('src/glsl/ir_function_inlining.cpp')
x.add('src/glsl/ir_hierarchical_visitor.cpp')
x.add('src/glsl/ir_hv_accept.cpp')
x.add('src/glsl/ir_if_simplification.cpp')
x.add('src/glsl/ir_if_to_cond_assign.cpp')
x.add('src/glsl/ir_import_prototypes.cpp')
x.add('src/glsl/ir_lower_jumps.cpp')
x.add('src/glsl/ir_mat_op_to_vec.cpp')
x.add('src/glsl/ir_mod_to_fract.cpp')
x.add('src/glsl/ir_noop_swizzle.cpp')
x.add('src/glsl/ir_print_glsl_visitor.cpp')
x.add('src/glsl/ir_print_visitor.cpp')
x.add('src/glsl/ir_reader.cpp')
x.add('src/glsl/ir_rvalue_visitor.cpp')
x.add('src/glsl/ir_structure_splitting.cpp')
x.add('src/glsl/ir_sub_to_add_neg.cpp')
x.add('src/glsl/ir_swizzle_swizzle.cpp')
x.add('src/glsl/ir_tree_grafting.cpp')
x.add('src/glsl/ir_unused_structs.cpp')
x.add('src/glsl/ir_validate.cpp')
x.add('src/glsl/ir_variable.cpp')
x.add('src/glsl/ir_variable_refcount.cpp')
x.add('src/glsl/ir_vec_index_to_cond_assign.cpp')
x.add('src/glsl/ir_vec_index_to_swizzle.cpp')
x.add('src/glsl/link_functions.cpp')
x.add('src/glsl/linker.cpp')
x.add('src/glsl/loop_analysis.cpp')
x.add('src/glsl/loop_controls.cpp')
x.add('src/glsl/loop_unroll.cpp')
x.add('src/glsl/lower_noise.cpp')
x.add('src/glsl/lower_variable_index_to_cond_assign.cpp')
x.add('src/glsl/opt_redundant_jumps.cpp')
x.add('src/glsl/s_expression.cpp')


glsl_optimizer = x.create()
Export('glsl_optimizer')
