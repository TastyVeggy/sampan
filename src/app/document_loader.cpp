#include "sampan/app/document_loader.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringDecoder>
#include <QVariant>

#include <cstddef>
#include <expected>
#include <utility>

namespace sampan::app {
namespace {

constexpr qint64 kMaximumSourceBytes = 4 * 1024 * 1024;
constexpr int kTransferTimeoutMilliseconds = 15'000;
constexpr int kMaximumRedirects = 10;

[[nodiscard]] bool defaults_to_http(const QString &host) {
  QHostAddress address;
  return host.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0 ||
         !host.contains(QLatin1Char('.')) ||
         host.endsWith(QStringLiteral(".local"), Qt::CaseInsensitive) ||
         address.setAddress(host);
}

[[nodiscard]] std::expected<QUrl, QString>
resolve_location(const QString &input) {
  const QString location = input.trimmed();
  if (location.isEmpty()) {
    return std::unexpected{QStringLiteral("Enter a file path or URL")};
  }

  const bool has_explicit_scheme =
      location.contains(QStringLiteral("://")) ||
      location.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive);
  if (has_explicit_scheme) {
    const QUrl explicit_url{location, QUrl::StrictMode};
    const QString scheme = explicit_url.scheme().toLower();
    if (scheme != QStringLiteral("file") && scheme != QStringLiteral("http") &&
        scheme != QStringLiteral("https")) {
      return std::unexpected{
          QStringLiteral("Unsupported URL scheme: %1").arg(scheme)};
    }
    if (!explicit_url.isValid()) {
      return std::unexpected{QStringLiteral("Invalid URL")};
    }
    return explicit_url;
  }

  const QFileInfo file_info{location};
  const qsizetype first_separator = location.indexOf(QLatin1Char('/'));
  const QString first_segment =
      location.first(first_separator < 0 ? location.size() : first_separator);
  const bool looks_like_local_path =
      file_info.exists() || QDir::isAbsolutePath(location) ||
      location.startsWith(QStringLiteral("./")) ||
      location.startsWith(QStringLiteral("../")) ||
      location.contains(QLatin1Char('\\')) ||
      (!location.contains(QLatin1Char('/')) &&
       location.endsWith(QStringLiteral(".yl"), Qt::CaseInsensitive)) ||
      (!first_segment.contains(QLatin1Char('.')) &&
       !first_segment.contains(QLatin1Char(':')) &&
       first_segment != QStringLiteral("localhost"));
  if (looks_like_local_path) {
    return QUrl::fromLocalFile(QDir::cleanPath(file_info.absoluteFilePath()));
  }

  QUrl web_url{QStringLiteral("http://") + location, QUrl::StrictMode};
  if (!web_url.isValid() || web_url.host().isEmpty()) {
    return std::unexpected{QStringLiteral("Invalid URL")};
  }
  web_url.setScheme(defaults_to_http(web_url.host()) ? QStringLiteral("http")
                                                     : QStringLiteral("https"));
  return web_url;
}

[[nodiscard]] std::expected<std::string, QString>
validated_source(const QByteArray &bytes) {
  QStringDecoder decoder{QStringDecoder::Utf8};
  const QString decoded = decoder.decode(bytes);
  (void)decoded;
  if (decoder.hasError()) {
    return std::unexpected{QStringLiteral("Document is not valid UTF-8")};
  }
  return std::string{bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

[[nodiscard]] QString network_error(const QNetworkReply &reply) {
  const QVariant status_value =
      reply.attribute(QNetworkRequest::HttpStatusCodeAttribute);
  if (status_value.isValid()) {
    const int status = status_value.toInt();
    if (status < 200 || status >= 300) {
      const QString reason =
          reply.attribute(QNetworkRequest::HttpReasonPhraseAttribute)
              .toString();
      return reason.isEmpty()
                 ? QStringLiteral("HTTP request failed with status %1")
                       .arg(status)
                 : QStringLiteral("HTTP request failed with status %1: %2")
                       .arg(status)
                       .arg(reason);
    }
  }
  return reply.errorString();
}

} // namespace

DocumentLoader::DocumentLoader(QObject *parent)
    : QObject{parent}, network_manager_{this} {
}

DocumentLoader::~DocumentLoader() {
  cancel();
}

void DocumentLoader::load(const QString &location, Completion completion) {
  cancel();

  const std::expected<QUrl, QString> resolved = resolve_location(location);
  if (!resolved.has_value()) {
    completion(std::unexpected{resolved.error()});
    return;
  }

  if (resolved->isLocalFile()) {
    load_file(*resolved, std::move(completion));
    return;
  }
  load_network(*resolved, std::move(completion));
}

void DocumentLoader::cancel() {
  completion_ = {};
  buffer_.clear();
  if (reply_ == nullptr) {
    return;
  }

  QNetworkReply *const reply = std::exchange(reply_, nullptr);
  reply->disconnect(this);
  reply->abort();
  reply->deleteLater();
}

bool DocumentLoader::is_loading() const noexcept {
  return reply_ != nullptr;
}

void DocumentLoader::load_file(const QUrl &url, Completion completion) {
  const QFileInfo info{url.toLocalFile()};
  if (!info.exists()) {
    completion(std::unexpected{QStringLiteral("File does not exist: %1")
                                   .arg(info.absoluteFilePath())});
    return;
  }
  if (!info.isFile()) {
    completion(std::unexpected{QStringLiteral("Location is not a file: %1")
                                   .arg(info.absoluteFilePath())});
    return;
  }
  if (info.size() > kMaximumSourceBytes) {
    completion(
        std::unexpected{QStringLiteral("Document exceeds the %1 MiB limit")
                            .arg(kMaximumSourceBytes / (1024 * 1024))});
    return;
  }

  QFile file{info.absoluteFilePath()};
  if (!file.open(QIODevice::ReadOnly)) {
    completion(
        std::unexpected{QStringLiteral("Cannot open %1: %2")
                            .arg(info.absoluteFilePath(), file.errorString())});
    return;
  }

  const QByteArray bytes = file.read(kMaximumSourceBytes + 1);
  if (bytes.size() > kMaximumSourceBytes) {
    completion(
        std::unexpected{QStringLiteral("Document exceeds the %1 MiB limit")
                            .arg(kMaximumSourceBytes / (1024 * 1024))});
    return;
  }
  if (file.error() != QFileDevice::NoError) {
    completion(
        std::unexpected{QStringLiteral("Cannot read %1: %2")
                            .arg(info.absoluteFilePath(), file.errorString())});
    return;
  }

  std::expected<std::string, QString> source = validated_source(bytes);
  if (!source.has_value()) {
    completion(std::unexpected{source.error()});
    return;
  }

  const QString canonical_path = info.canonicalFilePath();
  completion(LoadedDocument{
      .url = QUrl::fromLocalFile(
          canonical_path.isEmpty() ? info.absoluteFilePath() : canonical_path),
      .source = std::move(*source)});
}

void DocumentLoader::load_network(const QUrl &url, Completion completion) {
  QNetworkRequest request{url};
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                       QNetworkRequest::NoLessSafeRedirectPolicy);
  request.setMaximumRedirectsAllowed(kMaximumRedirects);
  request.setTransferTimeout(kTransferTimeoutMilliseconds);
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    QStringLiteral("Sampan/0.1"));

  completion_ = std::move(completion);
  buffer_.clear();
  reply_ = network_manager_.get(request);
  QNetworkReply *const request_reply = reply_;

  QObject::connect(request_reply, &QNetworkReply::metaDataChanged, this,
                   [this, request_reply] {
                     if (request_reply != reply_) {
                       return;
                     }
                     bool valid = false;
                     const qint64 content_length =
                         request_reply
                             ->header(QNetworkRequest::ContentLengthHeader)
                             .toLongLong(&valid);
                     if (valid && content_length > kMaximumSourceBytes) {
                       fail(QStringLiteral("Document exceeds the %1 MiB limit")
                                .arg(kMaximumSourceBytes / (1024 * 1024)));
                     }
                   });
  QObject::connect(request_reply, &QIODevice::readyRead, this,
                   [this, request_reply] {
                     if (request_reply != reply_) {
                       return;
                     }
                     buffer_.append(request_reply->readAll());
                     if (buffer_.size() > kMaximumSourceBytes) {
                       fail(QStringLiteral("Document exceeds the %1 MiB limit")
                                .arg(kMaximumSourceBytes / (1024 * 1024)));
                     }
                   });
  QObject::connect(
      request_reply, &QNetworkReply::finished, this, [this, request_reply] {
        if (request_reply != reply_) {
          return;
        }
        buffer_.append(request_reply->readAll());
        if (buffer_.size() > kMaximumSourceBytes) {
          fail(QStringLiteral("Document exceeds the size limit"));
          return;
        }
        if (request_reply->error() != QNetworkReply::NoError) {
          fail(network_error(*request_reply));
          return;
        }

        std::expected<std::string, QString> source = validated_source(buffer_);
        if (!source.has_value()) {
          fail(source.error());
          return;
        }
        succeed(LoadedDocument{.url = request_reply->url(),
                               .source = std::move(*source)});
      });
}

void DocumentLoader::fail(QString message) {
  finish(std::unexpected{std::move(message)});
}

void DocumentLoader::succeed(LoadedDocument document) {
  finish(LoadResult{std::move(document)});
}

void DocumentLoader::finish(LoadResult result) {
  QNetworkReply *const reply = std::exchange(reply_, nullptr);
  Completion completion = std::move(completion_);
  buffer_.clear();
  if (reply != nullptr) {
    reply->disconnect(this);
    reply->abort();
    reply->deleteLater();
  }
  if (completion) {
    completion(std::move(result));
  }
}

} // namespace sampan::app
