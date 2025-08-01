/*
 * Copyright 2021 David Metz <david.c.metz@ntnu.no>
 * See COPYING for terms of redistribution.
 */

#include <jlm/hls/backend/rvsdg2rhls/add-buffers.hpp>
#include <jlm/hls/backend/rvsdg2rhls/add-forks.hpp>
#include <jlm/hls/backend/rvsdg2rhls/add-prints.hpp>
#include <jlm/hls/backend/rvsdg2rhls/add-sinks.hpp>
#include <jlm/hls/backend/rvsdg2rhls/alloca-conv.hpp>
#include <jlm/hls/backend/rvsdg2rhls/check-rhls.hpp>
#include <jlm/hls/backend/rvsdg2rhls/decouple-mem-state.hpp>
#include <jlm/hls/backend/rvsdg2rhls/distribute-constants.hpp>
#include <jlm/hls/backend/rvsdg2rhls/GammaConversion.hpp>
#include <jlm/hls/backend/rvsdg2rhls/instrument-ref.hpp>
#include <jlm/hls/backend/rvsdg2rhls/mem-conv.hpp>
#include <jlm/hls/backend/rvsdg2rhls/mem-queue.hpp>
#include <jlm/hls/backend/rvsdg2rhls/mem-sep.hpp>
#include <jlm/hls/backend/rvsdg2rhls/memstate-conv.hpp>
#include <jlm/hls/backend/rvsdg2rhls/merge-gamma.hpp>
#include <jlm/hls/backend/rvsdg2rhls/remove-redundant-buf.hpp>
#include <jlm/hls/backend/rvsdg2rhls/rhls-dne.hpp>
#include <jlm/hls/backend/rvsdg2rhls/rvsdg2rhls.hpp>
#include <jlm/hls/backend/rvsdg2rhls/stream-conv.hpp>
#include <jlm/hls/backend/rvsdg2rhls/ThetaConversion.hpp>
#include <jlm/hls/backend/rvsdg2rhls/UnusedStateRemoval.hpp>
#include <jlm/hls/opt/cne.hpp>
#include <jlm/hls/opt/InvariantLambdaMemoryStateRemoval.hpp>
#include <jlm/hls/opt/IOBarrierRemoval.hpp>
#include <jlm/hls/util/view.hpp>
#include <jlm/llvm/backend/IpGraphToLlvmConverter.hpp>
#include <jlm/llvm/backend/RvsdgToIpGraphConverter.hpp>
#include <jlm/llvm/ir/CallSummary.hpp>
#include <jlm/llvm/ir/operators/alloca.hpp>
#include <jlm/llvm/ir/operators/call.hpp>
#include <jlm/llvm/ir/operators/delta.hpp>
#include <jlm/llvm/ir/operators/IntegerOperations.hpp>
#include <jlm/llvm/ir/RvsdgModule.hpp>
#include <jlm/llvm/opt/alias-analyses/Optimization.hpp>
#include <jlm/llvm/opt/DeadNodeElimination.hpp>
#include <jlm/llvm/opt/inlining.hpp>
#include <jlm/llvm/opt/InvariantValueRedirection.hpp>
#include <jlm/llvm/opt/inversion.hpp>
#include <jlm/llvm/opt/reduction.hpp>
#include <jlm/rvsdg/traverser.hpp>
#include <jlm/rvsdg/view.hpp>
#include <jlm/util/Statistics.hpp>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>

#include <regex>

namespace jlm::hls
{

void
split_opt(llvm::RvsdgModule & rm)
{
  // TODO: figure out which optimizations to use here
  jlm::llvm::DeadNodeElimination dne;
  jlm::hls::cne cne;
  jlm::llvm::InvariantValueRedirection ivr;
  jlm::llvm::LoopUnswitching tgi;
  jlm::llvm::NodeReduction red;
  jlm::util::StatisticsCollector statisticsCollector;
  tgi.Run(rm, statisticsCollector);
  dne.Run(rm, statisticsCollector);
  cne.Run(rm, statisticsCollector);
  ivr.Run(rm, statisticsCollector);
  red.Run(rm, statisticsCollector);
  dne.Run(rm, statisticsCollector);
}

void
pre_opt(jlm::llvm::RvsdgModule & rm)
{
  // TODO: figure out which optimizations to use here
  jlm::llvm::DeadNodeElimination dne;
  jlm::hls::cne cne;
  jlm::llvm::InvariantValueRedirection ivr;
  jlm::llvm::LoopUnswitching tgi;
  jlm::util::StatisticsCollector statisticsCollector;
  tgi.Run(rm, statisticsCollector);
  dne.Run(rm, statisticsCollector);
  cne.Run(rm, statisticsCollector);
  ivr.Run(rm, statisticsCollector);
  dne.Run(rm, statisticsCollector);
  cne.Run(rm, statisticsCollector);
  dne.Run(rm, statisticsCollector);
}

void
dump_xml(llvm::RvsdgModule & rvsdgModule, const std::string & file_name)
{
  auto xml_file = fopen(file_name.c_str(), "w");
  jlm::rvsdg::view_xml(&rvsdgModule.Rvsdg().GetRootRegion(), xml_file);
  fclose(xml_file);
}

bool
function_match(rvsdg::LambdaNode * ln, const std::string & function_name)
{
  const std::regex fn_regex(function_name);
  if (std::regex_match(
          dynamic_cast<llvm::LlvmLambdaOperation &>(ln->GetOperation()).name(),
          fn_regex))
  { // TODO: handle C++ name mangling
    return true;
  }
  return false;
}

const jlm::rvsdg::Output *
trace_call(jlm::rvsdg::Input * input)
{
  auto graph = input->region()->graph();

  auto argument = dynamic_cast<const rvsdg::RegionArgument *>(input->origin());
  const rvsdg::Output * result = nullptr;
  if (auto theta = rvsdg::TryGetOwnerNode<rvsdg::ThetaNode>(*input->origin()))
  {
    result = trace_call(theta->MapOutputLoopVar(*input->origin()).input);
  }
  else if (argument == nullptr)
  {
    result = input->origin();
  }
  else if (argument->region() == &graph->GetRootRegion())
  {
    result = argument;
  }
  else
  {
    JLM_ASSERT(argument->input() != nullptr);
    result = trace_call(argument->input());
  }
  return result;
}

void
inline_calls(rvsdg::Region * region)
{
  for (auto & node : rvsdg::TopDownTraverser(region))
  {
    if (auto structnode = dynamic_cast<rvsdg::StructuralNode *>(node))
    {
      for (size_t n = 0; n < structnode->nsubregions(); n++)
      {
        inline_calls(structnode->subregion(n));
      }
    }
    else if (dynamic_cast<const llvm::CallOperation *>(&(node->GetOperation())))
    {
      auto traced = jlm::hls::trace_call(node->input(0));
      auto so = dynamic_cast<const rvsdg::StructuralOutput *>(traced);
      if (!so)
      {
        if (auto graphImport = dynamic_cast<const llvm::GraphImport *>(traced))
        {
          if (graphImport->Name().rfind("decouple_", 0) == 0)
          {
            // can't inline pseudo functions used for decoupling
            continue;
          }
          if (graphImport->Name().rfind("hls_", 0) == 0)
          {
            // can't inline pseudo functions used for streaming
            continue;
          }
          throw util::Error("can not inline external function " + graphImport->Name());
        }
      }
      JLM_ASSERT(rvsdg::is<rvsdg::LambdaOperation>(so->node()));
      auto ln = dynamic_cast<const rvsdg::StructuralOutput *>(traced)->node();
      llvm::inlineCall(
          dynamic_cast<jlm::rvsdg::SimpleNode *>(node),
          dynamic_cast<const rvsdg::LambdaNode *>(ln));
      // restart for this region
      inline_calls(region);
      return;
    }
  }
}

size_t alloca_cnt = 0;

void
convert_alloca(rvsdg::Region * region)
{
  for (auto & node : rvsdg::TopDownTraverser(region))
  {
    if (auto structnode = dynamic_cast<rvsdg::StructuralNode *>(node))
    {
      for (size_t n = 0; n < structnode->nsubregions(); n++)
      {
        convert_alloca(structnode->subregion(n));
      }
    }
    else if (auto po = dynamic_cast<const llvm::AllocaOperation *>(&(node->GetOperation())))
    {
      auto rr = &region->graph()->GetRootRegion();
      auto delta_name = jlm::util::strfmt("hls_alloca_", alloca_cnt++);
      auto delta_type = llvm::PointerType::Create();
      std::cout << "alloca " << delta_name << ": " << po->value_type().debug_string() << "\n";
      auto db = llvm::DeltaNode::Create(
          rr,
          std::static_pointer_cast<const rvsdg::ValueType>(po->ValueType()),
          delta_name,
          llvm::linkage::external_linkage,
          "",
          false);
      // create zero constant of allocated type
      rvsdg::Output * cout = nullptr;
      if (auto bt = dynamic_cast<const llvm::IntegerConstantOperation *>(&po->value_type()))
      {
        cout = llvm::IntegerConstantOperation::Create(
                   *db->subregion(),
                   bt->Representation().nbits(),
                   0)
                   .output(0);
      }
      else
      {
        cout = llvm::ConstantAggregateZeroOperation::Create(*db->subregion(), po->ValueType());
      }
      auto delta = &db->finalize(cout);
      jlm::llvm::GraphExport::Create(*delta, delta_name);
      auto delta_local = route_to_region_rvsdg(delta, region);
      node->output(0)->divert_users(delta_local);
      // TODO: check that the input to alloca is a bitconst 1
      // TODO: handle general case of other nodes getting state edge without a merge
      JLM_ASSERT(node->output(1)->nusers() == 1);
      auto & mux_in = *node->output(1)->Users().begin();
      auto mux_node = rvsdg::TryGetOwnerNode<rvsdg::Node>(mux_in);
      if (dynamic_cast<const llvm::MemoryStateMergeOperation *>(&mux_node->GetOperation()))
      {
        // merge after alloca -> remove merge
        JLM_ASSERT(mux_node->ninputs() == 2);
        auto other_index = mux_in.index() ? 0 : 1;
        mux_node->output(0)->divert_users(mux_node->input(other_index)->origin());
        jlm::rvsdg::remove(mux_node);
      }
      else
      {
        // TODO: fix this properly by adding a state edge to the LambdaEntryMemState and routing it
        // to the region
        JLM_ASSERT(false);
      }
      jlm::rvsdg::remove(node);
    }
  }
}

llvm::DeltaNode *
rename_delta(llvm::DeltaNode * odn)
{
  auto name = odn->name();
  std::replace_if(
      name.begin(),
      name.end(),
      [](char c)
      {
        return c == '.';
      },
      '_');
  std::cout << "renaming delta node " << odn->name() << " to " << name << "\n";
  auto db = llvm::DeltaNode::Create(
      odn->region(),
      std::static_pointer_cast<const rvsdg::ValueType>(odn->Type()),
      name,
      llvm::linkage::external_linkage,
      "",
      odn->constant());
  /* add dependencies */
  rvsdg::SubstitutionMap rmap;
  for (auto ctxVar : odn->GetContextVars())
  {
    auto input = ctxVar.input;
    auto nd = db->AddContextVar(*input->origin()).inner;
    rmap.insert(ctxVar.inner, nd);
  }

  /* copy subregion */
  odn->subregion()->copy(db->subregion(), rmap, false, false);

  auto result = rmap.lookup(odn->subregion()->result(0)->origin());
  auto data = &db->finalize(result);

  odn->output().divert_users(data);
  jlm::rvsdg::remove(odn);
  return rvsdg::TryGetOwnerNode<llvm::DeltaNode>(*data);
}

rvsdg::LambdaNode *
change_linkage(rvsdg::LambdaNode * ln, llvm::linkage link)
{
  const auto & op = dynamic_cast<llvm::LlvmLambdaOperation &>(ln->GetOperation());
  auto lambda = rvsdg::LambdaNode::Create(
      *ln->region(),
      llvm::LlvmLambdaOperation::Create(op.Type(), op.name(), link, op.attributes()));

  /* add context variables */
  rvsdg::SubstitutionMap subregionmap;
  for (const auto & cv : ln->GetContextVars())
  {
    auto origin = cv.input->origin();
    auto newcv = lambda->AddContextVar(*origin);
    subregionmap.insert(cv.inner, newcv.inner);
  }
  /* collect function arguments */
  auto args = ln->GetFunctionArguments();
  auto newArgs = lambda->GetFunctionArguments();
  JLM_ASSERT(args.size() == newArgs.size());
  for (std::size_t n = 0; n < args.size(); ++n)
  {
    subregionmap.insert(args[n], newArgs[n]);
  }

  /* copy subregion */
  ln->subregion()->copy(lambda->subregion(), subregionmap, false, false);

  /* collect function results */
  std::vector<jlm::rvsdg::Output *> results;
  for (auto result : ln->GetFunctionResults())
    results.push_back(subregionmap.lookup(result->origin()));

  /* finalize lambda */
  lambda->finalize(results);

  divert_users(ln, outputs(lambda));
  jlm::rvsdg::remove(ln);

  return lambda;
}

std::unique_ptr<jlm::llvm::RvsdgModule>
split_hls_function(llvm::RvsdgModule & rm, const std::string & function_name)
{
  // TODO: use a different datastructure for rhls?
  // create a copy of rm
  auto rhls = llvm::RvsdgModule::Create(rm.SourceFileName(), rm.TargetTriple(), rm.DataLayout());
  std::cout << "processing " << rm.SourceFileName().name() << "\n";
  auto root = &rm.Rvsdg().GetRootRegion();
  for (auto node : rvsdg::TopDownTraverser(root))
  {
    if (auto ln = dynamic_cast<rvsdg::LambdaNode *>(node))
    {
      if (!function_match(ln, function_name))
      {
        continue;
      }
      inline_calls(ln->subregion());
      split_opt(rm);
      //            convert_alloca(ln->subregion());
      rvsdg::SubstitutionMap smap;
      for (size_t i = 0; i < ln->ninputs(); ++i)
      {
        auto orig_node_output = dynamic_cast<jlm::rvsdg::node_output *>(ln->input(i)->origin());
        if (!orig_node_output)
        {
          // handle decouple stuff
          auto oldGraphImport = dynamic_cast<llvm::GraphImport *>(ln->input(i)->origin());
          auto & newGraphImport = llvm::GraphImport::Create(
              rhls->Rvsdg(),
              oldGraphImport->ValueType(),
              oldGraphImport->ImportedType(),
              oldGraphImport->Name(),
              oldGraphImport->Linkage());
          smap.insert(ln->input(i)->origin(), &newGraphImport);
          continue;
        }
        auto orig_node = orig_node_output->node();
        if (auto oln = dynamic_cast<rvsdg::LambdaNode *>(orig_node))
        {
          throw util::Error(
              "Inlining of function "
              + dynamic_cast<llvm::LlvmLambdaOperation &>(oln->GetOperation()).name()
              + " not supported");
        }
        else if (auto odn = dynamic_cast<llvm::DeltaNode *>(orig_node))
        {
          // modify name to not contain .
          if (odn->name().find('.') != std::string::npos)
          {
            odn = rename_delta(odn);
          }
          std::cout << "delta node " << odn->name() << ": " << odn->Type()->debug_string() << "\n";
          // add import for delta to rhls
          auto & graphImport = llvm::GraphImport::Create(
              rhls->Rvsdg(),
              odn->Type(),
              llvm::PointerType::Create(),
              odn->name(),
              llvm::linkage::external_linkage);
          smap.insert(ln->input(i)->origin(), &graphImport);
          // add export for delta to rm
          // TODO: check if not already exported and maybe adjust linkage?
          jlm::llvm::GraphExport::Create(odn->output(), odn->name());
        }
        else
        {
          throw util::Error("Unsupported node type: " + orig_node->DebugString());
        }
      }
      // copy function into rhls
      auto new_ln = ln->copy(&rhls->Rvsdg().GetRootRegion(), smap);
      new_ln = change_linkage(new_ln, llvm::linkage::external_linkage);
      auto oldExport = jlm::llvm::ComputeCallSummary(*ln).GetRvsdgExport();
      jlm::llvm::GraphExport::Create(*new_ln->output(), oldExport ? oldExport->Name() : "");
      // add function as input to rm and remove it
      const auto & op = dynamic_cast<llvm::LlvmLambdaOperation &>(ln->GetOperation());
      auto & graphImport = llvm::GraphImport::Create(
          rm.Rvsdg(),
          op.Type(),
          op.Type(),
          op.name(),
          llvm::linkage::external_linkage); // TODO: change linkage?
      ln->output()->divert_users(&graphImport);
      remove(ln);
      std::cout << "function "
                << dynamic_cast<llvm::LlvmLambdaOperation &>(new_ln->GetOperation()).name()
                << " extracted for HLS\n";
      return rhls;
    }
  }
  throw util::Error("HLS function " + function_name + " not found");
}

void
rvsdg2ref(llvm::RvsdgModule & rhls, const util::FilePath & path)
{
  dump_ref(rhls, path);
}

void
rvsdg2rhls(llvm::RvsdgModule & rhls, util::StatisticsCollector & collector)
{
  pre_opt(rhls);

  IOBarrierRemoval ioBarrierRemoval;
  ioBarrierRemoval.Run(rhls, collector);

  // TODO: do mem state separation early, so there are no false dependencies between loops
  mem_sep_argument(rhls);
  merge_gamma(rhls);
  RemoveUnusedStates(rhls);

  llvm::DeadNodeElimination llvmDne;
  llvmDne.Run(rhls, collector);
  jlm::llvm::LoopUnswitching tgi;
  // simplify loops
  tgi.Run(rhls, collector);
  jlm::hls::cne cne;
  cne.Run(rhls, collector);
  llvmDne.Run(rhls, collector);
  // merge gammas that were pulled out of loops
  merge_gamma(rhls);
  llvmDne.Run(rhls, collector);
  RemoveUnusedStates(rhls);
  // main conversion steps
  distribute_constants(rhls);
  ConvertGammaNodes(rhls);
  ConvertThetaNodes(rhls);
  cne.Run(rhls, collector);
  // rhls optimization
  dne(rhls);
  alloca_conv(rhls);
  jlm::hls::stream_conv(rhls);
  mem_queue(rhls);
  decouple_mem_state(rhls);
  RemoveUnusedStates(rhls);
  MemoryConverter(rhls);
  llvm::NodeReduction llvmRed;
  llvmRed.Run(rhls, collector);
  MemoryStateSplitConversion::CreateAndRun(rhls, collector);
  RedundantBufferElimination::CreateAndRun(rhls, collector);
  SinkInsertion::CreateAndRun(rhls, collector);
  ForkInsertion::CreateAndRun(rhls, collector);
  add_buffers(rhls);
  // ensure that all rhls rules are met
  check_rhls(rhls);
}

void
dump_ref(llvm::RvsdgModule & rhls, const util::FilePath & path)
{
  auto reference =
      llvm::RvsdgModule::Create(rhls.SourceFileName(), rhls.TargetTriple(), rhls.DataLayout());
  rvsdg::SubstitutionMap smap;
  rhls.Rvsdg().GetRootRegion().copy(&reference->Rvsdg().GetRootRegion(), smap, true, true);
  pre_opt(*reference);
  instrument_ref(*reference);
  for (size_t i = 0; i < reference->Rvsdg().GetRootRegion().narguments(); ++i)
  {
    auto graphImport =
        util::AssertedCast<const llvm::GraphImport>(reference->Rvsdg().GetRootRegion().argument(i));
    std::cout << "impport " << graphImport->Name() << ": " << graphImport->Type()->debug_string()
              << "\n";
  }
  ::llvm::LLVMContext ctx;
  jlm::util::StatisticsCollector statisticsCollector;
  auto jm2 = llvm::RvsdgToIpGraphConverter::CreateAndConvertModule(*reference, statisticsCollector);
  auto lm2 = llvm::IpGraphToLlvmConverter::CreateAndConvertModule(*jm2, ctx);
  std::error_code EC;
  ::llvm::raw_fd_ostream os(path.to_str(), EC);
  lm2->print(os, nullptr);
}

}
