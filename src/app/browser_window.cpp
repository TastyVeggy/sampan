#include "sampan/app/browser_window.hpp"

#include <QByteArray>
#include <QFile>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "sampan/analyze/analyzer.hpp"
#include "sampan/app/document_loader.hpp"
#include "sampan/app/document_view.hpp"
#include "sampan/core/diagnostic.hpp"
#include "sampan/node/node.hpp"
#include "sampan/parse/parser.hpp"

namespace sampan::app {
namespace {

const QString kHomeLocation = QStringLiteral("sampan:home");
const QString kHomeResource = QStringLiteral(":/sampan/home.yl");

void configure_navigation_button(QPushButton &button, const QIcon &icon,
                                 const QString &name) {
  button.setText({});
  button.setIcon(icon);
  button.setToolTip(name);
  button.setAccessibleName(name);
}

void set_navigation_enabled(QPushButton &button, const bool enabled) {
  auto *opacity =
      qobject_cast<QGraphicsOpacityEffect *>(button.graphicsEffect());
  if (opacity == nullptr) {
    opacity = new QGraphicsOpacityEffect{&button};
    button.setGraphicsEffect(opacity);
  }
  opacity->setOpacity(enabled ? 1.0 : 0.35);
  button.setEnabled(enabled);
}

[[nodiscard]] QString
format_diagnostics(const QString &source_name,
                   const std::vector<core::Diagnostic> &diagnostics) {
  QString result;
  for (const core::Diagnostic &diagnostic : diagnostics) {
    if (!result.isEmpty()) {
      result += QLatin1Char('\n');
    }
    result += QStringLiteral("%1:%2:%3: %4: %5")
                  .arg(source_name)
                  .arg(diagnostic.span.start_line)
                  .arg(diagnostic.span.start_column)
                  .arg(QString::fromUtf8(core::to_string(diagnostic.severity)))
                  .arg(QString::fromStdString(diagnostic.message));
  }
  return result;
}

[[nodiscard]] std::unique_ptr<node::Tree> make_home_tree() {
  QFile file{kHomeResource};
  if (!file.open(QIODevice::ReadOnly)) {
    qFatal("failed to open Sampan's embedded home page");
  }

  const QByteArray bytes = file.readAll();
  parse::ParseResult parsed = parse::parse(
      std::string{bytes.constData(), static_cast<std::size_t>(bytes.size())},
      0);
  if (!parsed.diagnostics.empty() || parsed.document == nullptr) {
    qFatal("Sampan's embedded home page does not parse");
  }

  analyze::AnalyzeResult analyzed = analyze::analyze(*parsed.document);
  if (!analyzed.diagnostics.empty() || analyzed.tree == nullptr) {
    qFatal("Sampan's embedded home page does not pass analysis");
  }
  return std::move(analyzed.tree);
}

} // namespace

BrowserWindow::BrowserWindow(std::optional<QString> initial_location,
                             QWidget *parent)
    : QMainWindow{parent}, loader_{this}, navigation_{new QWidget{this}},
      back_button_{new QPushButton{QStringLiteral("Back"), navigation_}},
      forward_button_{new QPushButton{QStringLiteral("Forward"), navigation_}},
      reload_button_{new QPushButton{QStringLiteral("Reload"), navigation_}},
      address_{new QLineEdit{navigation_}},
      go_button_{new QPushButton{QStringLiteral("Go"), navigation_}},
      error_{new QLabel{this}},
      document_view_{new DocumentView{nullptr, this}} {
  setWindowTitle(QStringLiteral("Sampan"));
  resize(1000, 700);

  navigation_->setObjectName(QStringLiteral("navigationBar"));
  auto *navigation_layout = new QHBoxLayout{navigation_};
  navigation_layout->setContentsMargins(8, 6, 8, 6);
  navigation_layout->setSpacing(6);
  back_button_->setObjectName(QStringLiteral("backButton"));
  forward_button_->setObjectName(QStringLiteral("forwardButton"));
  reload_button_->setObjectName(QStringLiteral("reloadButton"));
  configure_navigation_button(*back_button_,
                              style()->standardIcon(QStyle::SP_ArrowBack),
                              QStringLiteral("Back"));
  configure_navigation_button(*forward_button_,
                              style()->standardIcon(QStyle::SP_ArrowForward),
                              QStringLiteral("Forward"));
  configure_navigation_button(*reload_button_,
                              style()->standardIcon(QStyle::SP_BrowserReload),
                              QStringLiteral("Reload"));
  navigation_layout->addWidget(back_button_);
  navigation_layout->addWidget(forward_button_);
  navigation_layout->addWidget(reload_button_);
  address_->setObjectName(QStringLiteral("addressBar"));
  address_->setClearButtonEnabled(true);
  address_->setPlaceholderText(
      QStringLiteral("File path or https://example.com/page.yl"));
  navigation_layout->addWidget(address_, /* stretch */ 1);
  navigation_layout->addWidget(go_button_);

  error_->setObjectName(QStringLiteral("navigationError"));
  error_->setWordWrap(true);
  error_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  error_->setStyleSheet(QStringLiteral(
      "QLabel { background: #ffe8e6; color: #8a1f17; padding: 8px; "
      "border-bottom: 1px solid #e4a19b; }"));
  error_->hide();

  auto *central = new QWidget{this};
  auto *central_layout = new QVBoxLayout{central};
  central_layout->setContentsMargins(0, 0, 0, 0);
  central_layout->setSpacing(0);
  central_layout->addWidget(navigation_);
  central_layout->addWidget(error_);
  central_layout->addWidget(document_view_, 1);
  setCentralWidget(central);

  QObject::connect(address_, &QLineEdit::returnPressed, this,
                   [this] { navigate(address_->text(), true); });
  QObject::connect(go_button_, &QPushButton::clicked, this,
                   [this] { navigate(address_->text(), true); });
  QObject::connect(reload_button_, &QPushButton::clicked, this,
                   [this] { navigate(current_location_, false); });
  QObject::connect(back_button_, &QPushButton::clicked, this, [this] {
    if (history_index_ > 0) {
      navigate_history(history_index_ - 1);
    }
  });
  QObject::connect(forward_button_, &QPushButton::clicked, this, [this] {
    if (history_index_ + 1 < history_.size()) {
      navigate_history(history_index_ + 1);
    }
  });

  const bool starts_at_home =
      !initial_location.has_value() || initial_location->trimmed().isEmpty();
  if (starts_at_home) {
    commit_home(true);
  } else {
    update_history_buttons();
    navigate(*initial_location, true);
  }
}

BrowserWindow::~BrowserWindow() = default;

void BrowserWindow::navigate(const QString &location) {
  navigate(location, true);
}

QString BrowserWindow::current_location() const {
  return current_location_;
}

QString BrowserWindow::error_message() const {
  return error_->text();
}

bool BrowserWindow::is_home() const noexcept {
  return current_location_ == kHomeLocation;
}

bool BrowserWindow::is_loading() const noexcept {
  return loader_.is_loading();
}

DocumentView *BrowserWindow::document_view() const noexcept {
  return document_view_;
}

void BrowserWindow::navigate(const QString &location, const bool add_to_history,
                             const std::optional<std::size_t> history_target) {
  clear_error();
  const QString requested = location.trimmed();
  if (requested.isEmpty() || requested == kHomeLocation) {
    loader_.cancel();
    commit_home(add_to_history, history_target);
    return;
  }

  address_->setText(requested);
  set_loading(true);
  loader_.load(
      requested, [this, add_to_history, history_target](LoadResult result) {
        set_loading(false);
        if (!result.has_value()) {
          show_error(result.error());
          update_title();
          return;
        }
        compile_and_commit(std::move(*result), add_to_history, history_target);
      });
}

void BrowserWindow::navigate_history(const std::size_t target) {
  navigate(history_[target], false, target);
}

void BrowserWindow::compile_and_commit(
    LoadedDocument loaded, const bool add_to_history,
    const std::optional<std::size_t> history_target) {
  const QString source_name = loaded.url.toDisplayString();
  parse::ParseResult parsed = parse::parse(std::move(loaded.source), 0);
  if (!parsed.diagnostics.empty() || parsed.document == nullptr) {
    show_error(format_diagnostics(source_name, parsed.diagnostics));
    update_title();
    return;
  }

  analyze::AnalyzeResult analyzed = analyze::analyze(*parsed.document);
  if (!analyzed.diagnostics.empty() || analyzed.tree == nullptr) {
    show_error(format_diagnostics(source_name, analyzed.diagnostics));
    update_title();
    return;
  }

  document_view_->set_tree(std::move(analyzed.tree));
  current_location_ = loaded.url.toDisplayString(QUrl::FullyEncoded);
  address_->setText(current_location_);
  if (add_to_history) {
    push_history(current_location_);
  } else {
    if (history_target.has_value()) {
      history_index_ = *history_target;
    }
    update_history_buttons();
  }
  clear_error();
  update_title();
}

void BrowserWindow::commit_home(
    const bool add_to_history,
    const std::optional<std::size_t> history_target) {
  if (!is_home()) {
    document_view_->set_tree(make_home_tree());
  }
  current_location_ = kHomeLocation;
  address_->setText(current_location_);
  set_loading(false);
  clear_error();
  if (add_to_history) {
    push_history(current_location_);
  } else {
    if (history_target.has_value()) {
      history_index_ = *history_target;
    }
    update_history_buttons();
  }
  update_title();
}

void BrowserWindow::push_history(const QString &location) {
  if (history_index_ + 1 < history_.size()) {
    history_.erase(history_.begin() +
                       static_cast<std::ptrdiff_t>(history_index_ + 1),
                   history_.end());
  }
  if (!history_.empty() && history_.back() == location) {
    history_index_ = history_.size() - 1;
  } else {
    history_.push_back(location);
    history_index_ = history_.size() - 1;
  }
  update_history_buttons();
}

void BrowserWindow::update_history_buttons() {
  set_navigation_enabled(*back_button_, history_index_ > 0);
  set_navigation_enabled(*forward_button_,
                         history_index_ + 1 < history_.size());
}

void BrowserWindow::set_loading(const bool loading) {
  address_->setEnabled(!loading);
  go_button_->setEnabled(!loading);
  reload_button_->setEnabled(!loading);
  if (loading) {
    setWindowTitle(QStringLiteral("Loading… — Sampan"));
  }
}

void BrowserWindow::show_error(const QString &message) {
  error_->setText(message);
  error_->setVisible(!message.isEmpty());
}

void BrowserWindow::clear_error() {
  error_->clear();
  error_->hide();
}

void BrowserWindow::update_title() {
  setWindowTitle(current_location_.isEmpty() || is_home()
                     ? QStringLiteral("Sampan")
                     : QStringLiteral("%1 — Sampan").arg(current_location_));
}

} // namespace sampan::app
