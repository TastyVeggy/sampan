#include <gtest/gtest.h>

#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QGraphicsOpacityEffect>
#include <QHostAddress>
#include <QPushButton>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QUrl>

#include <functional>

#include "sampan/app/browser_window.hpp"
#include "sampan/app/document_loader.hpp"

namespace {

[[nodiscard]] bool wait_until(const std::function<bool()> &condition) {
  QElapsedTimer timer;
  timer.start();
  while (!condition() && timer.elapsed() < 2'000) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
  }
  return condition();
}

[[nodiscard]] QString write_document(QTemporaryDir &directory,
                                     const QByteArray &source) {
  const QString path = directory.filePath(QStringLiteral("document.yl"));
  QFile file{path};
  if (!file.open(QIODevice::WriteOnly) || file.write(source) != source.size()) {
    return {};
  }
  return path;
}

void serve_response(QTcpServer &server, const QByteArray &status,
                    const QByteArray &body) {
  QObject::connect(
      &server, &QTcpServer::newConnection, &server, [&server, status, body] {
        QTcpSocket *const socket = server.nextPendingConnection();
        ASSERT_NE(socket, nullptr);
        QObject::connect(socket, &QTcpSocket::readyRead, socket,
                         [socket, status, body] {
                           (void)socket->readAll();
                           const QByteArray response =
                               "HTTP/1.1 " + status +
                               "\r\nContent-Type: text/plain; "
                               "charset=utf-8\r\nContent-Length: " +
                               QByteArray::number(body.size()) +
                               "\r\nConnection: close\r\n\r\n" + body;
                           socket->write(response);
                           socket->disconnectFromHost();
                         });
      });
}

TEST(BrowserWindow, StartsOnHomePage) {
  sampan::app::BrowserWindow window;

  EXPECT_TRUE(window.is_home());
  EXPECT_EQ(window.current_location(), QStringLiteral("sampan:home"));
  EXPECT_TRUE(window.error_message().isEmpty());
  EXPECT_NE(window.document_view(), nullptr);
}

TEST(BrowserWindow, ExplicitInitialLocationStartsTheHistory) {
  QTemporaryDir first_directory;
  QTemporaryDir second_directory;
  ASSERT_TRUE(first_directory.isValid());
  ASSERT_TRUE(second_directory.isValid());
  const QString first_path = write_document(first_directory, "page {}");
  const QString second_path = write_document(second_directory, "page {}");
  ASSERT_FALSE(first_path.isEmpty());
  ASSERT_FALSE(second_path.isEmpty());

  sampan::app::BrowserWindow window{std::optional<QString>{first_path}};
  auto *const back =
      window.findChild<QPushButton *>(QStringLiteral("backButton"));
  ASSERT_NE(back, nullptr);

  EXPECT_FALSE(window.is_home());
  EXPECT_EQ(window.current_location(),
            QUrl::fromLocalFile(first_path).toString(QUrl::FullyEncoded));
  EXPECT_FALSE(back->isEnabled());

  window.navigate(second_path);
  ASSERT_TRUE(back->isEnabled());
  back->click();

  EXPECT_EQ(window.current_location(),
            QUrl::fromLocalFile(first_path).toString(QUrl::FullyEncoded));
  EXPECT_FALSE(back->isEnabled());
}

TEST(BrowserWindow, FailedInitialLocationLeavesABlankDocument) {
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());

  sampan::app::BrowserWindow window{
      std::optional<QString>{directory.filePath(QStringLiteral("missing.yl"))}};
  auto *const back =
      window.findChild<QPushButton *>(QStringLiteral("backButton"));
  ASSERT_NE(back, nullptr);

  EXPECT_FALSE(window.is_home());
  EXPECT_TRUE(window.current_location().isEmpty());
  EXPECT_FALSE(window.error_message().isEmpty());
  EXPECT_FALSE(back->isEnabled());
}

TEST(BrowserWindow, BackAndForwardButtonsTrackHistoryPosition) {
  QTemporaryDir first_directory;
  QTemporaryDir second_directory;
  ASSERT_TRUE(first_directory.isValid());
  ASSERT_TRUE(second_directory.isValid());
  const QString first_path = write_document(first_directory, "page {}");
  const QString second_path = write_document(second_directory, "page {}");
  ASSERT_FALSE(first_path.isEmpty());
  ASSERT_FALSE(second_path.isEmpty());

  sampan::app::BrowserWindow window;
  sampan::app::DocumentView *const document_view = window.document_view();
  ASSERT_NE(document_view, nullptr);
  auto *const back =
      window.findChild<QPushButton *>(QStringLiteral("backButton"));
  auto *const forward =
      window.findChild<QPushButton *>(QStringLiteral("forwardButton"));
  auto *const reload =
      window.findChild<QPushButton *>(QStringLiteral("reloadButton"));
  ASSERT_NE(back, nullptr);
  ASSERT_NE(forward, nullptr);
  ASSERT_NE(reload, nullptr);
  const auto *const back_opacity =
      qobject_cast<QGraphicsOpacityEffect *>(back->graphicsEffect());
  const auto *const forward_opacity =
      qobject_cast<QGraphicsOpacityEffect *>(forward->graphicsEffect());
  ASSERT_NE(back_opacity, nullptr);
  ASSERT_NE(forward_opacity, nullptr);
  EXPECT_TRUE(back->text().isEmpty());
  EXPECT_TRUE(forward->text().isEmpty());
  EXPECT_FALSE(back->icon().isNull());
  EXPECT_FALSE(forward->icon().isNull());
  EXPECT_FALSE(reload->icon().isNull());
  EXPECT_FALSE(back->isEnabled());
  EXPECT_FALSE(forward->isEnabled());
  EXPECT_LT(back_opacity->opacity(), 0.5);
  EXPECT_LT(forward_opacity->opacity(), 0.5);

  window.navigate(first_path);
  window.navigate(second_path);
  EXPECT_TRUE(back->isEnabled());
  EXPECT_FALSE(forward->isEnabled());
  EXPECT_DOUBLE_EQ(back_opacity->opacity(), 1.0);
  EXPECT_LT(forward_opacity->opacity(), 0.5);

  back->click();
  EXPECT_TRUE(back->isEnabled());
  EXPECT_TRUE(forward->isEnabled());

  back->click();
  EXPECT_TRUE(window.is_home());
  EXPECT_EQ(window.document_view(), document_view);
  EXPECT_FALSE(back->isEnabled());
  EXPECT_TRUE(forward->isEnabled());

  forward->click();
  EXPECT_FALSE(window.is_home());
  EXPECT_EQ(window.document_view(), document_view);
  EXPECT_TRUE(back->isEnabled());
  EXPECT_TRUE(forward->isEnabled());
}

TEST(BrowserWindow, OpensNativePathAndFileUrl) {
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const QString path = write_document(directory, "page { background: #abc }");
  ASSERT_FALSE(path.isEmpty());

  sampan::app::BrowserWindow window;
  sampan::app::DocumentView *const home_view = window.document_view();
  ASSERT_NE(home_view, nullptr);
  window.navigate(path);
  ASSERT_NE(window.document_view(), nullptr);
  sampan::app::DocumentView *const initial_view = window.document_view();
  EXPECT_EQ(initial_view, home_view);
  EXPECT_FALSE(window.is_home());
  EXPECT_TRUE(window.current_location().startsWith(QStringLiteral("file:")));

  window.navigate(QUrl::fromLocalFile(path).toString());
  EXPECT_EQ(window.document_view(), initial_view);
  EXPECT_TRUE(window.error_message().isEmpty());
}

TEST(BrowserWindow, FailedNavigationPreservesCurrentDocument) {
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const QString path = write_document(directory, "page {}");
  ASSERT_FALSE(path.isEmpty());

  sampan::app::BrowserWindow window;
  window.navigate(path);
  sampan::app::DocumentView *const successful_view = window.document_view();
  ASSERT_NE(successful_view, nullptr);

  window.navigate(directory.filePath(QStringLiteral("missing.yl")));
  EXPECT_EQ(window.document_view(), successful_view);
  EXPECT_FALSE(window.error_message().isEmpty());
}

TEST(BrowserWindow, SourceDiagnosticsPreserveCurrentDocument) {
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const QString path = write_document(directory, "page {}");
  ASSERT_FALSE(path.isEmpty());

  sampan::app::BrowserWindow window;
  window.navigate(path);
  sampan::app::DocumentView *const successful_view = window.document_view();
  ASSERT_NE(successful_view, nullptr);

  ASSERT_FALSE(write_document(directory, "page { unknown: true }").isEmpty());
  window.navigate(path);
  EXPECT_EQ(window.document_view(), successful_view);
  EXPECT_TRUE(window.error_message().contains(QStringLiteral(":1:")));
  EXPECT_TRUE(window.error_message().contains(QStringLiteral("error:")));
}

TEST(DocumentLoader, RejectsInvalidUtf8) {
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const QString path = write_document(directory, QByteArray{"\xC3\x28", 2});
  ASSERT_FALSE(path.isEmpty());

  sampan::app::DocumentLoader loader;
  bool completed = false;
  loader.load(path, [&completed](sampan::app::LoadResult result) {
    completed = true;
    ASSERT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().contains(QStringLiteral("UTF-8")));
  });
  EXPECT_TRUE(completed);
}

TEST(BrowserWindow, FetchesCompleteHttpDocument) {
  QTcpServer server;
  ASSERT_TRUE(server.listen(QHostAddress::LocalHost, 0));

  const QByteArray source{"page { background: #def }"};
  serve_response(server, "200 OK", source);

  sampan::app::BrowserWindow window;
  const QUrl url{QStringLiteral("http://127.0.0.1:%1/document.yl")
                     .arg(server.serverPort())};
  window.navigate(url.toString());

  ASSERT_TRUE(wait_until([&window] { return !window.is_loading(); }));
  EXPECT_NE(window.document_view(), nullptr);
  EXPECT_TRUE(window.error_message().isEmpty());
  EXPECT_TRUE(window.current_location().startsWith(QStringLiteral("http:")));
}

TEST(BrowserWindow, BareHostDefaultsToHttp) {
  QTcpServer server;
  ASSERT_TRUE(server.listen(QHostAddress::LocalHost, 0));
  serve_response(server, "200 OK", "page {}");

  sampan::app::BrowserWindow window;
  window.navigate(
      QStringLiteral("127.0.0.1:%1/document.yl").arg(server.serverPort()));

  ASSERT_TRUE(wait_until([&window] { return !window.is_loading(); }));
  EXPECT_NE(window.document_view(), nullptr);
  EXPECT_TRUE(window.error_message().isEmpty());
  EXPECT_TRUE(window.current_location().startsWith(QStringLiteral("http:")));
}

TEST(BrowserWindow, ReportsHttpErrorsWithoutReplacingThePage) {
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const QString path = write_document(directory, "page {}");
  ASSERT_FALSE(path.isEmpty());

  QTcpServer server;
  ASSERT_TRUE(server.listen(QHostAddress::LocalHost, 0));
  serve_response(server, "404 Not Found", "missing");

  sampan::app::BrowserWindow window;
  window.navigate(path);
  sampan::app::DocumentView *const successful_view = window.document_view();
  ASSERT_NE(successful_view, nullptr);

  window.navigate(QStringLiteral("http://127.0.0.1:%1/missing.yl")
                      .arg(server.serverPort()));
  ASSERT_TRUE(wait_until([&window] { return !window.is_loading(); }));
  EXPECT_EQ(window.document_view(), successful_view);
  EXPECT_TRUE(window.error_message().contains(QStringLiteral("404")));
}

TEST(BrowserWindow, NewNavigationCancelsAnOlderRequest) {
  QTcpServer stalled_server;
  ASSERT_TRUE(stalled_server.listen(QHostAddress::LocalHost, 0));
  QObject::connect(&stalled_server, &QTcpServer::newConnection, &stalled_server,
                   [&stalled_server] {
                     QTcpSocket *const socket =
                         stalled_server.nextPendingConnection();
                     ASSERT_NE(socket, nullptr);
                     QObject::connect(socket, &QTcpSocket::readyRead, socket,
                                      [socket] { (void)socket->readAll(); });
                   });

  QTcpServer successful_server;
  ASSERT_TRUE(successful_server.listen(QHostAddress::LocalHost, 0));
  serve_response(successful_server, "200 OK", "page {}");

  sampan::app::BrowserWindow window;
  window.navigate(QStringLiteral("http://127.0.0.1:%1/slow.yl")
                      .arg(stalled_server.serverPort()));
  ASSERT_TRUE(window.is_loading());
  window.navigate(QStringLiteral("http://127.0.0.1:%1/current.yl")
                      .arg(successful_server.serverPort()));

  ASSERT_TRUE(wait_until([&window] { return !window.is_loading(); }));
  EXPECT_NE(window.document_view(), nullptr);
  EXPECT_TRUE(window.current_location().contains(
      QString::number(successful_server.serverPort())));
  EXPECT_TRUE(window.error_message().isEmpty());
}

} // namespace
