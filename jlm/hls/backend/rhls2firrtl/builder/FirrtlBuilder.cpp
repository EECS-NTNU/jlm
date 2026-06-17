/*
 * Copyright 2024 JLM contributors
 * See COPYING for terms of redistribution.
 */

#include <jlm/hls/backend/rhls2firrtl/builder/FirrtlBuilder.hpp>

namespace jlm {
namespace hls {
namespace backend {
namespace rhls2firrtl {
namespace builder {

circt::firrtl::WireOp
FirrtlBuilder::createWire(const std::string & name, circt::firrtl::FIRRTLBaseType type) {
  return body_->push_back(circt::firrtl::WireOp::create(
      body_->getContext(), type, name, mlir::ArrayAttr(), mlir::UnknownLoc()));
}

void
FirrtlBuilder::connect(mlir::Value sink, mlir::Value source) {
  body_->push_back(circt::firrtl::ConnectOp::create(
      body_->getContext(), sink, source));
}

circt::firrtl::SubfieldOp
FirrtlBuilder::getSubfield(mlir::Value bundle, llvm::StringRef fieldName) {
  return body_->push_back(circt::firrtl::SubfieldOp::create(
      body_->getContext(), bundle, fieldName));
}

circt::firrtl::SubfieldOp
FirrtlBuilder::getSubfield(mlir::Value bundle, int index) {
  return body_->push_back(circt::firrtl::SubfieldOp::create(
      body_->getContext(), bundle, index));
}

circt::firrtl::ConstantOp
FirrtlBuilder::createConstant(int64_t value, int width) {
  auto type = circt::firrtl::IntType::get(body_->getContext(), false, width);
  return body_->push_back(circt::firrtl::ConstantOp::create(
      body_->getContext(), type, mlir::APInt(width, value)));
}

circt::firrtl::InvalidValueOp
FirrtlBuilder::createInvalid(circt::firrtl::FIRRTLBaseType type) {
  return body_->push_back(circt::firrtl::InvalidValueOp::create(
      body_->getContext(), type));
}

circt::firrtl::AddPrimOp
FirrtlBuilder::add(mlir::Value lhs, mlir::Value rhs) {
  return body_->push_back(circt::firrtl::AddPrimOp::create(
      body_->getContext(), lhs, rhs));
}

circt::firrtl::SubPrimOp
FirrtlBuilder::sub(mlir::Value lhs, mlir::Value rhs) {
  return body_->push_back(circt::firrtl::SubPrimOp::create(
      body_->getContext(), lhs, rhs));
}

circt::firrtl::AndPrimOp
FirrtlBuilder::andOp(mlir::Value lhs, mlir::Value rhs) {
  return body_->push_back(circt::firrtl::AndPrimOp::create(
      body_->getContext(), lhs, rhs));
}

circt::firrtl::OrPrimOp
FirrtlBuilder::orOp(mlir::Value lhs, mlir::Value rhs) {
  return body_->push_back(circt::firrtl::OrPrimOp::create(
      body_->getContext(), lhs, rhs));
}

circt::firrtl::XorPrimOp
FirrtlBuilder::xorOp(mlir::Value lhs, mlir::Value rhs) {
  return body_->push_back(circt::firrtl::XorPrimOp::create(
      body_->getContext(), lhs, rhs));
}

circt::firrtl::EQPrimOp
FirrtlBuilder::eq(mlir::Value lhs, mlir::Value rhs) {
  return body_->push_back(circt::firrtl::EQPrimOp::create(
      body_->getContext(), lhs, rhs));
}

circt::firrtl::NEQPrimOp
FirrtlBuilder::neq(mlir::Value lhs, mlir::Value rhs) {
  return body_->push_back(circt::firrtl::NEQPrimOp::create(
      body_->getContext(), lhs, rhs));
}

circt::firrtl::LTPrimOp
FirrtlBuilder::lt(mlir::Value lhs, mlir::Value rhs) {
  return body_->push_back(circt::firrtl::LTPrimOp::create(
      body_->getContext(), lhs, rhs));
}

circt::firrtl::GTPrimOp
FirrtlBuilder::gt(mlir::Value lhs, mlir::Value rhs) {
  return body_->push_back(circt::firrtl::GTPrimOp::create(
      body_->getContext(), lhs, rhs));
}

circt::firrtl::LEQPrimOp
FirrtlBuilder::leq(mlir::Value lhs, mlir::Value rhs) {
  return body_->push_back(circt::firrtl::LEQPrimOp::create(
      body_->getContext(), lhs, rhs));
}

circt::firrtl::GEQPrimOp
FirrtlBuilder::geq(mlir::Value lhs, mlir::Value rhs) {
  return body_->push_back(circt::firrtl::GEQPrimOp::create(
      body_->getContext(), lhs, rhs));
}

circt::firrtl::MuxPrimOp
FirrtlBuilder::mux(mlir::Value sel, mlir::Value high, mlir::Value low) {
  return body_->push_back(circt::firrtl::MuxPrimOp::create(
      body_->getContext(), sel, high, low));
}

circt::firrtl::BitsPrimOp
FirrtlBuilder::dropMSBs(mlir::Value value, int dropCount) {
  int width = value.getType().cast<circt::firrtl::FIRRTLBaseType>().getWidthOrSentinel();
  if (width <= dropCount) {
    // If we're dropping more bits than we have, return a zero-width result
    auto type = circt::firrtl::IntType::get(body_->getContext(), false, 0);
    return body_->push_back(circt::firrtl::BitsPrimOp::create(
        body_->getContext(), value, 0, 0));
  }
  return body_->push_back(circt::firrtl::BitsPrimOp::create(
      body_->getContext(), value, width - 1, dropCount));
}

} // namespace builder
} // namespace rhls2firrtl
} // namespace backend
} // namespace hls
} // namespace jlm