/*
 * Copyright 2021 David Metz <david.c.metz@ntnu.no>
 * See COPYING for terms of redistribution.
 */

#include <jlm/hls/backend/rhls2firrtl/base-hls.hpp>
#include <jlm/hls/backend/rvsdg2rhls/bram-pointers.hpp>
#include <jlm/hls/backend/rvsdg2rhls/mem-conv.hpp>
#include <jlm/hls/backend/rvsdg2rhls/rhls-dne.hpp>
#include <jlm/hls/backend/rvsdg2rhls/rvsdg2rhls.hpp>
#include <jlm/hls/ir/hls.hpp>
#include <jlm/llvm/ir/operators/GetElementPtr.hpp>
#include <jlm/llvm/ir/operators/lambda.hpp>
#include <jlm/llvm/ir/operators/operators.hpp>
#include <jlm/rvsdg/bitstring/arithmetic.hpp>
#include <jlm/rvsdg/traverser.hpp>

namespace jlm::hls
{

void
replace_gep(rvsdg::simple_node * simplenode)
{
  auto op = dynamic_cast<const jlm::llvm::GetElementPtrOperation *>(&simplenode->operation());

  //    auto bc = jive::bitconstant_op::create(simplenode->region(),
  //    jive::bitvalue_repr(jlm::hls::pointer_bits, 0)); auto ptr = jlm::bits2ptr_op::create(bc,
  //    simplenode->input(0)->type()); simplenode->input(0)->divert_to(ptr);

  rvsdg::output * out = nullptr;
  JLM_ASSERT(op);
  const rvsdg::type * pointeeType = &op->GetPointeeType();
  for (size_t i = 1; i < simplenode->ninputs(); ++i)
  {
    int bits = jlm::hls::BaseHLS::JlmSize(pointeeType);
    int bytes = bits / 8;
    if (dynamic_cast<const rvsdg::bittype *>(pointeeType))
    {
      pointeeType = nullptr;
    }
    else if (auto arrayType = dynamic_cast<const jlm::llvm::arraytype *>(pointeeType))
    {
      pointeeType = &arrayType->element_type();
    }
    else
    {
      throw std::logic_error(pointeeType->debug_string() + " pointer not implemented!");
    }
    // TODO: strength reduction to shift if applicable
    auto constant = rvsdg::bitconstant_op::create(
        simplenode->region(),
        rvsdg::bitvalue_repr(jlm::hls::BaseHLS::GetPointerSizeInBits(), bytes));
    auto ot = dynamic_cast<const rvsdg::bittype *>(&simplenode->input(i)->origin()->type());
    auto origin = simplenode->input(i)->origin();
    JLM_ASSERT(ot);
    if (ot->nbits() != jlm::hls::BaseHLS::GetPointerSizeInBits())
    {
      origin = jlm::llvm::trunc_op::create(jlm::hls::BaseHLS::GetPointerSizeInBits(), origin);
    }
    auto mul =
        rvsdg::bitmul_op::create(jlm::hls::BaseHLS::GetPointerSizeInBits(), constant, origin);
    if (out)
    {
      out = rvsdg::bitadd_op::create(jlm::hls::BaseHLS::GetPointerSizeInBits(), out, mul);
    }
    else
    {
      out = mul;
    }
  }
  JLM_ASSERT(out);
  auto ptr = jlm::llvm::bits2ptr_op::create(out, simplenode->output(0)->Type());
  simplenode->output(0)->divert_users(ptr);
  remove(simplenode);
}

void
replace_geps(rvsdg::output * out)
{
  JLM_ASSERT(dynamic_cast<const jlm::llvm::PointerType *>(&out->type()));
  bool changed;
  do
  {
    changed = false;
    for (auto user : *out)
    {
      if (auto sti = dynamic_cast<rvsdg::structural_input *>(user))
      {
        JLM_ASSERT(sti->arguments.size() == 1);
        auto arg = sti->arguments.first();
        JLM_ASSERT(arg->nusers() == 1);
        auto arg_user = *arg->begin();
        auto ni = dynamic_cast<rvsdg::node_input *>(arg_user);
        JLM_ASSERT(ni);
        if (dynamic_cast<const jlm::hls::loop_constant_buffer_op *>(&ni->node()->operation()))
        {
          replace_geps(ni->node()->output(0));
        }
      }
      else if (auto si = dynamic_cast<rvsdg::simple_input *>(user))
      {
        auto simplenode = si->node();
        if (dynamic_cast<const jlm::hls::branch_op *>(&simplenode->operation()))
        {
          for (size_t i = 0; i < simplenode->noutputs(); ++i)
          {
            replace_geps(simplenode->output(i));
          }
        }
        else if (dynamic_cast<const jlm::llvm::GetElementPtrOperation *>(&simplenode->operation()))
        {
          replace_gep(simplenode);
          // users of out change here
          changed = true;
          break;
        }
        else if (
            rvsdg::is<jlm::hls::decoupled_load_op>(simplenode)
            || rvsdg::is<jlm::hls::load_op>(simplenode)
            || rvsdg::is<jlm::hls::store_op>(simplenode))
        {
          auto bc = rvsdg::bitconstant_op::create(
              simplenode->region(),
              rvsdg::bitvalue_repr(jlm::hls::BaseHLS::GetPointerSizeInBits(), 0));
          auto ptr = jlm::llvm::bits2ptr_op::create(bc, si->Type());
          si->divert_to(ptr);
          changed = true;
          break;
        }
        else
        {
          auto bc = rvsdg::bitconstant_op::create(
              simplenode->region(),
              rvsdg::bitvalue_repr(jlm::hls::BaseHLS::GetPointerSizeInBits(), 0));
          auto ptr = jlm::llvm::bits2ptr_op::create(bc, si->Type());
          si->divert_to(ptr);
          changed = true;
          break;
          //                    JLM_UNREACHABLE("Unexpected node type");
        }
      }
    }
  } while (changed);
}

void
bram_pointers(rvsdg::Region * root)
{
  // pass that removes unnecessary base pointers if brams are used
  auto lambda = dynamic_cast<jlm::llvm::lambda::node *>(root->nodes.begin().ptr());
  for (size_t i = 0; i < lambda->subregion()->narguments(); ++i)
  {
    auto arg = lambda->subregion()->argument(i);
    if (auto pt = dynamic_cast<const jlm::llvm::PointerType *>(&arg->type()))
    {
      // Decoupled loads are user specified and encoded as function calls that need special
      // treatment
      std::unordered_set<jlm::rvsdg::output *> visited;
      if (IsDecoupledFunctionPointer((jlm::rvsdg::output *)(arg), visited))
      {
        // We are only interested in the address of the load and not the function pointer itself
        continue;
      }
      // if (dynamic_cast<const jlm::llvm::FunctionType *>(&pt->GetElementType())) {
      //     // skip function pointers
      //     continue;
      // }
      replace_geps(arg);
      auto bc = rvsdg::bitconstant_op::create(
          arg->region(),
          rvsdg::bitvalue_repr(jlm::hls::BaseHLS::GetPointerSizeInBits(), 0));
      auto ptr = jlm::llvm::bits2ptr_op::create(bc, arg->Type());
      arg->divert_users(ptr);
    }
  }
}

void
bram_pointers(jlm::llvm::RvsdgModule & rm)
{
  auto & graph = rm.Rvsdg();
  auto root = graph.root();
  bram_pointers(root);
  dne(rm);
}

}
