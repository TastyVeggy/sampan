#pragma once

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QUrl>

#include <expected>
#include <functional>
#include <string>

class QNetworkReply;

namespace sampan::app {

struct LoadedDocument {
  QUrl url;
  std::string source;
};

using LoadResult = std::expected<LoadedDocument, QString>;

class DocumentLoader final : public QObject {
public:
  using Completion = std::function<void(LoadResult)>;

  explicit DocumentLoader(QObject *parent = nullptr);
  ~DocumentLoader() override;

  DocumentLoader(const DocumentLoader &) = delete;
  DocumentLoader &operator=(const DocumentLoader &) = delete;
  DocumentLoader(DocumentLoader &&) = delete;
  DocumentLoader &operator=(DocumentLoader &&) = delete;

  void load(const QString &location, Completion completion);
  void cancel();

  [[nodiscard]] bool is_loading() const noexcept;

private:
  void load_file(const QUrl &url, Completion completion);
  void load_network(const QUrl &url, Completion completion);
  void fail(QString message);
  void succeed(LoadedDocument document);
  void finish(LoadResult result);

  QNetworkAccessManager network_manager_;
  QNetworkReply *reply_ = nullptr;
  QByteArray buffer_;
  Completion completion_;
};

} // namespace sampan::app
