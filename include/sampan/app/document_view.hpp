#pragma once

#include <QAbstractScrollArea>

#include <memory>

class QResizeEvent;
class QShowEvent;

namespace sampan::layout {

struct LayoutBox;

}

namespace sampan::node {

struct Tree;

}

namespace sampan::app {

class DocumentView final : public QAbstractScrollArea {
public:
  explicit DocumentView(std::unique_ptr<node::Tree> tree,
                        QWidget *parent = nullptr);
  ~DocumentView() override;

  void set_tree(std::unique_ptr<node::Tree> tree);

protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void showEvent(QShowEvent *event) override;
  void scrollContentsBy(int dx, int dy) override;

private:
  void rebuild_layout();

  std::unique_ptr<node::Tree> tree_;
  std::unique_ptr<layout::LayoutBox> layout_;
  bool layout_dirty_ = true;
  bool rebuilding_layout_ = false;
};

} // namespace sampan::app
