/*
 * Copyright 2018 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_IR_OPERATORS_GETELEMENTPTR_HPP
#define JLM_IR_OPERATORS_GETELEMENTPTR_HPP

#include <jive/types/bitstring/type.h>
#include <jive/rvsdg/simple-node.h>

#include <jlm/jlm/ir/tac.hpp>
#include <jlm/jlm/ir/types.hpp>

namespace jlm {

/* getelementptr operator */

class getelementptr_op final : public jive::simple_op {
public:
	virtual
	~getelementptr_op();

	inline
	getelementptr_op(
		const jlm::ptrtype & ptype,
		const std::vector<jive::bittype> & btypes,
		const jlm::ptrtype & rtype)
	: simple_op(create_srcports(ptype, btypes), {rtype})
	{}

	virtual bool
	operator==(const operation & other) const noexcept override;

	virtual std::string
	debug_string() const override;

	virtual std::unique_ptr<jive::operation>
	copy() const override;

	inline size_t
	nindices() const noexcept
	{
		return narguments()-1;
	}

	const jive::type &
	pointee_type() const noexcept
	{
		return static_cast<const jlm::ptrtype*>(&argument(0).type())->pointee_type();
	}

private:
	static inline std::vector<jive::port>
	create_srcports(const ptrtype & ptype, const std::vector<jive::bittype> & btypes)
	{
		std::vector<jive::port> ports(1, ptype);
		for (const auto & type : btypes)
			ports.push_back({type});

		return ports;
	}
};

static inline bool
is_getelementptr_op(const jive::operation & op)
{
	return dynamic_cast<const jlm::getelementptr_op*>(&op) != nullptr;
}

static inline std::unique_ptr<jlm::tac>
create_getelementptr_tac(
	const variable * address,
	const std::vector<const variable*> offsets,
	jlm::variable * result)
{
	auto at = dynamic_cast<const jlm::ptrtype*>(&address->type());
	if (!at) throw jlm::error("expected pointer type.");

	std::vector<jive::bittype> bts;
	for (const auto & v : offsets) {
		auto bt = dynamic_cast<const jive::bittype*>(&v->type());
		if (!bt) throw jlm::error("expected bitstring type.");
		bts.push_back(*bt);
	}

	auto rt = dynamic_cast<const jlm::ptrtype*>(&result->type());
	if (!rt) throw jlm::error("expected pointer type.");

	jlm::getelementptr_op op(*at, bts, *rt);
	std::vector<const variable*> operands(1, address);
	operands.insert(operands.end(), offsets.begin(), offsets.end());
	return tac::create(op, operands, {result});
}

}

#endif
