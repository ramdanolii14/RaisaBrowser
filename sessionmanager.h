#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QWebEngineProfile>
#include <QString>

class SessionManager {
public:
    static QWebEngineProfile* createProfile();

    // Dipanggil setelah OAuth2 berhasil untuk inject Authorization header
    // ke request accounts.google.com/OAuthLogin
    static void setOAuthToken(const QString &token);
    static void clearOAuthToken();

private:
    static QWebEngineProfile *s_profile;

public:
    static QString            s_pendingToken;
};

#endif // SESSIONMANAGER_H