/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_RVSDG_SIMPLE_NORMAL_FORM_HPP
#define JLM_RVSDG_SIMPLE_NORMAL_FORM_HPP

#include <jlm/rvsdg/node-normal-form.hpp>

namespace jlm::rvsdg
{

class SimpleOperation;

class simple_normal_form : public node_normal_form
{
public:
  virtual ~simple_normal_form() noexcept;

  simple_normal_form(
      const std::type_info & operator_class,
      jlm::rvsdg::node_normal_form * parent,
      Graph * graph) noexcept;

  virtual bool
  normalize_node(Node * node) const override;

  virtual std::vector<jlm::rvsdg::output *>
  normalized_create(
      rvsdg::Region * region,
      const SimpleOperation & op,
      const std::vector<jlm::rvsdg::output *> & arguments) const;

  virtual void
  set_cse(bool enable);

  inline bool
  get_cse() const noexcept
  {
    return enable_cse_;
  }

private:
  bool enable_cse_;
};

}

#endif
