#ifndef OPENSKYAUTHMANAGER_H
#define OPENSKYAUTHMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>

class OpenSkyAuthManager : public QObject
{
    Q_OBJECT

public:
    explicit OpenSkyAuthManager(QObject *parent = nullptr);

    void setCredentials(const QString& clientId, const QString& clientSecret);
    void authenticate();
    
    bool isAuthenticated() const { return !m_accessToken.isEmpty(); }
    QString accessToken() const { return m_accessToken; }

signals:
    void authenticationSuccess();
    void authenticationFailed(const QString& error);

private slots:
    void onAuthenticationReply();

private:
    void requestAccessToken();

    QNetworkAccessManager* m_networkManager;
    QString m_clientId;
    QString m_clientSecret;
    QString m_accessToken;
};

#endif // OPENSKYAUTHMANAGER_H