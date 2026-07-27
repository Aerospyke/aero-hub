#pragma once

#include <QAbstractItemModel>
#include <QSettings>
#include <memory>
#include <vector>

class JsbSettingsTreeModel final : public QAbstractItemModel {
  Q_OBJECT

 public:
  explicit JsbSettingsTreeModel(QSettings* settings, QObject* parent = nullptr);
  JsbSettingsTreeModel() = delete;

  // QAbstractItemModel overrides
  [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  [[nodiscard]] QModelIndex parent(const QModelIndex& index) const override;
  [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
  [[nodiscard]] int columnCount(const QModelIndex& parent) const override;
  [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  enum CustomRoles : uint16_t {
    NameRole = Qt::UserRole + 1,
    ValueRole
  };

 private:
  struct Node {
    QString name;
    QString value;
    std::vector<std::unique_ptr<Node>> children;
    Node* parent = nullptr;

    Node() = default;
    explicit Node(QString n, QString v = QString(), Node* p = nullptr)
        : name(std::move(n)), value(std::move(v)), parent(p) {}
  };

  std::unique_ptr<Node> root_;

  void buildTree(QSettings* settings);
  static Node* getNode(const QModelIndex& index);
};