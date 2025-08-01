/*
 * Copyright 2021 David Metz <david.c.metz@ntnu.no>
 * See COPYING for terms of redistribution.
 */

#include <jlm/hls/backend/rvsdg2rhls/add-buffers.hpp>
#include <jlm/hls/backend/rvsdg2rhls/hls-function-util.hpp>
#include <jlm/hls/backend/rvsdg2rhls/rvsdg2rhls.hpp>
#include <jlm/hls/ir/hls.hpp>
#include <jlm/hls/util/view.hpp>
#include <jlm/rvsdg/traverser.hpp>

namespace jlm::hls
{

rvsdg::Input *
FindUserNode(rvsdg::Output * out)
{

  auto user = &out->SingleUser();
  if (auto br = dynamic_cast<backedge_result *>(user))
  {
    return FindUserNode(br->argument());
  }
  else if (auto rr = dynamic_cast<rvsdg::RegionResult *>(user))
  {

    if (rr->output() && rvsdg::TryGetOwnerNode<LoopNode>(*rr->output()))
    {
      return FindUserNode(rr->output());
    }
    else
    {
      // lambda result
      return rr;
    }
  }
  else if (auto si = dynamic_cast<rvsdg::StructuralInput *>(user))
  {
    JLM_ASSERT(rvsdg::TryGetOwnerNode<LoopNode>(*user));
    return FindUserNode(si->arguments.begin().ptr());
  }
  else if (rvsdg::TryGetOwnerNode<rvsdg::SimpleNode>(*user))
  {
    return user;
  }

  JLM_UNREACHABLE("This should not have happened!");
}

void
PlaceBuffer(rvsdg::Output * out, size_t capacity, bool passThrough)
{
  // places or re-places a buffer on an output
  auto user = FindUserNode(out);
  // don't place buffers after constants
  if (is_constant(rvsdg::TryGetOwnerNode<rvsdg::SimpleNode>(*out)))
  {
    return;
  }
  auto [forkNode, forkOperation] = rvsdg::TryGetSimpleNodeAndOp<ForkOperation>(*out);
  if (forkOperation && forkOperation->IsConstant())
  {
    return;
  }

  // TODO: handle out being a buf?
  auto [bufferNode, bufferOperation] = rvsdg::TryGetSimpleNodeAndOp<BufferOperation>(*user);
  if (bufferOperation
      && (bufferOperation->IsPassThrough() != passThrough
          || bufferOperation->Capacity() != capacity))
  {
    // replace buffer and keep larger size
    auto node = rvsdg::TryGetOwnerNode<rvsdg::SimpleNode>(*user);
    passThrough = passThrough && bufferOperation->IsPassThrough();
    capacity = std::max(capacity, bufferOperation->Capacity());
    auto bufOut = BufferOperation::create(*node->input(0)->origin(), capacity, passThrough)[0];
    node->output(0)->divert_users(bufOut);
    JLM_ASSERT(node->IsDead());
    remove(node);
  }
  else
  {
    // create new buffer
    auto & directUser = *out->Users().begin();
    auto newOut = BufferOperation::create(*out, capacity, passThrough)[0];
    directUser.divert_to(newOut);
  }
}

const size_t BufferSizeForkState = 1;
const size_t BufferSizeForkControl = 8;
const size_t BufferSizeForkOther = 4;

void
OptimizeFork(rvsdg::SimpleNode * node)
{
  const auto fork = util::AssertedCast<const ForkOperation>(&node->GetOperation());
  bool inLoop = rvsdg::is<LoopOperation>(node->region()->node());
  if (fork->IsConstant() || !inLoop)
  {
    // cForks and forks outside of loops should have no buffers after it
    for (size_t i = 0; i < node->noutputs(); ++i)
    {
      auto user = FindUserNode(node->output(0));
      auto [bufferNode, bufferOperation] = rvsdg::TryGetSimpleNodeAndOp<BufferOperation>(*user);
      if (bufferOperation)
      {
        bufferNode->output(0)->divert_users(node->output(0));
        JLM_ASSERT(bufferNode->IsDead());
        remove(bufferNode);
      }
    }
  }
  else
  {
    // forks inside of loops should have buffers after it
    size_t bufferSize = BufferSizeForkOther;
    if (rvsdg::is<rvsdg::ControlType>(node->input(0)->Type()))
    {
      bufferSize = BufferSizeForkControl;
    }
    else if (rvsdg::is<rvsdg::StateType>(node->input(0)->Type()))
    {
      bufferSize = BufferSizeForkState;
    }
    for (size_t i = 0; i < node->noutputs(); ++i)
    {
      PlaceBuffer(node->output(i), bufferSize, true);
    }
  }
}

const size_t BufferSizeBranchState = BufferSizeForkControl;

void
OptimizeBranch(rvsdg::SimpleNode * node)
{
  auto branch = dynamic_cast<const BranchOperation *>(&node->GetOperation());
  JLM_ASSERT(branch);
  bool inLoop = rvsdg::is<LoopOperation>(node->region()->node());
  if (inLoop && !branch->loop)
  {
    // TODO: this optimization is for long stores with responses. It might be better to do it
    // somewhere else and more selectively (only when there is a store in one of the gamma
    // subregions, and only on outputs that don't go to store)
    if (rvsdg::is<rvsdg::StateType>(node->input(1)->Type()))
    {
      for (size_t i = 0; i < node->noutputs(); ++i)
      {
        PlaceBuffer(node->output(i), BufferSizeBranchState, true);
      }
    }
  }
}

void
OptimizeStateGate(rvsdg::SimpleNode * node)
{
  // TODO: remove duplicate? somewhere else?
  // TODO: place buffers on state outputs?
}

void
OptimizeAddrQ(rvsdg::SimpleNode * node)
{
  auto addrq = dynamic_cast<const AddressQueueOperation *>(&node->GetOperation());
  JLM_ASSERT(addrq);
  // place buffer on addr output
  PlaceBuffer(node->output(0), addrq->capacity, true);
}

void
OptimizeBuffer(rvsdg::SimpleNode * node)
{
  auto buf = dynamic_cast<const BufferOperation *>(&node->GetOperation());
  JLM_ASSERT(buf);
  auto user = FindUserNode(node->output(0));
  auto [bufferNode, bufferOperation] = rvsdg::TryGetSimpleNodeAndOp<BufferOperation>(*user);
  if (bufferOperation)
  {
    auto node2 = rvsdg::TryGetOwnerNode<rvsdg::SimpleNode>(*user);
    // merge buffers and keep larger size
    bool passThrough = buf->IsPassThrough() && bufferOperation->IsPassThrough();
    auto capacity = std::max(buf->Capacity(), bufferOperation->Capacity());
    auto newOut = BufferOperation::create(*node->input(0)->origin(), capacity, passThrough)[0];
    JLM_ASSERT(node2->region() == newOut->region());
    node2->output(0)->divert_users(newOut);
    JLM_ASSERT(node2->IsDead());
    remove(node2);
    JLM_ASSERT(node->IsDead());
    remove(node);
  }
}

void
OptimizeLoop(LoopNode * loopNode)
{
  // TODO: should this be changed?
  bool outerLoop = !rvsdg::is<LoopOperation>(loopNode->region()->node());
  if (outerLoop)
  {
    // push buffers above branches, so they also act as output buffers
    for (size_t i = 0; i < loopNode->noutputs(); ++i)
    {
      auto out = loopNode->output(i);
      auto res = out->results.begin().ptr();
      auto [branchNode, branchOperation] =
          rvsdg::TryGetSimpleNodeAndOp<BranchOperation>(*res->origin());
      if (!branchOperation)
      {
        // this is a memory operation or stream
        continue;
      }
      JLM_ASSERT(branchOperation->loop);
      auto oldBufInput = &branchNode->output(1)->SingleUser();
      auto [oldBufferNode, oldBufferOperation] =
          rvsdg::TryGetSimpleNodeAndOp<BufferOperation>(*oldBufInput);
      if (std::get<1>(rvsdg::TryGetSimpleNodeAndOp<SinkOperation>(*oldBufInput)))
      {
        // no backedge
        continue;
      }
      JLM_ASSERT(oldBufferOperation);
      auto oldBufNode = rvsdg::TryGetOwnerNode<rvsdg::SimpleNode>(*oldBufInput);
      // place new buffers
      PlaceBuffer(
          branchNode->input(1)->origin(),
          oldBufferOperation->Capacity(),
          oldBufferOperation->IsPassThrough());
      // this buffer should just make the fork buf non-passthrough - needed to avoid combinatorial
      // cycle
      PlaceBuffer(
          branchNode->input(0)->origin(),
          oldBufferOperation->Capacity(),
          oldBufferOperation->IsPassThrough());
      // remove old buffer
      oldBufNode->output(0)->divert_users(oldBufInput->origin());
      JLM_ASSERT(oldBufNode->IsDead());
      remove(oldBufNode);
    }
  }
  else
  {
    // add input buffers
    for (size_t i = 0; i < loopNode->ninputs(); ++i)
    {
      auto in = loopNode->input(i);
      auto arg = in->arguments.begin().ptr();
      auto user = &arg->SingleUser();
      // only do this for proper loop variables
      if (auto [node, muxOperation] = rvsdg::TryGetSimpleNodeAndOp<MuxOperation>(*user);
          muxOperation)
      {
        if (!muxOperation->loop)
        {
          // stream
          continue;
        }
      }
      else if (std::get<1>(rvsdg::TryGetSimpleNodeAndOp<LoopConstantBufferOperation>(*user)))
      {
      }
      else
      {
        continue;
      }
      PlaceBuffer(in->origin(), 2, false);
    }
  }
}

void
AddBuffers(rvsdg::Region * region)
{
  for (auto & node : rvsdg::TopDownTraverser(region))
  {
    if (auto structnode = dynamic_cast<rvsdg::StructuralNode *>(node))
    {
      auto loop = dynamic_cast<LoopNode *>(node);
      JLM_ASSERT(loop);
      OptimizeLoop(loop);
      for (size_t n = 0; n < structnode->nsubregions(); n++)
      {
        AddBuffers(structnode->subregion(n));
      }
    }
    else if (auto simple = dynamic_cast<jlm::rvsdg::SimpleNode *>(node))
    {
      if (jlm::rvsdg::is<BufferOperation>(node))
      {
        OptimizeBuffer(simple);
      }
      else if (jlm::rvsdg::is<ForkOperation>(node))
      {
        //        OptimizeFork(simple);
      }
      else if (jlm::rvsdg::is<BranchOperation>(node))
      {
        //        OptimizeBranch(simple);
      }
      else if (jlm::rvsdg::is<StateGateOperation>(node))
      {
        //        OptimizeStateGate(simple);
      }
      else if (jlm::rvsdg::is<AddressQueueOperation>(node))
      {
        OptimizeAddrQ(simple);
      }
    }
  }
}

size_t MemoryLatency = 10;

constexpr uint32_t
round_up_pow2(uint32_t x)
{
  if (x == 0)
    return 1;
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return x + 1;
}

void
MaximizeBuffers(rvsdg::Region * region)
{
  //  const size_t capacity = 256;
  std::vector<jlm::rvsdg::SimpleNode *> nodes;
  for (auto & node : rvsdg::TopDownTraverser(region))
  {
    if (auto structnode = dynamic_cast<rvsdg::StructuralNode *>(node))
    {
      auto loop = dynamic_cast<LoopNode *>(node);
      JLM_ASSERT(loop);
      for (size_t n = 0; n < structnode->nsubregions(); n++)
      {
        MaximizeBuffers(structnode->subregion(n));
      }
    }
    else if (auto sn = dynamic_cast<jlm::rvsdg::SimpleNode *>(node))
    {
      if (rvsdg::is<BufferOperation>(node))
      {
        nodes.push_back(sn);
      }
      else if (rvsdg::is<DecoupledLoadOperation>(node))
      {
        nodes.push_back(sn);
      }
    }
  }
  for (auto node : nodes)
  {
    if (auto dl = dynamic_cast<const DecoupledLoadOperation *>(&node->GetOperation()))
    {
      auto capacity = round_up_pow2(MemoryLatency);
      if (dl->capacity < capacity)
      {
        divert_users(
            node,
            DecoupledLoadOperation::create(
                *node->input(0)->origin(),
                *node->input(1)->origin(),
                capacity));
        remove(node);
      }
    }
  }
}

std::vector<size_t>
NodeCycles(rvsdg::SimpleNode * node, std::vector<size_t> & input_cycles)
{
  auto max_cycles = *std::max_element(input_cycles.begin(), input_cycles.end());
  if (auto op = dynamic_cast<const llvm::FBinaryOperation *>(&node->GetOperation()))
  {
    if (op->fpop() == llvm::fpop::add)
    {
      return { max_cycles + 1 };
    }
  }
  else if (auto op = dynamic_cast<const BufferOperation *>(&node->GetOperation()))
  {
    if (op->IsPassThrough())
    {
      return { max_cycles + 0 };
    }
    return { max_cycles + 1 };
  }
  else if (dynamic_cast<const AddressQueueOperation *>(&node->GetOperation()))
  {
    return { input_cycles[0] };
  }
  else if (rvsdg::is<DecoupledLoadOperation>(node))
  {
    return { max_cycles + MemoryLatency, 0 };
  }
  else if (rvsdg::is<StateGateOperation>(node))
  {
    // handle special state gate that sits on dec_load response
    auto sg0_user = &node->output(0)->SingleUser();
    if (std::get<1>(rvsdg::TryGetSimpleNodeAndOp<DecoupledLoadOperation>(*sg0_user))
        && sg0_user->index() == 1)
    {
      JLM_ASSERT(max_cycles == 0);
      return { 0, MemoryLatency };
    }
  }
  else if (rvsdg::is<StoreOperation>(node))
  {
    JLM_ASSERT(node->noutputs() == 3);
    return { max_cycles + MemoryLatency, 0, 0 };
  }
  return std::vector<size_t>(node->noutputs(), max_cycles);
}

const size_t UnlimitedBufferCapacity = std::numeric_limits<uint32_t>::max();

std::vector<size_t>
NodeCapacity(rvsdg::SimpleNode * node, std::vector<size_t> & input_capacities)
{
  auto min_capacity = *std::min_element(input_capacities.begin(), input_capacities.end());
  if (auto op = dynamic_cast<const llvm::FBinaryOperation *>(&node->GetOperation()))
  {
    if (op->fpop() == llvm::fpop::add)
    {
      return { min_capacity + 1 };
    }
  }
  else if (auto op = dynamic_cast<const BufferOperation *>(&node->GetOperation()))
  {
    return { min_capacity + op->Capacity() };
  }
  else if (dynamic_cast<const AddressQueueOperation *>(&node->GetOperation()))
  {
    return { input_capacities[0] };
  }
  else if (auto op = dynamic_cast<const DecoupledLoadOperation *>(&node->GetOperation()))
  {
    return { min_capacity + op->capacity, 0 };
  }
  else if (rvsdg::is<StateGateOperation>(node))
  {
    // handle special state gate that sits on dec_load response
    auto sg0_user = &node->output(0)->SingleUser();
    if (std::get<1>(rvsdg::TryGetSimpleNodeAndOp<DecoupledLoadOperation>(*sg0_user))
        && sg0_user->index() == 1)
    {
      JLM_ASSERT(min_capacity == UnlimitedBufferCapacity);
      return { 0, MemoryLatency };
    }
  }
  else if (rvsdg::is<StoreOperation>(node))
  {
    return { min_capacity + MemoryLatency, 0, 0 };
  }
  return std::vector<size_t>(node->noutputs(), min_capacity);
}

void
CreateLoopFrontier(
    const LoopNode * loop,
    std::unordered_map<rvsdg::Output *, size_t> & output_cycles,
    std::unordered_set<rvsdg::Input *> & frontier,
    std::unordered_set<backedge_result *> & stream_backedges,
    std::unordered_set<rvsdg::SimpleNode *> & top_muxes)
{
  for (size_t i = 0; i < loop->ninputs(); ++i)
  {
    auto in = loop->input(i);
    auto arg = in->arguments.begin().ptr();

    auto user = &arg->SingleUser();
    auto userNode = rvsdg::TryGetOwnerNode<rvsdg::SimpleNode>(*user);
    auto [muxNode, muxOperation] = rvsdg::TryGetSimpleNodeAndOp<MuxOperation>(*user);
    if (std::get<1>(rvsdg::TryGetSimpleNodeAndOp<LoopConstantBufferOperation>(*user))
        || (muxOperation && muxOperation->loop))
    {
      top_muxes.insert(userNode);
      // we start from these
      auto out = userNode->output(0);
      output_cycles[out] = output_cycles[in->origin()];
      frontier.insert(&out->SingleUser());
    }
    // these are needed so we can finish with an empty frontier
    output_cycles[arg] = output_cycles[in->origin()];
    frontier.insert(&arg->SingleUser());
  }
  for (auto & tn : loop->subregion()->TopNodes())
  {
    JLM_ASSERT(is_constant(&tn));
    auto out = tn.output(0);
    output_cycles[out] = 0;
    frontier.insert(&out->SingleUser());
  }
  for (auto arg : loop->subregion()->Arguments())
  {
    auto backedge = dynamic_cast<backedge_argument *>(arg);
    if (!backedge)
    {
      continue;
    }
    auto user = &arg->SingleUser();
    auto [muxNode, muxOperation] = rvsdg::TryGetSimpleNodeAndOp<MuxOperation>(*user);
    if ((muxOperation && muxOperation->loop))
    {
      continue;
    }
    if (std::get<1>(rvsdg::TryGetSimpleNodeAndOp<BufferOperation>(*user)))
    {
      auto bufNode = rvsdg::TryGetOwnerNode<rvsdg::SimpleNode>(*user);
      if (std::get<1>(rvsdg::TryGetSimpleNodeAndOp<PredicateBufferOperation>(
              bufNode->output(0)->SingleUser())))
      {
        // skip predicate buffer
        continue;
      }
    }
    // this comes from somewhere inside the loop
    output_cycles[arg] = 0;
    frontier.insert(&arg->SingleUser());
    stream_backedges.insert(backedge->result());
  }
}

void
CalculateLoopCycleDepth(
    LoopNode * loop,
    std::unordered_map<rvsdg::Output *, size_t> & output_cycles,
    bool analyze_inner_loop = false);

void
PushCycleFrontier(
    std::unordered_map<rvsdg::Output *, size_t> & output_cycles,
    std::unordered_set<rvsdg::Input *> & frontier,
    std::unordered_set<backedge_result *> & stream_backedges,
    std::unordered_set<rvsdg::SimpleNode *> & top_muxes)
{
  bool changed = false;
  do
  {
    changed = false;
    for (auto in : frontier)
    {
      if (auto simpleNode = rvsdg::TryGetOwnerNode<rvsdg::SimpleNode>(*in))
      {
        bool all_contained = true;
        for (size_t i = 0; i < simpleNode->ninputs(); ++i)
        {
          auto f = frontier.find(simpleNode->input(i));
          if (f == frontier.end())
          {
            all_contained = false;
          }
        }
        if (!all_contained)
          continue;
        // all inputs of node are in frontier - move them forward
        std::vector<size_t> input_cycles;
        for (size_t i = 0; i < simpleNode->ninputs(); ++i)
        {
          input_cycles.push_back(output_cycles[simpleNode->input(i)->origin()]);
          frontier.erase(simpleNode->input(i));
        }
        std::vector<size_t> out_cycles = NodeCycles(simpleNode, input_cycles);

        if (top_muxes.find(simpleNode) != top_muxes.end())
        {
          if (dynamic_cast<const MuxOperation *>(&simpleNode->GetOperation()))
          {
            // TODO: do this in NodeCycles instead?
            // this works for most cases, but is not ideal if the backedge has an II > 1, and the
            // predicate hasn't
            auto pred_latency = output_cycles[simpleNode->input(0)->origin()];
            auto input_latency = output_cycles[simpleNode->input(1)->origin()];
            auto backedge_latency = output_cycles[simpleNode->input(2)->origin()];
            auto out_latency = backedge_latency - pred_latency + input_latency;
            std::cout << "top_mux " << simpleNode << " pred latency: " << pred_latency
                      << " input latency: " << input_latency
                      << " backedge latency: " << backedge_latency
                      << " out latency: " << out_latency << std::endl;
            output_cycles[simpleNode->output(0)] = out_latency;
          }
          else
          {
            JLM_ASSERT(rvsdg::is<LoopConstantBufferOperation>(simpleNode->GetOperation()));
            // don't update output cycles
          }
        }
        else
        {
          for (size_t i = 0; i < simpleNode->noutputs(); ++i)
          {
            auto out = simpleNode->output(i);
            output_cycles[out] = out_cycles[i];
            frontier.insert(&out->SingleUser());
          }
        }
        changed = true;
        break;
      }
      else if (auto be = dynamic_cast<backedge_result *>(in))
      {
        frontier.erase(in);
        auto out = be->argument();
        if (stream_backedges.find(be) == stream_backedges.end())
        {
          // skip stream backedges
          output_cycles[out] = output_cycles[in->origin()];
          frontier.insert(&out->SingleUser());
        }
        changed = true;
        break;
      }
      else if (auto rr = dynamic_cast<rvsdg::RegionResult *>(in))
      {
        frontier.erase(in);
        auto out = rr->output();
        JLM_ASSERT(out);
        output_cycles[out] = output_cycles[in->origin()];
        // don't continue frontier out of loop
        changed = true;
        break;
      }
      else
      {
        auto inner_loop = rvsdg::TryGetOwnerNode<LoopNode>(*in);
        JLM_ASSERT(inner_loop);
        bool all_contained = true;
        for (size_t i = 0; i < inner_loop->ninputs(); ++i)
        {
          auto f = frontier.find(inner_loop->input(i));
          if (f == frontier.end())
          {
            all_contained = false;
          }
        }
        if (!all_contained)
          continue;
        for (size_t i = 0; i < inner_loop->ninputs(); ++i)
        {
          frontier.erase(inner_loop->input(i));
        }
        // TODO: do we just want the latency of a single iteration here?
        CalculateLoopCycleDepth(inner_loop, output_cycles, true);
        for (size_t i = 0; i < inner_loop->noutputs(); ++i)
        {
          std::cout << "output latency " << i << " " << output_cycles[inner_loop->output(i)]
                    << std::endl;
          frontier.insert(&inner_loop->output(i)->SingleUser());
        }
        changed = true;
        break;
      }
    }
  } while (changed);
  // TODO: is "changed" even necessary or can we just wait for frontier to be empty?
  if (!frontier.empty())
  {
    std::unordered_map<rvsdg::Output *, std::string> o_color;
    std::unordered_map<rvsdg::Input *, std::string> i_color;
    for (auto i : frontier)
    {
      i_color.insert({ i, "red" });
    }
  }
  JLM_ASSERT(frontier.empty());
}

void
CalculateLoopCycleDepth(
    LoopNode * loop,
    std::unordered_map<rvsdg::Output *, size_t> & output_cycles,
    bool analyze_inner_loop)
{
  if (!analyze_inner_loop)
  {
    for (size_t i = 0; i < loop->ninputs(); ++i)
    {
      auto in = loop->input(i);
      output_cycles[in->origin()] = 0;
    }
  }
  std::unordered_set<rvsdg::Input *> frontier;
  std::unordered_set<backedge_result *> stream_backedges;
  std::unordered_set<rvsdg::SimpleNode *> top_muxes;
  CreateLoopFrontier(loop, output_cycles, frontier, stream_backedges, top_muxes);
  std::unordered_set<rvsdg::Input *> frontier2(frontier);
  std::cout << "CalculateLoopCycleDepth(" << loop << ", " << analyze_inner_loop << ")" << std::endl;
  /* the reason for having two iterations here is a loop value being updated at the end of the loop,
   * for example the nextRow in SPMV. In theory more iterations could be necessary, until things
   * only increase by the iterative intensity.
   */
  // TODO: should there be more iterations of this? We could iterate until there is no more change
  // in the difference. This would also give us the II
  PushCycleFrontier(output_cycles, frontier, stream_backedges, top_muxes);

  std::unordered_map<rvsdg::Output *, std::string> o_color;
  std::unordered_map<rvsdg::Input *, std::string> i_color;
  std::unordered_map<rvsdg::Output *, std::string> tail_label;
  if (!analyze_inner_loop)
  {
    for (auto i : frontier2)
    {
      i_color.insert({ i, "red" });
    }
    for (auto [o, l] : output_cycles)
    {
      tail_label[o] = std::to_string(l);
    }
  }
  std::cout << "second iteration" << std::endl;
  PushCycleFrontier(output_cycles, frontier2, stream_backedges, top_muxes);
  if (!analyze_inner_loop)
  {
    for (auto [o, l] : output_cycles)
    {
      tail_label[o] = std::to_string(l);
    }
  }
}

void
setMemoryLatency(size_t memoryLatency)
{
  MemoryLatency = memoryLatency;
}

const size_t MaximumBufferSize = 512;

size_t
PlaceBufferLoop(rvsdg::Output * out, size_t min_capacity, bool passThrough)
{
  // places or re-places a buffer on an output
  // don't place buffers after constants
  JLM_ASSERT(!is_constant(rvsdg::TryGetOwnerNode<rvsdg::SimpleNode>(*out)));
  auto [forkNode, forkOperation] = rvsdg::TryGetSimpleNodeAndOp<ForkOperation>(*out);
  JLM_ASSERT(!(forkOperation && forkOperation->IsConstant()));

  if (rvsdg::is<rvsdg::LambdaOperation>(out->region()->node()))
  {
    // don't place buffers outside loops
    return min_capacity;
  }

  auto arg = dynamic_cast<rvsdg::RegionArgument *>(out);
  if (arg && arg->input())
  {
    return PlaceBufferLoop(arg->input()->origin(), min_capacity, passThrough);
  }

  // push buf above loop_const_buf
  if (auto [loopConstantNode, op] = rvsdg::TryGetSimpleNodeAndOp<LoopConstantBufferOperation>(*out);
      op)
  {
    return std::min(
        PlaceBufferLoop(loopConstantNode->input(0)->origin(), min_capacity, passThrough),
        PlaceBufferLoop(loopConstantNode->input(1)->origin(), min_capacity, passThrough));
  }

  if (auto [node, bufferOperation] = rvsdg::TryGetSimpleNodeAndOp<BufferOperation>(*out);
      bufferOperation)
  {
    // replace buffer and keep larger size
    passThrough = passThrough && bufferOperation->IsPassThrough();
    size_t capacity = round_up_pow2(bufferOperation->Capacity() + min_capacity);
    // if the maximum buffer size is exceeded place a smaller buffer, but pretend a large one was
    // placed, to prevent additional buffers further down
    auto actual_capacity = std::min(capacity, MaximumBufferSize);
    auto bufOut =
        BufferOperation::create(*node->input(0)->origin(), actual_capacity, passThrough)[0];
    node->output(0)->divert_users(bufOut);
    JLM_ASSERT(node->IsDead());
    remove(node);
    return capacity;
  }
  else
  {
    // create new buffer
    auto & directUser = *out->Users().begin();
    size_t capacity = round_up_pow2(min_capacity);
    // if the maximum buffer size is exceeded place a smaller buffer, but pretend a large one was
    // placed, to prevent additional buffers further down
    auto actual_capacity = std::min(capacity, MaximumBufferSize);
    auto newOut = BufferOperation::create(*out, actual_capacity, passThrough)[0];
    directUser.divert_to(newOut);
    return capacity;
  }
}

void
AdjustLoopBuffers(
    LoopNode * loop,
    std::unordered_map<rvsdg::Output *, size_t> & output_cycles,
    std::unordered_map<rvsdg::Output *, size_t> & buffer_capacity,
    bool analyze_inner_loop = false)
{
  if (!analyze_inner_loop)
  {
    for (size_t i = 0; i < loop->ninputs(); ++i)
    {
      auto in = loop->input(i);
      buffer_capacity[in->origin()] = 0;
    }
  }
  std::unordered_set<rvsdg::Input *> frontier;
  std::unordered_set<backedge_result *> stream_backedges;
  std::unordered_set<rvsdg::SimpleNode *> top_muxes;
  CreateLoopFrontier(loop, buffer_capacity, frontier, stream_backedges, top_muxes);
  // set buffer capacity for constant nodes to max
  for (auto & tn : loop->subregion()->TopNodes())
  {
    auto out = tn.output(0);
    // don't use size_t max, since that is used to signal down below
    buffer_capacity[out] = UnlimitedBufferCapacity;
  }
  // same for inputs - we only care what happens within the loop
  for (size_t i = 0; i < loop->ninputs(); ++i)
  {
    auto in = loop->input(i);
    buffer_capacity[in->origin()] = UnlimitedBufferCapacity;
    buffer_capacity[in->arguments.begin().ptr()] = UnlimitedBufferCapacity;
  }
  // TODO: unlimited buffers for stream backedges, but loose that property if a fork is reached?
  // this might also make the current special case handling of addrqs unnecessary

  std::unordered_map<rvsdg::Output *, std::string> o_color;
  std::unordered_map<rvsdg::Input *, std::string> i_color;
  std::unordered_map<rvsdg::Output *, std::string> tail_label;
  if (!analyze_inner_loop)
  {
    for (auto i : frontier)
    {
      i_color.insert({ i, "red" });
    }
    for (auto [o, l] : buffer_capacity)
    {
      tail_label[o] = std::to_string(l);
    }
  }

  bool changed = false;
  do
  {
    changed = false;
    for (auto in : frontier)
    {
      if (auto simpleNode = rvsdg::TryGetOwnerNode<rvsdg::SimpleNode>(*in))
      {
        bool all_contained = true;
        for (size_t i = 0; i < simpleNode->ninputs(); ++i)
        {
          auto f = frontier.find(simpleNode->input(i));
          if (f == frontier.end())
          {
            all_contained = false;
          }
        }
        if (!all_contained)
          continue;
        // all inputs of node are in frontier - move them forward
        size_t max_cycles = 0;
        for (size_t i = 0; i < simpleNode->ninputs(); ++i)
        {
          max_cycles = std::max(max_cycles, output_cycles[simpleNode->input(i)->origin()]);
          frontier.erase(simpleNode->input(i));
        }

        std::vector<size_t> input_capacities;
        // adjust capacities
        for (size_t i = 0; i < simpleNode->ninputs(); ++i)
        {
          auto capacity = buffer_capacity[simpleNode->input(i)->origin()];
          if (!analyze_inner_loop && (!rvsdg::is<AddressQueueOperation>(simpleNode))
              && capacity < max_cycles)
          {
            size_t capacity_diff = max_cycles - capacity;
            capacity += PlaceBufferLoop(simpleNode->input(i)->origin(), capacity_diff, true);
            buffer_capacity[simpleNode->input(i)->origin()] = capacity;
          }
          input_capacities.push_back(capacity);
        }

        if (top_muxes.find(simpleNode) != top_muxes.end())
        {
          // we reached our starting point again
          auto mux = dynamic_cast<const MuxOperation *>(&simpleNode->GetOperation());
          if (mux)
          {
            std::cout << "top_mux " << simpleNode
                      << " pred capacity: " << buffer_capacity[simpleNode->input(0)->origin()]
                      << " backedge capacity: " << buffer_capacity[simpleNode->input(2)->origin()]
                      << std::endl;
          }
        }
        else
        {
          std::vector<size_t> out_capacities = NodeCapacity(simpleNode, input_capacities);
          for (size_t i = 0; i < simpleNode->noutputs(); ++i)
          {
            auto out = simpleNode->output(i);
            buffer_capacity[out] = out_capacities[i];
            JLM_ASSERT(analyze_inner_loop || buffer_capacity[out] >= output_cycles[out]);
            frontier.insert(&out->SingleUser());
          }
        }
        changed = true;
        break;
      }
      else if (auto be = dynamic_cast<backedge_result *>(in))
      {
        frontier.erase(in);
        auto out = be->argument();
        buffer_capacity[out] = buffer_capacity[in->origin()];
        if (stream_backedges.find(be) == stream_backedges.end())
        {
          frontier.insert(&out->SingleUser());
        }
        changed = true;
        break;
      }
      else if (auto rr = dynamic_cast<rvsdg::RegionResult *>(in))
      {
        frontier.erase(in);
        auto out = rr->output();
        JLM_ASSERT(out);
        buffer_capacity[out] = buffer_capacity[in->origin()];
        // don't continue frontier out of loop
        changed = true;
        break;
      }
      else
      {
        auto inner_loop = rvsdg::TryGetOwnerNode<LoopNode>(*in);
        JLM_ASSERT(inner_loop);
        bool all_contained = true;
        for (size_t i = 0; i < inner_loop->ninputs(); ++i)
        {
          auto f = frontier.find(inner_loop->input(i));
          if (f == frontier.end())
          {
            all_contained = false;
          }
        }
        if (!all_contained)
          continue;
        // all inputs of node are in frontier - move them forward
        size_t max_cycles = 0;
        for (size_t i = 0; i < inner_loop->ninputs(); ++i)
        {
          max_cycles = std::max(max_cycles, output_cycles[inner_loop->input(i)->origin()]);
          frontier.erase(inner_loop->input(i));
        }
        // adjust capacities
        for (size_t i = 0; i < inner_loop->ninputs(); ++i)
        {
          auto capacity = buffer_capacity[inner_loop->input(i)->origin()];
          if (!analyze_inner_loop && capacity < max_cycles)
          {
            auto user = &inner_loop->input(i)->arguments.begin().ptr()->SingleUser();
            auto [muxNode, muxOperation] = rvsdg::TryGetSimpleNodeAndOp<MuxOperation>(*user);
            if (auto [node, op] = rvsdg::TryGetSimpleNodeAndOp<LoopConstantBufferOperation>(*user);
                (muxOperation && muxOperation->loop) || op)
            {
              size_t capacity_diff = max_cycles - capacity;
              capacity += PlaceBufferLoop(inner_loop->input(i)->origin(), capacity_diff, true);
              buffer_capacity[inner_loop->input(i)->origin()] = capacity;
            }
            else
            {
              // don't put buffers on decouples, streams, and addrq stuff
            }
          }
        }

        AdjustLoopBuffers(inner_loop, output_cycles, buffer_capacity, true);
        for (size_t i = 0; i < inner_loop->noutputs(); ++i)
        {
          frontier.insert(&inner_loop->output(i)->SingleUser());
        }
        changed = true;
        break;
      }
    }
  } while (changed);
  // TODO: is "changed" even necessary or can we just wait for frontier to be empty?
  // benefit of it is we won't get infinite loop in case something violates this
  JLM_ASSERT(frontier.empty());
  // TODO: take iterative intensity into account. E.g. we can use half the buffer capacity if II is
  // 2
  // TODO: remove buffers on cycles?
  // TODO: don't place buf2 on const buf if it gos up to another const buf - even through a branch?
  // TODO: still run buffer resize pass, but without upsizing?

  if (!analyze_inner_loop)
  {
    for (auto [o, l] : buffer_capacity)
    {
      tail_label[o] = std::to_string(l);
    }
  }
}

void
CalculateLoopDepths(rvsdg::Region * region)
{
  for (auto node : rvsdg::TopDownTraverser(region))
  {
    if (auto loop = dynamic_cast<LoopNode *>(node))
    {
      // process inner loops first
      CalculateLoopDepths(loop->subregion());
      std::unordered_map<rvsdg::Output *, size_t> output_cycles;
      CalculateLoopCycleDepth(loop, output_cycles);
      std::unordered_map<rvsdg::Output *, size_t> buffer_capacity;
      AdjustLoopBuffers(loop, output_cycles, buffer_capacity);
    }
  }
}

void
add_buffers(llvm::RvsdgModule & rm)
{
  auto & graph = rm.Rvsdg();
  auto root = &graph.GetRootRegion();
  auto lambda = dynamic_cast<rvsdg::LambdaNode *>(root->Nodes().begin().ptr());
  AddBuffers(lambda->subregion());
  MaximizeBuffers(lambda->subregion());
  CalculateLoopDepths(lambda->subregion());
}

}
