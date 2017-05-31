/*
 * Copyright 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_IR_MODULE_HPP
#define JLM_IR_MODULE_HPP

#include <jlm/IR/clg.hpp>
#include <jlm/IR/expression.hpp>

namespace jlm {

class expr;
class global_variable;

class module final {
public:
	inline
	~module()
	{}

	inline
	module() noexcept
	{}

	inline jlm::clg &
	clg() noexcept
	{
		return clg_;
	}

	inline const jlm::clg &
	clg() const noexcept
	{
		return clg_;
	}

	global_variable *
	add_global_variable(const std::string & name, const expr & e, bool exported);

	inline const expr *
	lookup_global_variable(const global_variable * v)
	{
		auto it = globals_.find(v);
		return it != globals_.end() ? it->second.get() : nullptr;
	}

	/* FIXME: this is ugly */
	std::unordered_map<const global_variable*, std::unique_ptr<const expr>>::const_iterator
	begin() const
	{
		return globals_.begin();
	}

	std::unordered_map<const global_variable*, std::unique_ptr<const expr>>::const_iterator
	end() const
	{
		return globals_.end();
	}

	const variable *
	create_variable(const jive::base::type & type, const std::string & name, bool exported)
	{
		auto v = std::make_unique<variable>(type, name, exported);
		auto pv = v.get();
		variables_.insert(std::move(v));
		return pv;
	}

	const variable *
	create_variable(const jive::base::type & type, bool exported)
	{
		static uint64_t c = 0;
		auto v = std::make_unique<variable>(type, strfmt("v", c), exported);
		auto pv = v.get();
		variables_.insert(std::move(v));
		return pv;
	}

	const variable *
	create_variable(clg_node * node)
	{
		auto v = std::unique_ptr<variable>(new function_variable(node));
		auto pv = v.get();
		variables_.insert(std::move(v));
		return pv;
	}

private:
	jlm::clg clg_;
	std::unordered_set<std::unique_ptr<const variable>> variables_;
	std::unordered_map<const global_variable*, std::unique_ptr<const expr>> globals_;
};

}

#endif
