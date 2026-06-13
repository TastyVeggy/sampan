#pragma once

#include <QAbstractScrollArea>

#include <memory>

#include "sampan/node/node.hpp"

namespace sampan::app {

class DocumentView final : public QAbstractScrollArea {
public:
  explicit DocumentView(std::unique_ptr<node::Tree> tree,
                        QWidget *parent = nullptr);

protected:
  void paintEvent(QPaintEvent *event) override;
  void scrollContentsBy(int dx, int dy) override;

private:
  std::unique_ptr<node::Tree> tree_;
};

} // namespace sampan::app
