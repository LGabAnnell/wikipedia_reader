#ifndef FAKE_NETWORK_REPLY_H
#define FAKE_NETWORK_REPLY_H

#include <QNetworkReply>
#include <QNetworkAccessManager>

struct FakeNetworkResponse {
    QByteArray body;
    QByteArray contentType = "application/json";
    int statusCode = 200;
    QNetworkReply::NetworkError networkError = QNetworkReply::NoError;
    QString errorString;
};

class FakeNetworkReply final : public QNetworkReply {
    Q_OBJECT

  public:
    FakeNetworkReply(const QNetworkRequest &request, QNetworkAccessManager::Operation operation,
                     const FakeNetworkResponse &response, bool deferred = false, QObject *parent = nullptr);

    qint64 bytesAvailable() const override;
    void abort() override;
    void complete();

  protected:
    qint64 readData(char *data, qint64 maxSize) override;

  private:
    void finishReply();

    QByteArray m_body;
    qint64 m_position = 0;
    QNetworkReply::NetworkError m_networkError;
    QString m_errorString;
    bool m_completed = false;
};

#endif // FAKE_NETWORK_REPLY_H
