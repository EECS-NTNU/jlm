/*
 * Copyright 2013 2014 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/jlm/ir/ipgraph.hpp>
#include <jlm/jlm/ir/cfg.hpp>
#include <jlm/jlm/ir/tac.hpp>

#include <jive/common.h>

#include <stdio.h>

#include <algorithm>

/* Tarjan's SCC algorithm */

static void
strongconnect(
	const jlm::ipgraph_node * node,
	std::unordered_map<const jlm::ipgraph_node*, std::pair<size_t,size_t>> & map,
	std::vector<const jlm::ipgraph_node*> & node_stack,
	size_t & index,
	std::vector<std::unordered_set<const jlm::ipgraph_node*>> & sccs)
{
	map.emplace(node, std::make_pair(index, index));
	node_stack.push_back(node);
	index++;

	for (auto callee : *node) {
		if (map.find(callee) == map.end()) {
			/* successor has not been visited yet; recurse on it */
			strongconnect(callee, map, node_stack, index, sccs);
			map[node].second = std::min(map[node].second, map[callee].second);
		} else if (std::find(node_stack.begin(), node_stack.end(), callee) != node_stack.end()) {
			/* successor is in stack and hence in the current SCC */
			map[node].second = std::min(map[node].second, map[callee].first);
		}
	}

	if (map[node].second == map[node].first) {
		std::unordered_set<const jlm::ipgraph_node*> scc;
		const jlm::ipgraph_node * w;
		do {
			w = node_stack.back();
			node_stack.pop_back();
			scc.insert(w);
		} while (w != node);

		sccs.push_back(scc);
	}
}

namespace jlm {

/* ipgraph */

void
ipgraph::add_node(std::unique_ptr<ipgraph_node> node)
{
	JLM_DEBUG_ASSERT(nodes_.find(node->name()) == nodes_.end());
	nodes_[node->name()] = std::move(node);
}

ipgraph_node *
ipgraph::lookup_node(const std::string & name) const
{
	if (nodes_.find(name) != nodes_.end())
		return nodes_.find(name)->second.get();

	return nullptr;
}

std::vector<ipgraph_node*>
ipgraph::nodes() const
{
	std::vector<ipgraph_node*> v;
	for (auto i = nodes_.begin(); i != nodes_.end(); i++)
		v.push_back(i->second.get());

	return v;
}

std::vector<std::unordered_set<const ipgraph_node*>>
ipgraph::find_sccs() const
{
	std::vector<std::unordered_set<const ipgraph_node*>> sccs;

	std::unordered_map<const ipgraph_node*, std::pair<size_t,size_t>> map;
	std::vector<const ipgraph_node*> node_stack;
	size_t index = 0;

	auto nodes = this->nodes();
	for (auto node : nodes) {
		if (map.find(node) == map.end())
			strongconnect(node, map, node_stack, index, sccs);
	}

	return sccs;
}

/* ipgraph node */

ipgraph_node::~ipgraph_node()
{}

/* function node */

function_node::~function_node()
{}

const std::string &
function_node::name() const noexcept
{
	return name_;
}

const jive::type &
function_node::type() const noexcept
{
	return type_;
}

const jlm::linkage &
function_node::linkage() const noexcept
{
	return linkage_;
}

/* function variable */

fctvariable::~fctvariable()
{}

/* data node */

data_node::~data_node()
{}

const std::string &
data_node::name() const noexcept
{
	return name_;
}

const jive::type &
data_node::type() const noexcept
{
	return *type_;
}

const jlm::linkage &
data_node::linkage() const noexcept
{
	return linkage_;
}

}
