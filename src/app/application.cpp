#include "sampan/app/application.hpp"

#include <QApplication>
#include <QString>

#include <optional>
#include <string>
#include <utility>

#include "sampan/app/browser_window.hpp"

namespace sampan::app {

int run(int argc, char *argv[], std::optional<std::string> initial_location) {
  QApplication application{argc, argv};

  std::optional<QString> requested_location;
  if (initial_location.has_value()) {
    requested_location = QString::fromStdString(*initial_location);
  }

  BrowserWindow window{std::move(requested_location)};
  window.show();
  return application.exec();
}

} // namespace sampan::app
