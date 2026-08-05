#include "jsb_settings_tree_model.h"

#include "ah_common/settings.hpp"

#include <functional>

JsbSettingsTreeModel::JsbSettingsTreeModel(const ah::Settings* settings, QObject* parent) : QAbstractItemModel(parent) {
  root_ = std::make_unique<Node>("root");
  if (settings) {
    buildTree(settings);
  }
}

void JsbSettingsTreeModel::buildTree(const ah::Settings* settings) {
  const ah::Settings::Node* jsb = settings->JsbSim().Tree();
  if (!jsb) {
    return;
  }

  // Mirror ah::Settings::Node (values + children) into the UI tree.
  std::function<void(const ah::Settings::Node&, Node*)> walk;
  walk = [&](const ah::Settings::Node& src, Node* parent_ui) {
    for (const auto& kv : src.values) {
      parent_ui->children.push_back(
          std::make_unique<Node>(QString::fromStdString(kv.first), QString::fromStdString(kv.second), parent_ui));
    }
    for (const auto& ch : src.children) {
      auto section = std::make_unique<Node>(QString::fromStdString(ch.first), QString(), parent_ui);
      Node* section_ptr = section.get();
      parent_ui->children.push_back(std::move(section));
      walk(ch.second, section_ptr);
    }
  };

  walk(*jsb, root_.get());
}

JsbSettingsTreeModel::Node* JsbSettingsTreeModel::getNode(const QModelIndex& index) {
  if (!index.isValid()) {
    return nullptr;
  }
  return static_cast<Node*>(index.internalPointer());
}

QModelIndex JsbSettingsTreeModel::index(int row, int column, const QModelIndex& parent) const {
  if (!hasIndex(row, column, parent)) {
    return {};
  }

  Node* parentNode = parent.isValid() ? getNode(parent) : root_.get();
  if (!parentNode || row < 0 || static_cast<size_t>(row) >= parentNode->children.size()) {
    return {};
  }

  Node* child = parentNode->children[static_cast<size_t>(row)].get();
  return createIndex(row, column, child);
}

QModelIndex JsbSettingsTreeModel::parent(const QModelIndex& index) const {
  if (!index.isValid()) {
    return {};
  }

  Node* childNode = getNode(index);
  if (!childNode || childNode->parent == root_.get() || !childNode->parent) {
    return {};
  }

  Node* parentNode = childNode->parent;
  Node* grandParent = parentNode->parent ? parentNode->parent : root_.get();
  for (int r = 0; r < static_cast<int>(grandParent->children.size()); ++r) {
    if (grandParent->children[static_cast<size_t>(r)].get() == parentNode) {
      return createIndex(r, 0, parentNode);
    }
  }
  return {};
}

int JsbSettingsTreeModel::rowCount(const QModelIndex& parent) const {
  Node* parentNode = parent.isValid() ? getNode(parent) : root_.get();
  if (!parentNode) {
    return 0;
  }
  return static_cast<int>(parentNode->children.size());
}

int JsbSettingsTreeModel::columnCount(const QModelIndex& /*parent*/) const {
  return 2;
}

QVariant JsbSettingsTreeModel::data(const QModelIndex& index, int role) const {
  Node* node = getNode(index);
  if (!node) {
    return {};
  }

  const bool isNameCol = (index.column() == 0);

  switch (role) {
    case Qt::DisplayRole:
      return isNameCol ? node->name : node->value;
    case static_cast<int>(CustomRoles::NameRole):
      return node->name;
    case static_cast<int>(CustomRoles::ValueRole):
      return node->value;
    default:
      return {};
  }
}

QVariant JsbSettingsTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
    if (section == 0)
      return QStringLiteral("Setting");
    if (section == 1)
      return QStringLiteral("Value");
  }
  return {};
}

QHash<int, QByteArray> JsbSettingsTreeModel::roleNames() const {
  return {{Qt::DisplayRole, "display"},
          {static_cast<int>(CustomRoles::NameRole), "name"},
          {static_cast<int>(CustomRoles::ValueRole), "value"}};
}
