/*
 * Copyright 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2011 2012 2014 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_RVSDG_BITSTRING_BITOPERATION_CLASSES_HPP
#define JLM_RVSDG_BITSTRING_BITOPERATION_CLASSES_HPP

#include <jlm/rvsdg/binary.hpp>
#include <jlm/rvsdg/bitstring/type.hpp>
#include <jlm/rvsdg/bitstring/value-representation.hpp>
#include <jlm/rvsdg/unary.hpp>

namespace jlm::rvsdg
{

/* Represents a unary operation on a bitstring of a specific width,
 * produces another bitstring of the same width. */
class BitUnaryOperation : public UnaryOperation
{
public:
  ~BitUnaryOperation() noexcept override;

  explicit BitUnaryOperation(const std::shared_ptr<const bittype> & type) noexcept
      : UnaryOperation(type, type)
  {}

  inline const bittype &
  type() const noexcept
  {
    return *std::static_pointer_cast<const bittype>(argument(0));
  }

  unop_reduction_path_t
  can_reduce_operand(const jlm::rvsdg::Output * arg) const noexcept override;

  jlm::rvsdg::Output *
  reduce_operand(unop_reduction_path_t path, jlm::rvsdg::Output * arg) const override;

  virtual bitvalue_repr
  reduce_constant(const bitvalue_repr & arg) const = 0;

  virtual std::unique_ptr<BitUnaryOperation>
  create(size_t nbits) const = 0;
};

/* Represents a binary operation (possibly normalized n-ary if associative)
 * on a bitstring of a specific width, produces another bitstring of the
 * same width. */
class BitBinaryOperation : public BinaryOperation
{
public:
  ~BitBinaryOperation() noexcept override;

  explicit BitBinaryOperation(const std::shared_ptr<const bittype> type, size_t arity = 2) noexcept
      : BinaryOperation({ arity, type }, type)
  {}

  /* reduction methods */
  binop_reduction_path_t
  can_reduce_operand_pair(const jlm::rvsdg::Output * arg1, const jlm::rvsdg::Output * arg2)
      const noexcept override;

  jlm::rvsdg::Output *
  reduce_operand_pair(
      binop_reduction_path_t path,
      jlm::rvsdg::Output * arg1,
      jlm::rvsdg::Output * arg2) const override;

  virtual bitvalue_repr
  reduce_constants(const bitvalue_repr & arg1, const bitvalue_repr & arg2) const = 0;

  virtual std::unique_ptr<BitBinaryOperation>
  create(size_t nbits) const = 0;

  inline const bittype &
  type() const noexcept
  {
    return *std::static_pointer_cast<const bittype>(result(0));
  }
};

enum class compare_result
{
  undecidable,
  static_true,
  static_false
};

class BitCompareOperation : public BinaryOperation
{
public:
  ~BitCompareOperation() noexcept override;

  explicit BitCompareOperation(std::shared_ptr<const bittype> type) noexcept
      : BinaryOperation({ type, type }, bittype::Create(1))
  {}

  binop_reduction_path_t
  can_reduce_operand_pair(const jlm::rvsdg::Output * arg1, const jlm::rvsdg::Output * arg2)
      const noexcept override;

  jlm::rvsdg::Output *
  reduce_operand_pair(
      binop_reduction_path_t path,
      jlm::rvsdg::Output * arg1,
      jlm::rvsdg::Output * arg2) const override;

  virtual compare_result
  reduce_constants(const bitvalue_repr & arg1, const bitvalue_repr & arg2) const = 0;

  virtual std::unique_ptr<BitCompareOperation>
  create(size_t nbits) const = 0;

  inline const bittype &
  type() const noexcept
  {
    return *std::static_pointer_cast<const bittype>(argument(0));
  }
};

}

#endif
