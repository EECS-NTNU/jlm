/*
 * Copyright 2021 David Metz <david.c.metz@ntnu.no>
 * See COPYING for terms of redistribution.
 */

#include <jlm/hls/ir/hls.hpp>
#include <jlm/util/Hash.hpp>

namespace jlm::hls
{

std::size_t
triggertype::ComputeHash() const noexcept
{
  return typeid(triggertype).hash_code();
}

std::shared_ptr<const triggertype>
triggertype::Create()
{
  static const triggertype instance;
  return std::shared_ptr<const triggertype>(std::shared_ptr<void>(), &instance);
}

std::size_t
bundletype::ComputeHash() const noexcept
{
  std::size_t seed = typeid(bundletype).hash_code();
  for (auto & element : elements_)
  {
    auto firstHash = std::hash<std::string>()(element.first);
    util::CombineHashesWithSeed(seed, firstHash, element.second->ComputeHash());
  }

  return seed;
}

EntryArgument::~EntryArgument() noexcept = default;

EntryArgument &
EntryArgument::Copy(rvsdg::Region & region, rvsdg::StructuralInput * input)
{
  return EntryArgument::Create(region, *input, Type());
}

backedge_argument &
backedge_argument::Copy(rvsdg::Region & region, rvsdg::StructuralInput * input)
{
  JLM_ASSERT(input == nullptr);
  return *backedge_argument::create(&region, Type());
}

backedge_result &
backedge_result::Copy(rvsdg::output & origin, rvsdg::StructuralOutput * output)
{
  JLM_ASSERT(output == nullptr);
  return *backedge_result::create(&origin);
}

ExitResult::~ExitResult() noexcept = default;

ExitResult &
ExitResult::Copy(rvsdg::output & origin, rvsdg::StructuralOutput * output)
{
  return Create(origin, *output);
}

rvsdg::StructuralOutput *
loop_node::AddLoopVar(jlm::rvsdg::output * origin, jlm::rvsdg::output ** buffer)
{
  auto input = rvsdg::StructuralInput::create(this, origin, origin->Type());
  auto output = rvsdg::StructuralOutput::create(this, origin->Type());

  auto & argument_in = EntryArgument::Create(*subregion(), *input, origin->Type());
  auto argument_loop = add_backedge(origin->Type());

  auto mux =
      hls::mux_op::create(*predicate_buffer(), { &argument_in, argument_loop }, false, true)[0];
  auto branch = hls::branch_op::create(*predicate()->origin(), *mux, true);
  if (buffer != nullptr)
  {
    *buffer = mux;
  }
  ExitResult::Create(*branch[0], *output);
  auto result_loop = argument_loop->result();
  auto buf = hls::buffer_op::create(*branch[1], 2)[0];
  result_loop->divert_to(buf);
  return output;
}

[[nodiscard]] const rvsdg::Operation &
loop_node::GetOperation() const noexcept
{
  static const loop_op singleton;
  return singleton;
}

jlm::rvsdg::output *
loop_node::add_loopconst(jlm::rvsdg::output * origin)
{
  auto input = rvsdg::StructuralInput::create(this, origin, origin->Type());

  auto & argument_in = EntryArgument::Create(*subregion(), *input, origin->Type());
  auto buffer = hls::loop_constant_buffer_op::create(*predicate_buffer(), argument_in)[0];
  return buffer;
}

loop_node *
loop_node::copy(rvsdg::Region * region, rvsdg::SubstitutionMap & smap) const
{
  auto loop = create(region, false);

  for (size_t i = 0; i < ninputs(); ++i)
  {
    auto in_origin = smap.lookup(input(i)->origin());
    auto inp = rvsdg::StructuralInput::create(loop, in_origin, in_origin->Type());
    smap.insert(input(i), loop->input(i));
    auto oarg = input(i)->arguments.begin().ptr();
    auto & narg = EntryArgument::Create(*loop->subregion(), *inp, oarg->Type());
    smap.insert(oarg, &narg);
  }
  for (size_t i = 0; i < noutputs(); ++i)
  {
    auto out = rvsdg::StructuralOutput::create(loop, output(i)->Type());
    smap.insert(output(i), out);
    smap.insert(output(i), out);
  }
  for (size_t i = 0; i < subregion()->narguments(); ++i)
  {
    auto arg = subregion()->argument(i);
    if (auto ba = dynamic_cast<backedge_argument *>(arg))
    {
      auto na = loop->add_backedge(arg->Type());
      smap.insert(ba, na);
    }
  }

  subregion()->copy(loop->subregion(), smap, false, false);
  loop->_predicate_buffer = dynamic_cast<jlm::rvsdg::node_output *>(smap.lookup(_predicate_buffer));
  // redirect backedges
  for (size_t i = 0; i < subregion()->narguments(); ++i)
  {
    auto arg = subregion()->argument(i);
    if (auto ba = dynamic_cast<backedge_argument *>(arg))
    {
      auto na = dynamic_cast<backedge_argument *>(smap.lookup(ba));
      na->result()->divert_to(smap.lookup(ba->result()->origin()));
    }
  }
  for (size_t i = 0; i < noutputs(); ++i)
  {
    auto outp = output(i);
    auto res = outp->results.begin().ptr();
    auto origin = smap.lookup(res->origin());
    ExitResult::Create(*origin, *loop->output(i));
  }

  return loop;
}

backedge_argument *
loop_node::add_backedge(std::shared_ptr<const jlm::rvsdg::Type> type)
{
  auto argument_loop = backedge_argument::create(subregion(), std::move(type));
  auto result_loop = backedge_result::create(argument_loop);
  argument_loop->result_ = result_loop;
  result_loop->argument_ = argument_loop;
  return argument_loop;
}

loop_node *
loop_node::create(rvsdg::Region * parent, bool init)
{
  auto ln = new loop_node(parent);
  if (init)
  {
    auto predicate = jlm::rvsdg::control_false(ln->subregion());
    auto pred_arg = ln->add_backedge(rvsdg::ControlType::Create(2));
    pred_arg->result()->divert_to(predicate);
    // we need a buffer without pass-through behavior to avoid a combinatorial cycle of ready
    // signals
    auto pre_buffer = hls::buffer_op::create(*pred_arg, 2)[0];
    ln->_predicate_buffer =
        dynamic_cast<jlm::rvsdg::node_output *>(hls::predicate_buffer_op::create(*pre_buffer)[0]);
  }
  return ln;
}

void
loop_node::set_predicate(jlm::rvsdg::output * p)
{
  auto node = jlm::rvsdg::output::GetNode(*predicate()->origin());
  predicate()->origin()->divert_users(p);
  if (node && !node->has_users())
    remove(node);
}

std::shared_ptr<const bundletype>
get_mem_req_type(std::shared_ptr<const rvsdg::ValueType> elementType, bool write)
{
  std::vector<std::pair<std::string, std::shared_ptr<const jlm::rvsdg::Type>>> elements;
  elements.emplace_back("addr", llvm::PointerType::Create());
  elements.emplace_back("size", jlm::rvsdg::bittype::Create(4));
  elements.emplace_back("id", jlm::rvsdg::bittype::Create(8));
  if (write)
  {
    elements.emplace_back("data", std::move(elementType));
    elements.emplace_back("write", jlm::rvsdg::bittype::Create(1));
  }
  return std::make_shared<bundletype>(std::move(elements));
}

std::shared_ptr<const bundletype>
get_mem_res_type(std::shared_ptr<const jlm::rvsdg::ValueType> dataType)
{
  std::vector<std::pair<std::string, std::shared_ptr<const jlm::rvsdg::Type>>> elements;
  elements.emplace_back("data", std::move(dataType));
  elements.emplace_back("id", jlm::rvsdg::bittype::Create(8));
  return std::make_shared<bundletype>(std::move(elements));
}
}
