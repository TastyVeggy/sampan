#include <gtest/gtest.h>

#include <QApplication>
#include <QColor>
#include <QImage>
#include <QScrollBar>

#include <memory>
#include <string>
#include <utility>

#include "sampan/analyze/analyzer.hpp"
#include "sampan/app/document_view.hpp"
#include "sampan/parse/parser.hpp"

namespace {

[[nodiscard]] std::unique_ptr<sampan::node::Tree>
analyze_source(std::string source) {
  sampan::parse::ParseResult parsed =
      sampan::parse::parse(std::move(source), 0);
  if (!parsed.diagnostics.empty() || parsed.document == nullptr) {
    return nullptr;
  }
  sampan::analyze::AnalyzeResult analyzed =
      sampan::analyze::analyze(*parsed.document);
  return std::move(analyzed.tree);
}

void settle(sampan::app::DocumentView &view) {
  view.show();
  for (int iteration = 0; iteration < 3; ++iteration) {
    view.viewport()->update();
    QApplication::processEvents();
  }
}

TEST(DocumentView, AutoSizedPageTracksViewportWithoutScrolling) {
  std::unique_ptr<sampan::node::Tree> tree = analyze_source("page {}");
  ASSERT_NE(tree, nullptr);

  sampan::app::DocumentView view{std::move(tree)};
  view.resize(200, 300);
  settle(view);

  EXPECT_EQ(view.horizontalScrollBar()->maximum(), 0);
  EXPECT_EQ(view.verticalScrollBar()->maximum(), 0);
}

TEST(DocumentView, DefaultCanvasMatchesAnUnpaintedPage) {
  std::unique_ptr<sampan::node::Tree> tree =
      analyze_source("page { width: 100px height: 200px }");
  ASSERT_NE(tree, nullptr);

  sampan::app::DocumentView view{std::move(tree)};
  view.resize(200, 300);
  settle(view);

  QImage image{view.viewport()->size(), QImage::Format_ARGB32};
  view.viewport()->render(&image);
  EXPECT_EQ(image.pixelColor(10, 10), QColor{Qt::white});
  EXPECT_EQ(image.pixelColor(image.width() - 10, image.height() - 10),
            QColor{Qt::white});
}

TEST(DocumentView, AddsBothScrollBarsForAnOversizedPage) {
  std::unique_ptr<sampan::node::Tree> tree =
      analyze_source("page { width: 500px height: 600px }");
  ASSERT_NE(tree, nullptr);

  sampan::app::DocumentView view{std::move(tree)};
  view.resize(200, 300);
  settle(view);

  EXPECT_GT(view.horizontalScrollBar()->maximum(), 0);
  EXPECT_GT(view.verticalScrollBar()->maximum(), 0);
}

TEST(DocumentView, ScrollsToDescendantsOutsideAnExplicitPage) {
  std::unique_ptr<sampan::node::Tree> tree =
      analyze_source("page { width: 100px height: 100px "
                     "box { width: 500px height: 600px } }");
  ASSERT_NE(tree, nullptr);

  sampan::app::DocumentView view{std::move(tree)};
  view.resize(200, 300);
  settle(view);

  EXPECT_GT(view.horizontalScrollBar()->maximum(), 0);
  EXPECT_GT(view.verticalScrollBar()->maximum(), 0);

  view.horizontalScrollBar()->setValue(view.horizontalScrollBar()->maximum());
  view.verticalScrollBar()->setValue(view.verticalScrollBar()->maximum());
  QApplication::processEvents();
  EXPECT_EQ(view.horizontalScrollBar()->value(),
            view.horizontalScrollBar()->maximum());
  EXPECT_EQ(view.verticalScrollBar()->value(),
            view.verticalScrollBar()->maximum());
}

} // namespace

int main(int argc, char *argv[]) {
  QApplication application{argc, argv};
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
