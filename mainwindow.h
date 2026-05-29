#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QWebEngineProfile>
#include <QWebEnginePage>
#include <QWebEnginePermission>
#include <QWebEngineCookieStore>
#include <QTimer>
#include <QMap>
#include <QUrl>
#include <QLabel>
#include <QTcpServer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEvent>

struct HistoryEntry {
    QString title;
    QString url;
    QString timestamp;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void addHistoryEntry(const QString &title, const QString &url);

    // Dibuat public agar bisa diakses dari lambda di injectWebEngineSession
    QString m_googleDisplayName;

private slots:
    void addNewTab();
    void closeTab(int index);
    void showHistory();
    void toggleTheme();
    void setEngineGoogle();
    void setEngineBing();
    void setEngineDuck();
    void handleTabChange(int index);
    void updateAvatarFromCookies();
    void handlePermissionRequest(QWebEnginePermission permission);

    // OAuth2 Google Sign-In
    void startGoogleOAuth();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QTabWidget        *tabs;
    QWebEngineProfile *myProfile;
    bool               isDark;
    QString            searchUrl;
    QString            currentSearchEngine;

    QList<HistoryEntry> history;

    QLabel *avatarLabel;
    void    setDefaultAvatar();
    void    setAvatarFromUrl(const QString &imageUrl);

    QMap<QWidget*, QTimer*>  loadingTimers;
    QMap<QWidget*, int>      loadingDots;
    QMap<QWidget*, QString>  savedTitles;

    void    startLoadingAnimation(QWidget *tab);
    void    stopLoadingAnimation(QWidget *tab, const QString &finalTitle);
    QString getGreeting();
    QUrl    newTabUrl();

    static QString permissionName(QWebEnginePermission::PermissionType type);
    static QString permissionIcon(QWebEnginePermission::PermissionType type);

    // OAuth2
    QString     m_oauthVerifier;
    QString     m_oauthState;
    QTcpServer *m_oauthServer = nullptr;

    void           handleOAuthCallback(const QString &code);
    void           exchangeCodeForToken(const QString &code);
    void           fetchGoogleUserInfo(const QString &accessToken);
    void           injectWebEngineSession(const QString &accessToken);
    static QString generateRandomString(int len);
    static QByteArray sha256Base64Url(const QString &input);
};

#endif // MAINWINDOW_H