/*
 * Copyright 2024 JLM contributors
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_HLS_BACKEND_RHLS2FIRRTL_BUILDER_FIRRTLBUILDER_HPP
#define JLM_HLS_BACKEND_RHLS2FIRRTL_BUILDER_FIRRTLBUILDER_HPP

#include <jlm/hls/backend/rhls2firrtl/base-hls.hpp>

#include <circt/Dialect/FIRRTL/FIRRTLTypes.h>
#include <mlir/IR/Builders.h>
#include <mlir/IR/Value.h>

namespace jlm {
namespace hls {
namespace backend {
namespace rhls2firrtl {
namespace builder {

/**
 * \brief Helper class for common FIRRTL construction patterns.
 *
 * This class provides utilities for generating FIRRTL operations that are
 * commonly used in the HLS backend conversion process.
 */
class FirrtlBuilder {
public:
  FirrtlBuilder(mlir::Block * body)
      : body_(body) {}

  /// Create a wire with a given name and type
  circt::firrtl::WireOp createWire(const std::string & name, circt::firrtl::FIRRTLBaseType type);

  /// Connect two values (sink = source)
  void connect(mlir::Value sink, mlir::Value source);

  /// Get a subfield from a bundle
  circt::firrtl::SubfieldOp getSubfield(mlir::Value bundle, llvm::StringRef fieldName);

  /// Get a subfield from a bundle by index
  circt::firrtl::SubfieldOp getSubfield(mlir::Value bundle, int index);

  /// Create an integer constant
  circt::firrtl::ConstantOp createConstant(int64_t value, int width);

  /// Create an invalid value of given type
  circt::firrtl::InvalidValueOp createInvalid(circt::firrtl::FIRRTLBaseType type);

  /// Add two FIRRTL values (with carry bit dropped)
  circt::firrtl::AddPrimOp add(mlir::Value lhs, mlir::Value rhs);

  /// Subtract two FIRRTL values (with carry bit dropped)
  circt::firrtl::SubPrimOp sub(mlir::Value lhs, mlir::Value rhs);

  /// Bitwise AND
  circt::firrtl::AndPrimOp andOp(mlir::Value lhs, mlir::Value rhs);

  /// Bitwise OR
  circt::firrtl::OrPrimOp orOp(mlir::Value lhs, mlir::Value rhs);

  /// Bitwise XOR
  circt::firrtl::XorPrimOp xorOp(mlir::Value lhs, mlir::Value rhs);

  /// Equality comparison
  circt::firrtl::EQPrimOp eq(mlir::Value lhs, mlir::Value rhs);

  /// Inequality comparison
  circt::firrtl::NEQPrimOp neq(mlir::Value lhs, mlir::Value rhs);

  /// Less than comparison (unsigned)
  circt::firrtl::LTPrimOp lt(mlir::Value lhs, mlir::Value rhs);

  /// Greater than comparison (unsigned)
  circt::firrtl::GTPrimOp gt(mlir::Value lhs, mlir::Value rhs);

  /// Less than or equal comparison (unsigned)
  circt::firrtl::LEQPrimOp leq(mlir::Value lhs, mlir::Value rhs);

  /// Greater than or equal comparison (unsigned)
  circt::firrtl::GEQPrimOp geq(mlir::Value lhs, mlir::Value rhs);

  /// Mux operation
  circt::firrtl::MuxPrimOp mux(mlir::Value sel, mlir::Value high, mlir::Value low);

  /// Drop most significant bits (for handling overflow)
  circt::firrtl::BitsPrimOp dropMSBs(mlir::Value value, int dropCount);

private:
  mlir::Block * body_;
};

} // namespace builder
} // namespace rhls2firrtl
} // namespace backend
} // namespace hls
} // namespace jlm

#endif // JLM_HLS_BACKEND_RHLS2FIRRTL_BUILDER_FIRRTLBUILDER_HPP