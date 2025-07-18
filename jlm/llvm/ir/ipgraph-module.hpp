/*
 * Copyright 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_LLVM_IR_IPGRAPH_MODULE_HPP
#define JLM_LLVM_IR_IPGRAPH_MODULE_HPP

#include <jlm/llvm/ir/basic-block.hpp>
#include <jlm/llvm/ir/ipgraph.hpp>
#include <jlm/llvm/ir/tac.hpp>

#include <jlm/util/file.hpp>

namespace jlm::llvm
{

class GlobalValue final : public GlobalVariable
{
public:
  ~GlobalValue() noexcept override;

  explicit GlobalValue(DataNode * node)
      : GlobalVariable(node->Type(), node->name()),
        node_(node)
  {}

  GlobalValue(const GlobalValue &) = delete;

  GlobalValue(GlobalValue &&) = delete;

  GlobalValue &
  operator=(const GlobalValue &) = delete;

  GlobalValue &
  operator=(GlobalValue &&) = delete;

  DataNode *
  node() const noexcept
  {
    return node_;
  }

private:
  DataNode * node_;
};

static inline std::unique_ptr<GlobalValue>
create_gblvalue(DataNode * node)
{
  return std::make_unique<GlobalValue>(node);
}

class InterProceduralGraphModule final
{
  typedef std::unordered_set<const GlobalValue *>::const_iterator const_iterator;

public:
  ~InterProceduralGraphModule() noexcept = default;

  InterProceduralGraphModule(
      const jlm::util::FilePath & source_filename,
      const std::string & target_triple,
      const std::string & data_layout,
      std::vector<std::unique_ptr<StructType::Declaration>> declarations) noexcept
      : data_layout_(data_layout),
        target_triple_(target_triple),
        source_filename_(source_filename),
        StructTypeDeclarations_(std::move(declarations))
  {}

  InterProceduralGraphModule(
      const jlm::util::FilePath & source_filename,
      const std::string & target_triple,
      const std::string & data_layout) noexcept
      : InterProceduralGraphModule(source_filename, target_triple, data_layout, {})
  {}

  InterProceduralGraph &
  ipgraph() noexcept
  {
    return clg_;
  }

  const InterProceduralGraph &
  ipgraph() const noexcept
  {
    return clg_;
  }

  const_iterator
  begin() const
  {
    return globals_.begin();
  }

  const_iterator
  end() const
  {
    return globals_.end();
  }

  GlobalValue *
  create_global_value(DataNode * node)
  {
    auto v = llvm::create_gblvalue(node);
    auto ptr = v.get();
    globals_.insert(ptr);
    functions_[node] = ptr;
    variables_.insert(std::move(v));
    return ptr;
  }

  inline llvm::Variable *
  create_variable(std::shared_ptr<const jlm::rvsdg::Type> type, const std::string & name)
  {
    auto v = std::make_unique<llvm::Variable>(std::move(type), name);
    auto pv = v.get();
    variables_.insert(std::move(v));
    return pv;
  }

  inline llvm::Variable *
  create_variable(std::shared_ptr<const jlm::rvsdg::Type> type)
  {
    static uint64_t c = 0;
    auto v = std::make_unique<llvm::Variable>(std::move(type), jlm::util::strfmt("v", c++));
    auto pv = v.get();
    variables_.insert(std::move(v));
    return pv;
  }

  inline llvm::Variable *
  create_variable(FunctionNode * node)
  {
    JLM_ASSERT(!variable(node));

    auto v = std::unique_ptr<llvm::Variable>(new fctvariable(node));
    auto pv = v.get();
    functions_[node] = pv;
    variables_.insert(std::move(v));
    return pv;
  }

  const llvm::Variable *
  variable(const InterProceduralGraphNode * node) const noexcept
  {
    auto it = functions_.find(node);
    return it != functions_.end() ? it->second : nullptr;
  }

  const jlm::util::FilePath &
  source_filename() const noexcept
  {
    return source_filename_;
  }

  inline const std::string &
  target_triple() const noexcept
  {
    return target_triple_;
  }

  inline const std::string &
  data_layout() const noexcept
  {
    return data_layout_;
  }

  /**
   * Adds struct type declarations to the module. The module becomes the ownwer of the declarations.
   *
   * @param declarations The declarations added to the module
   */
  void
  SetStructTypeDeclarations(std::vector<std::unique_ptr<StructType::Declaration>> && declarations)
  {
    StructTypeDeclarations_ = std::move(declarations);
  }

  /**
   * Releases all struct type declarations from the module to the caller. The caller is the new
   * owner of the declarations.
   *
   * @return A vector of declarations.
   */
  std::vector<std::unique_ptr<StructType::Declaration>> &&
  ReleaseStructTypeDeclarations()
  {
    return std::move(StructTypeDeclarations_);
  }

  static std::unique_ptr<InterProceduralGraphModule>
  Create(
      const jlm::util::FilePath & sourceFilename,
      const std::string & targetTriple,
      const std::string & dataLayout,
      std::vector<std::unique_ptr<StructType::Declaration>> declarations)
  {
    return std::make_unique<InterProceduralGraphModule>(
        sourceFilename,
        targetTriple,
        dataLayout,
        std::move(declarations));
  }

  static std::unique_ptr<InterProceduralGraphModule>
  create(
      const jlm::util::FilePath & source_filename,
      const std::string & target_triple,
      const std::string & data_layout)
  {
    return Create(source_filename, target_triple, data_layout, {});
  }

private:
  InterProceduralGraph clg_;
  std::string data_layout_;
  std::string target_triple_;
  const jlm::util::FilePath source_filename_;
  std::unordered_set<const GlobalValue *> globals_;
  std::unordered_set<std::unique_ptr<llvm::Variable>> variables_;
  std::unordered_map<const InterProceduralGraphNode *, const llvm::Variable *> functions_;
  std::vector<std::unique_ptr<StructType::Declaration>> StructTypeDeclarations_;
};

static inline size_t
ntacs(const InterProceduralGraphModule & im)
{
  size_t ntacs = 0;
  for (const auto & n : im.ipgraph())
  {
    auto f = dynamic_cast<const FunctionNode *>(&n);
    if (!f)
      continue;

    auto cfg = f->cfg();
    if (!cfg)
      continue;

    for (const auto & node : *f->cfg())
    {
      if (auto bb = dynamic_cast<const BasicBlock *>(&node))
        ntacs += bb->tacs().ntacs();
    }
  }

  return ntacs;
}

}

#endif
