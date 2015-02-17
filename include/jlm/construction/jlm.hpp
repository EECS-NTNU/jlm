/*
 * Copyright 2014 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_CONSTRUCTION_JLM_HPP
#define JLM_CONSTRUCTION_JLM_HPP

#include <jlm/IR/clg.hpp>

#include <unordered_map>

namespace llvm {
	class BasicBlock;
	class Function;
	class Instruction;
	class Module;
	class Value;
}

namespace jlm {

typedef std::unordered_map<const llvm::BasicBlock*, jlm::frontend::basic_block*> basic_block_map;

typedef std::unordered_map<const llvm::Value*, const jlm::frontend::variable*> value_map;

void
convert_module(const llvm::Module & module, jlm::frontend::clg & clg);

}

#endif
