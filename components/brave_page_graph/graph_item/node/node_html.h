/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_HTML_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_HTML_H_

#include <string>
#include "brave/components/brave_page_graph/graph_item/node.h"
#include "brave/components/brave_page_graph/types.h"

using ::std::string;

namespace brave_page_graph {

class NodeHTML : public Node {
 public:
  NodeHTML() = delete;
  NodeHTML(const PageGraphId id, const DOMNodeID node_id,
    const string& tag_name);
  explicit NodeHTML(const NodeHTML& node) = default;
  ~NodeHTML() = default;
  string ItemName() const;

 protected:
  string ToStringBody() const;
  const DOMNodeId node_id_;
  const string tag_name_;
};

}  // namespace brave_page_graph

#endif BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_HTML_H_
