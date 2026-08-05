#include "jsb_settings_tree_model.h"

#include "ah_common/settings.hpp"

#include <QMap>

JsbSettingsTreeModel::JsbSettingsTreeModel(const ah::Settings* settings, QObject* parent)
    : QAbstractItemModel(parent) {
  root_ = std::make_unique<Node>("root");
  if (settings) {
    buildTree(settings);
  }
}

void JsbSettingsTreeModel::buildTree(const ah::Settings* settings) {
  const auto entries = settings->entriesWithPrefix("JSBSim/");
  QMap<QString, Node*> sectionMap;

  for (const auto& kv : entries) {
    const QString fullKey = QString::fromStdString(kv.first);
    if (!fullKey.startsWith(QStringLiteral("JSBSim/"))) {
      continue;
    }
    const QString subKey = fullKey.mid(7);  // strip "JSBSim/"
    const QStringList parts = subKey.split(QLatin1Char('/'));
    if (parts.isEmpty()) {
      continue;
    }

    const QString& sectionName = parts.first();
    Node* sectionNode = sectionMap.value(sectionName, nullptr);
    if (!sectionNode) {
      sectionNode = new Node(sectionName, QString(), root_.get());
      root_->children.emplace_back(sectionNode);
      sectionMap.insert(sectionName, sectionNode);
    }

    QString leafName;
    if (parts.size() > 1) {
      leafName = parts.mid(1).join(QLatin1Char('/'));
    } else {
      leafName = sectionName;
    }

    const QString val = QString::fromStdString(kv.second);
    auto leaf = std::make_unique<Node>(leafName, val, sectionNode);
    sectionNode->children.push_back(std::move(leaf));
  }
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
  // Find the row of this parent under its parent (grandparent)
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
    if (section == 0) return QStringLiteral("Setting");
    if (section == 1) return QStringLiteral("Value");
  }
  return {};
}

QHash<int, QByteArray> JsbSettingsTreeModel::roleNames() const {
  return {
    {Qt::DisplayRole, "display"},
    {static_cast<int>(CustomRoles::NameRole), "name"},
    {static_cast<int>(CustomRoles::ValueRole), "value"}
  };
}