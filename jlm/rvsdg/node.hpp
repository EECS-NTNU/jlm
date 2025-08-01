/*
 * Copyright 2010 2011 2012 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2011 2012 2013 2014 2015 2016 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_RVSDG_NODE_HPP
#define JLM_RVSDG_NODE_HPP

#include <jlm/rvsdg/operation.hpp>
#include <jlm/util/common.hpp>
#include <jlm/util/intrusive-list.hpp>
#include <jlm/util/iterator_range.hpp>
#include <jlm/util/IteratorWrapper.hpp>
#include <jlm/util/strfmt.hpp>

#include <unordered_set>
#include <utility>
#include <variant>

namespace jlm::rvsdg
{
namespace base
{
class type;
}

class Graph;
class Output;
class SubstitutionMap;

class Input
{
  friend class Node;
  friend class rvsdg::Region;

public:
  virtual ~Input() noexcept;

  Input(rvsdg::Node & owner, rvsdg::Output & origin, std::shared_ptr<const rvsdg::Type> type);

  Input(rvsdg::Region & owner, rvsdg::Output & origin, std::shared_ptr<const rvsdg::Type> type);

  Input(const Input &) = delete;

  Input(Input &&) = delete;

  Input &
  operator=(const Input &) = delete;

  Input &
  operator=(Input &&) = delete;

  inline size_t
  index() const noexcept
  {
    return index_;
  }

  jlm::rvsdg::Output *
  origin() const noexcept
  {
    return origin_;
  }

  void
  divert_to(jlm::rvsdg::Output * new_origin);

  [[nodiscard]] const std::shared_ptr<const rvsdg::Type> &
  Type() const noexcept
  {
    return Type_;
  }

  [[nodiscard]] rvsdg::Region *
  region() const noexcept;

  virtual std::string
  debug_string() const;

  [[nodiscard]] std::variant<Node *, Region *>
  GetOwner() const noexcept
  {
    return Owner_;
  }

  class Iterator final
  {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Input *;
    using difference_type = std::ptrdiff_t;
    using pointer = Input **;
    using reference = Input *&;

    constexpr explicit Iterator(Input * input)
        : Input_(input)
    {}

    [[nodiscard]] Input *
    GetInput() const noexcept
    {
      return Input_;
    }

    Input &
    operator*()
    {
      JLM_ASSERT(Input_ != nullptr);
      return *Input_;
    }

    Input *
    operator->() const
    {
      return Input_;
    }

    Iterator &
    operator++()
    {
      Input_ = ComputeNext();
      return *this;
    }

    Iterator
    operator++(int)
    {
      Iterator tmp = *this;
      ++*this;
      return tmp;
    }

    bool
    operator==(const Iterator & other) const
    {
      return Input_ == other.Input_;
    }

    bool
    operator!=(const Iterator & other) const
    {
      return !operator==(other);
    }

  private:
    [[nodiscard]] Input *
    ComputeNext() const;

    Input * Input_;
  };

  class ConstIterator final
  {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const Input *;
    using difference_type = std::ptrdiff_t;
    using pointer = const Input **;
    using reference = const Input *&;

    constexpr explicit ConstIterator(const Input * input)
        : Input_(input)
    {}

    [[nodiscard]] const Input *
    GetInput() const noexcept
    {
      return Input_;
    }

    const Input &
    operator*()
    {
      JLM_ASSERT(Input_ != nullptr);
      return *Input_;
    }

    const Input *
    operator->() const
    {
      return Input_;
    }

    ConstIterator &
    operator++()
    {
      Input_ = ComputeNext();
      return *this;
    }

    ConstIterator
    operator++(int)
    {
      ConstIterator tmp = *this;
      ++*this;
      return tmp;
    }

    bool
    operator==(const ConstIterator & other) const
    {
      return Input_ == other.Input_;
    }

    bool
    operator!=(const ConstIterator & other) const
    {
      return !operator==(other);
    }

  private:
    [[nodiscard]] Input *
    ComputeNext() const;

    const Input * Input_;
  };

private:
  static void
  CheckTypes(
      const Region & region,
      const Output & origin,
      const std::shared_ptr<const rvsdg::Type> & type);

  size_t index_;
  jlm::rvsdg::Output * origin_;
  std::variant<Node *, Region *> Owner_;
  std::shared_ptr<const rvsdg::Type> Type_;
};

template<class T>
static inline bool
is(const jlm::rvsdg::Input & input) noexcept
{
  static_assert(
      std::is_base_of<jlm::rvsdg::Input, T>::value,
      "Template parameter T must be derived from jlm::rvsdg::input.");

  return dynamic_cast<const T *>(&input) != nullptr;
}

class Output
{
  friend Input;
  friend class Node;
  friend class rvsdg::Region;

public:
  using UserIterator = util::PtrIterator<Input, std::unordered_set<Input *>::iterator>;
  using UserConstIterator =
      util::PtrIterator<const Input, std::unordered_set<Input *>::const_iterator>;

  using UserIteratorRange = util::IteratorRange<UserIterator>;
  using UserConstIteratorRange = util::IteratorRange<UserConstIterator>;

  virtual ~Output() noexcept;

  Output(rvsdg::Node & owner, std::shared_ptr<const rvsdg::Type> type);

  Output(rvsdg::Region * owner, std::shared_ptr<const rvsdg::Type> type);

  Output(const Output &) = delete;

  Output(Output &&) = delete;

  Output &
  operator=(const Output &) = delete;

  Output &
  operator=(Output &&) = delete;

  inline size_t
  index() const noexcept
  {
    return index_;
  }

  inline size_t
  nusers() const noexcept
  {
    return users_.size();
  }

  /**
   * Determines whether the output is dead.
   *
   * An output is considered dead when it has no users.
   *
   * @return True, if the output is dead, otherwise false.
   *
   * \see nusers()
   */
  [[nodiscard]] bool
  IsDead() const noexcept
  {
    return nusers() == 0;
  }

  inline void
  divert_users(jlm::rvsdg::Output * new_origin)
  {
    if (this == new_origin)
      return;

    while (users_.size())
      (*users_.begin())->divert_to(new_origin);
  }

  /**
   * @return The first and only user of the output.
   *
   * \pre The output has only a single user.
   */
  [[nodiscard]] rvsdg::Input &
  SingleUser() const noexcept
  {
    JLM_ASSERT(nusers() == 1);
    return **users_.begin();
  }

  UserIteratorRange
  Users()
  {
    return { UserIterator(users_.begin()), UserIterator(users_.end()) };
  }

  [[nodiscard]] UserConstIteratorRange
  Users() const
  {
    return { UserConstIterator(users_.begin()), UserConstIterator(users_.end()) };
  }

  [[nodiscard]] const std::shared_ptr<const rvsdg::Type> &
  Type() const noexcept
  {
    return Type_;
  }

  [[nodiscard]] rvsdg::Region *
  region() const noexcept;

  virtual std::string
  debug_string() const;

  [[nodiscard]] std::variant<Node *, Region *>
  GetOwner() const noexcept
  {
    return Owner_;
  }

  class Iterator final
  {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Output;
    using difference_type = std::ptrdiff_t;
    using pointer = Output *;
    using reference = Output &;

    constexpr explicit Iterator(Output * output)
        : Output_(output)
    {}

    [[nodiscard]] Output *
    GetOutput() const noexcept
    {
      return Output_;
    }

    Output &
    operator*()
    {
      JLM_ASSERT(Output_ != nullptr);
      return *Output_;
    }

    Output *
    operator->() const
    {
      return Output_;
    }

    Iterator &
    operator++()
    {
      Output_ = ComputeNext();
      return *this;
    }

    Iterator
    operator++(int)
    {
      Iterator tmp = *this;
      ++*this;
      return tmp;
    }

    bool
    operator==(const Iterator & other) const
    {
      return Output_ == other.Output_;
    }

    bool
    operator!=(const Iterator & other) const
    {
      return !operator==(other);
    }

  private:
    [[nodiscard]] Output *
    ComputeNext() const;

    Output * Output_;
  };

  class ConstIterator final
  {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const Output;
    using difference_type = std::ptrdiff_t;
    using pointer = const Output *;
    using reference = const Output &;

    constexpr explicit ConstIterator(const Output * output)
        : Output_(output)
    {}

    const Output *
    GetOutput() const noexcept
    {
      return Output_;
    }

    const Output &
    operator*() const
    {
      JLM_ASSERT(Output_ != nullptr);
      return *Output_;
    }

    const Output *
    operator->() const
    {
      return Output_;
    }

    ConstIterator &
    operator++()
    {
      Output_ = ComputeNext();
      return *this;
    }

    ConstIterator
    operator++(int)
    {
      const ConstIterator tmp = *this;
      ++*this;
      return tmp;
    }

    bool
    operator==(const ConstIterator & other) const
    {
      return Output_ == other.Output_;
    }

    bool
    operator!=(const ConstIterator & other) const
    {
      return !operator==(other);
    }

  private:
    [[nodiscard]] Output *
    ComputeNext() const;

    const Output * Output_;
  };

private:
  void
  remove_user(jlm::rvsdg::Input * user);

  void
  add_user(jlm::rvsdg::Input * user);

  size_t index_;
  std::variant<Node *, Region *> Owner_;
  std::shared_ptr<const rvsdg::Type> Type_;
  std::unordered_set<jlm::rvsdg::Input *> users_;
};

template<class T>
static inline bool
is(const jlm::rvsdg::Output * output) noexcept
{
  static_assert(
      std::is_base_of<jlm::rvsdg::Output, T>::value,
      "Template parameter T must be derived from jlm::rvsdg::output.");

  return dynamic_cast<const T *>(output) != nullptr;
}

/* node_input class */

class node_input : public jlm::rvsdg::Input
{
public:
  node_input(jlm::rvsdg::Output * origin, Node * node, std::shared_ptr<const rvsdg::Type> type);

  Node *
  node() const noexcept
  {
    auto owner = GetOwner();
    if (auto node = std::get_if<Node *>(&owner))
    {
      return *node;
    }

    JLM_UNREACHABLE("This should not have happened!");
  }
};

/* node_output class */

class node_output : public jlm::rvsdg::Output
{
public:
  node_output(Node * node, std::shared_ptr<const rvsdg::Type> type);

  [[nodiscard]] Node *
  node() const noexcept
  {
    return node_;
  }

private:
  Node * node_;
};

/* node class */

class Node
{
public:
  using InputIteratorRange = util::IteratorRange<Input::Iterator>;
  using InputConstIteratorRange = util::IteratorRange<Input::ConstIterator>;
  using OutputIteratorRange = util::IteratorRange<Output::Iterator>;
  using OutputConstIteratorRange = util::IteratorRange<Output::ConstIterator>;

  virtual ~Node();

  explicit Node(Region * region);

  [[nodiscard]] virtual const Operation &
  GetOperation() const noexcept = 0;

  inline size_t
  ninputs() const noexcept
  {
    return inputs_.size();
  }

  node_input *
  input(size_t index) const noexcept
  {
    JLM_ASSERT(index < ninputs());
    return inputs_[index].get();
  }

  [[nodiscard]] InputIteratorRange
  Inputs() noexcept
  {
    return { Input::Iterator(input(0)), Input::Iterator(nullptr) };
  }

  [[nodiscard]] InputConstIteratorRange
  Inputs() const noexcept
  {
    return { Input::ConstIterator(input(0)), Input::ConstIterator(nullptr) };
  }

  inline size_t
  noutputs() const noexcept
  {
    return outputs_.size();
  }

  node_output *
  output(size_t index) const noexcept
  {
    JLM_ASSERT(index < noutputs());
    return outputs_[index].get();
  }

  [[nodiscard]] OutputIteratorRange
  Outputs() noexcept
  {
    return { Output::Iterator(output(0)), Output::Iterator(nullptr) };
  }

  [[nodiscard]] OutputConstIteratorRange
  Outputs() const noexcept
  {
    return { Output::ConstIterator(output(0)), Output::ConstIterator(nullptr) };
  }

  inline void
  recompute_depth() noexcept;

  /**
   * \brief Determines whether the node is dead.
   *
   * A node is considered dead if all its outputs are dead.
   *
   * @return True, if the node is dead, otherwise false.
   *
   * \see output::IsDead()
   */
  [[nodiscard]] bool
  IsDead() const noexcept
  {
    for (auto & output : outputs_)
    {
      if (!output->IsDead())
        return false;
    }

    return true;
  }

  virtual std::string
  DebugString() const = 0;

protected:
  node_input *
  add_input(std::unique_ptr<node_input> input);

  /**
   * Removes an input from the node given the inputs' index.
   *
   * The removal of an input invalidates the node's existing input iterators.
   *
   * @param index The inputs' index. It must be between [0, ninputs()).
   *
   * \note The method must adjust the indices of the other inputs after the removal. Moreover, it
   * also might need to recompute the depth of the node.
   *
   * \see ninputs()
   * \see recompute_depth()
   * \see input#index()
   */
  void
  RemoveInput(size_t index);

  // FIXME: I really would not like to be RemoveInputsWhere() to be public
public:
  /**
   * Removes all inputs that match the condition specified by \p match.
   *
   * @tparam F A type that supports the function call operator: bool operator(const node_input&)
   * @param match Defines the condition for the inputs to remove.
   */
  template<typename F>
  void
  RemoveInputsWhere(const F & match)
  {
    // iterate backwards to avoid the invalidation of 'n' by RemoveInput()
    for (size_t n = ninputs() - 1; n != static_cast<size_t>(-1); n--)
    {
      auto & input = *Node::input(n);
      if (match(input))
      {
        RemoveInput(n);
      }
    }
  }

protected:
  node_output *
  add_output(std::unique_ptr<node_output> output)
  {
    output->index_ = noutputs();
    outputs_.push_back(std::move(output));
    return this->output(noutputs() - 1);
  }

  /**
   * Removes an output from the node given the outputs' index.
   *
   * An output can only be removed, if it has no users. The removal of an output invalidates the
   * node's existing output iterators.
   *
   * @param index The outputs' index. It must be between [0, noutputs()).
   *
   * \note The method must adjust the indices of the other outputs after the removal. The methods'
   * runtime is therefore O(n), where n is the node's number of outputs.
   *
   * \see noutputs()
   * \see output#index()
   * \see output#nusers()
   */
  void
  RemoveOutput(size_t index);

  // FIXME: I really would not like to be RemoveOutputsWhere() to be public
public:
  /**
   * Removes all outputs that have no users and match the condition specified by \p match.
   *
   * @tparam F A type that supports the function call operator: bool operator(const node_output&)
   * @param match Defines the condition for the outputs to remove.
   *
   * \see output#nusers()
   */
  template<typename F>
  void
  RemoveOutputsWhere(const F & match)
  {
    // iterate backwards to avoid the invalidation of 'n' by RemoveOutput()
    for (size_t n = noutputs() - 1; n != static_cast<size_t>(-1); n--)
    {
      auto & output = *Node::output(n);
      if (output.nusers() == 0 && match(output))
      {
        RemoveOutput(n);
      }
    }
  }

public:
  [[nodiscard]] Graph *
  graph() const noexcept
  {
    return graph_;
  }

  [[nodiscard]] rvsdg::Region *
  region() const noexcept
  {
    return region_;
  }

  virtual Node *
  copy(rvsdg::Region * region, const std::vector<jlm::rvsdg::Output *> & operands) const;

  /**
    \brief Copy a node with substitutions
    \param region Target region to create node in
    \param smap Operand substitutions
    \return Copied node

    Create a new node that is semantically equivalent to an
    existing node. The newly created node will use the same
    operands as the existing node unless there is a substitution
    registered for a particular operand.

    The given substitution map is updated so that all
    outputs of the original node will be substituted by
    corresponding outputs of the newly created node in
    subsequent \ref copy operations.
  */
  virtual Node *
  copy(rvsdg::Region * region, SubstitutionMap & smap) const = 0;

  inline size_t
  depth() const noexcept
  {
    return depth_;
  }

private:
  util::intrusive_list_anchor<Node> region_node_list_anchor_{};

  util::intrusive_list_anchor<Node> region_top_node_list_anchor_{};

  util::intrusive_list_anchor<Node> region_bottom_node_list_anchor_{};

public:
  typedef util::intrusive_list_accessor<Node, &Node::region_node_list_anchor_>
      region_node_list_accessor;

  typedef util::intrusive_list_accessor<Node, &Node::region_top_node_list_anchor_>
      region_top_node_list_accessor;

  typedef util::intrusive_list_accessor<Node, &Node::region_bottom_node_list_anchor_>
      region_bottom_node_list_accessor;

private:
  size_t depth_;
  Graph * graph_;
  rvsdg::Region * region_;
  std::vector<std::unique_ptr<node_input>> inputs_;
  std::vector<std::unique_ptr<node_output>> outputs_;
};

/**
 * \brief Checks if this is an input to a node of specified type.
 *
 * \tparam NodeType
 *   The node type to be matched against.
 *
 * \param input
 *   Input to be checked.
 *
 * \returns
 *   Owning node of requested type or nullptr.
 *
 * Checks if the specified input belongs to a node of requested type.
 * If this is the case, returns a pointer to the node of matched type.
 * If this is not the case (because either this as a region exit
 * result or its owning node is not of the requested type), returns
 * nullptr.
 *
 * See \ref def_use_inspection.
 */
template<typename NodeType>
inline NodeType *
TryGetOwnerNode(const rvsdg::Input & input) noexcept
{
  auto owner = input.GetOwner();
  if (const auto node = std::get_if<Node *>(&owner))
  {
    return dynamic_cast<NodeType *>(*node);
  }
  else
  {
    return nullptr;
  }
}

/**
 * \brief Checks if this is an output to a node of specified type.
 *
 * \tparam NodeType
 *   The node type to be matched against.
 *
 * \param output
 *   Output to be checked.
 *
 * \returns
 *   Owning node of requested type or nullptr.
 *
 * Checks if the specified output belongs to a node of requested type.
 * If this is the case, returns a pointer to the node of matched type.
 * If this is not the case (because either this as a region entry
 * argument or its owning node is not of the requested type), returns
 * nullptr.
 *
 * See \ref def_use_inspection.
 */
template<typename NodeType>
inline NodeType *
TryGetOwnerNode(const rvsdg::Output & output) noexcept
{
  auto owner = output.GetOwner();
  if (const auto node = std::get_if<Node *>(&owner))
  {
    return dynamic_cast<NodeType *>(*node);
  }
  else
  {
    return nullptr;
  }
}

/**
 * \brief Asserts that this is an input to a node of specified type.
 *
 * \tparam NodeType
 *   The node type to be matched against.
 *
 * \param input
 *   Input to be checked.
 *
 * \returns
 *   Owning node of requested type.
 *
 * Checks if the specified input belongs to a node of requested type.
 * If this is the case, returns a reference to the node of matched type,
 * otherwise throws std::logic_error.
 *
 * See \ref def_use_inspection.
 */
template<typename NodeType>
inline NodeType &
AssertGetOwnerNode(const rvsdg::Input & input)
{
  auto node = TryGetOwnerNode<NodeType>(input);
  if (!node)
  {
    throw std::logic_error(std::string("expected node of type ") + typeid(NodeType).name());
  }
  return *node;
}

/**
 * \brief Asserts that this is an output of a node of specified type.
 *
 * \tparam NodeType
 *   The node type to be matched against.
 *
 * \param output
 *   Output to be checked.
 *
 * \returns
 *   Owning node of requested type.
 *
 * Checks if the specified output belongs to a node of requested type.
 * If this is the case, returns a reference to the node of matched type,
 * otherwise throws std::logic_error.
 *
 * See \ref def_use_inspection.
 */
template<typename NodeType>
inline NodeType &
AssertGetOwnerNode(const rvsdg::Output & output)
{
  auto node = TryGetOwnerNode<NodeType>(output);
  if (!node)
  {
    throw std::logic_error(std::string("expected node of type ") + typeid(NodeType).name());
  }
  return *node;
}

inline Region *
TryGetOwnerRegion(const rvsdg::Input & input) noexcept
{
  auto owner = input.GetOwner();
  if (auto region = std::get_if<Region *>(&owner))
  {
    return *region;
  }
  else
  {
    return nullptr;
  }
}

inline Region *
TryGetOwnerRegion(const rvsdg::Output & output) noexcept
{
  auto owner = output.GetOwner();
  if (auto region = std::get_if<Region *>(&owner))
  {
    return *region;
  }
  else
  {
    return nullptr;
  }
}

static inline std::vector<jlm::rvsdg::Output *>
operands(const Node * node)
{
  std::vector<jlm::rvsdg::Output *> operands;
  for (size_t n = 0; n < node->ninputs(); n++)
    operands.push_back(node->input(n)->origin());
  return operands;
}

static inline std::vector<jlm::rvsdg::Output *>
outputs(const Node * node)
{
  std::vector<jlm::rvsdg::Output *> outputs;
  for (size_t n = 0; n < node->noutputs(); n++)
    outputs.push_back(node->output(n));
  return outputs;
}

static inline void
divert_users(Node * node, const std::vector<Output *> & outputs)
{
  JLM_ASSERT(node->noutputs() == outputs.size());

  for (size_t n = 0; n < outputs.size(); n++)
    node->output(n)->divert_users(outputs[n]);
}

template<class T>
static inline bool
is(const Node * node) noexcept
{
  if (!node)
    return false;

  return is<T>(node->GetOperation());
}

Node *
producer(const jlm::rvsdg::Output * output) noexcept;

}

#endif
