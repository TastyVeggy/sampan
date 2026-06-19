#pragma once

#include <QMainWindow>
#include <QString>

#include <cstddef>
#include <optional>
#include <vector>

#include "sampan/app/document_loader.hpp"

class QLabel;
class QLineEdit;
class QPushButton;

namespace sampan::app {

class DocumentView;

class BrowserWindow final : public QMainWindow {
public:
  explicit BrowserWindow(std::optional<QString> initial_location = std::nullopt,
                         QWidget *parent = nullptr);
  ~BrowserWindow() override;

  BrowserWindow(const BrowserWindow &) = delete;
  BrowserWindow &operator=(const BrowserWindow &) = delete;
  BrowserWindow(BrowserWindow &&) = delete;
  BrowserWindow &operator=(BrowserWindow &&) = delete;

  void navigate(const QString &location);

  [[nodiscard]] QString current_location() const;
  [[nodiscard]] QString error_message() const;
  [[nodiscard]] bool is_home() const noexcept;
  [[nodiscard]] bool is_loading() const noexcept;
  [[nodiscard]] DocumentView *document_view() const noexcept;

private:
  void navigate(const QString &location, bool add_to_history,
                std::optional<std::size_t> history_target = std::nullopt);
  void navigate_history(std::size_t target);
  void compile_and_commit(LoadedDocument loaded, bool add_to_history,
                          std::optional<std::size_t> history_target);
  void commit_home(bool add_to_history,
                   std::optional<std::size_t> history_target = std::nullopt);
  void push_history(const QString &location);
  void update_history_buttons();
  void set_loading(bool loading);
  void show_error(const QString &message);
  void clear_error();
  void update_title();

  DocumentLoader loader_;
  QWidget *navigation_;
  QPushButton *back_button_;
  QPushButton *forward_button_;
  QPushButton *reload_button_;
  QLineEdit *address_;
  QPushButton *go_button_;
  QLabel *error_;
  DocumentView *document_view_;
  QString current_location_;
  std::vector<QString> history_;
  std::size_t history_index_ = 0;
};

} // namespace sampan::app
