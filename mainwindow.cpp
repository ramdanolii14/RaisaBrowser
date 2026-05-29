// mainwindow.cpp
#include "mainwindow.h"
#include "sessionmanager.h"
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QWebEngineSettings>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEnginePermission>
#include <QWebEngineFullScreenRequest>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QComboBox>
#include <QLabel>
#include <QApplication>
#include <QTabBar>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QDateTime>
#include <QFile>
#include <QStandardPaths>
#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QWebEngineHistory>
#include <QScrollArea>
#include <QSplitter>
#include <QStatusBar>
#include <QPushButton>
#include <QFrame>
#include <QRegularExpression>
#include <QMessageBox>
#include <QPointer>
#include <QDesktopServices>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QWebEngineCookieStore>
#include <functional>
#include <memory>

// ─────────────────────────────────────────────────────────────────────────────
// OAuth2 credentials (dari Google Cloud Console)
// PENTING: Pastikan di Google Cloud Console, tipe aplikasinya adalah
// "Desktop app" (bukan Web application), agar redirect ke localhost diizinkan.
// ─────────────────────────────────────────────────────────────────────────────
static const QString kClientId     = "571310076781-58h7kdbo34rfv0b2cfke8ai28j8410ca.apps.googleusercontent.com";
static const QString kClientSecret = "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";
static const QString kRedirectUri  = "http://127.0.0.1:9876/oauth2callback";
static const int     kOAuthPort    = 9876;

// ─────────────────────────────────────────────────────────────────────────────
// Anti-detection script
//
// PERBAIKAN: chrome.runtime.id harus string (bukan undefined/null).
// Jika undefined, Google script akan crash saat akses .sendMessage.
// ─────────────────────────────────────────────────────────────────────────────
static const QString kAntiDetectionScript = R"JS(
(function() {
    'use strict';

    // Sembunyikan webdriver flag
    Object.defineProperty(navigator, 'webdriver', {
        get: () => undefined,
        configurable: true
    });

    // Inject window.chrome yang kompatibel
    // PENTING: runtime.id harus string kosong, BUKAN undefined/null.
    // Jika null/undefined, pemanggil yang langsung akses .sendMessage akan crash.
    if (!window.chrome) {
        Object.defineProperty(window, 'chrome', {
            value: {
                app: {
                    isInstalled: false,
                    InstallState: { DISABLED: 'disabled', INSTALLED: 'installed', NOT_INSTALLED: 'not_installed' },
                    RunningState: { CANNOT_RUN: 'cannot_run', READY_TO_RUN: 'ready_to_run', RUNNING: 'running' },
                    getDetails: function() { return null; },
                    getIsInstalled: function() { return false; },
                    installState: function(cb) { cb('not_installed'); }
                },
                csi: function() { return {}; },
                loadTimes: function() {
                    return {
                        commitLoadTime: Date.now() / 1000 - 0.5,
                        connectionInfo: 'h2',
                        finishDocumentLoadTime: Date.now() / 1000 - 0.1,
                        finishLoadTime: Date.now() / 1000,
                        firstPaintAfterLoadTime: 0,
                        firstPaintTime: Date.now() / 1000 - 0.2,
                        navigationType: 'Other',
                        npnNegotiatedProtocol: 'h2',
                        requestTime: Date.now() / 1000 - 0.8,
                        startLoadTime: Date.now() / 1000 - 0.7,
                        wasAlternateProtocolAvailable: false,
                        wasFetchedViaSpdy: true,
                        wasNpnNegotiated: true
                    };
                },
                // runtime.id = string kosong mencegah crash "Cannot read properties of null"
                runtime: {
                    id: '',
                    connect: function() {
                        return {
                            onMessage: { addListener: function() {}, removeListener: function() {} },
                            onDisconnect: { addListener: function() {}, removeListener: function() {} },
                            postMessage: function() {},
                            disconnect: function() {}
                        };
                    },
                    sendMessage: function() {},
                    onMessage: {
                        addListener: function() {},
                        removeListener: function() {},
                        hasListener: function() { return false; }
                    },
                    onConnect: {
                        addListener: function() {},
                        removeListener: function() {}
                    },
                    lastError: null,
                    getManifest: function() { return {}; },
                    getURL: function(path) { return path; }
                }
            },
            writable: false,
            configurable: false
        });
    }

    // Inject plugin list agar tidak terlihat seperti headless
    if (navigator.plugins.length === 0) {
        const makePlugin = (name, filename, desc, mimeTypes) => {
            const plugin = { name, filename, description: desc, length: mimeTypes.length };
            mimeTypes.forEach((mt, i) => {
                plugin[i] = { type: mt.type, suffixes: mt.suffixes, description: mt.desc, enabledPlugin: plugin };
            });
            plugin.item = i => plugin[i];
            plugin.namedItem = n => null;
            return plugin;
        };
        const plugins = [
            makePlugin('PDF Viewer', 'internal-pdf-viewer', 'Portable Document Format',
                [{ type: 'application/pdf', suffixes: 'pdf', desc: 'Portable Document Format' },
                 { type: 'text/pdf', suffixes: 'pdf', desc: 'Portable Document Format' }]),
            makePlugin('Chrome PDF Viewer', 'internal-pdf-viewer', 'Portable Document Format',
                [{ type: 'application/pdf', suffixes: 'pdf', desc: 'Portable Document Format' }]),
        ];
        Object.defineProperty(navigator, 'plugins', {
            get: () => {
                const arr = [...plugins];
                arr.item = i => arr[i];
                arr.namedItem = n => arr.find(p => p.name === n) || null;
                arr.refresh = () => {};
                Object.setPrototypeOf(arr, PluginArray.prototype);
                return arr;
            },
            configurable: true
        });
    }

    if (!navigator.languages || navigator.languages.length === 0) {
        Object.defineProperty(navigator, 'languages', {
            get: () => ['id-ID', 'id', 'en-US', 'en'],
            configurable: true
        });
    }

    Object.defineProperty(navigator, 'maxTouchPoints', {
        get: () => 0,
        configurable: true
    });
})();
)JS";

// ─────────────────────────────────────────────────────────────────────────────
// Nama & ikon tiap jenis izin
// ─────────────────────────────────────────────────────────────────────────────
QString MainWindow::permissionName(QWebEnginePermission::PermissionType type) {
    switch (type) {
        case QWebEnginePermission::PermissionType::Geolocation:              return "Lokasi";
        case QWebEnginePermission::PermissionType::MediaAudioCapture:        return "Mikrofon";
        case QWebEnginePermission::PermissionType::MediaVideoCapture:        return "Kamera";
        case QWebEnginePermission::PermissionType::MediaAudioVideoCapture:   return "Kamera & Mikrofon";
        case QWebEnginePermission::PermissionType::DesktopAudioVideoCapture: return "Rekam Layar & Audio";
        case QWebEnginePermission::PermissionType::DesktopVideoCapture:      return "Rekam Layar";
        case QWebEnginePermission::PermissionType::Notifications:            return "Notifikasi";
        case QWebEnginePermission::PermissionType::ClipboardReadWrite:       return "Baca/Tulis Clipboard";
        case QWebEnginePermission::PermissionType::LocalFontsAccess:         return "Akses Font Lokal";
        default:                                                              return "Izin Tidak Dikenal";
    }
}

QString MainWindow::permissionIcon(QWebEnginePermission::PermissionType type) {
    switch (type) {
        case QWebEnginePermission::PermissionType::Geolocation:              return "📍";
        case QWebEnginePermission::PermissionType::MediaAudioCapture:        return "🎙️";
        case QWebEnginePermission::PermissionType::MediaVideoCapture:        return "📷";
        case QWebEnginePermission::PermissionType::MediaAudioVideoCapture:   return "📹";
        case QWebEnginePermission::PermissionType::DesktopAudioVideoCapture: return "🖥️";
        case QWebEnginePermission::PermissionType::DesktopVideoCapture:      return "🖥️";
        case QWebEnginePermission::PermissionType::Notifications:            return "🔔";
        case QWebEnginePermission::PermissionType::ClipboardReadWrite:       return "📋";
        case QWebEnginePermission::PermissionType::LocalFontsAccess:         return "🔤";
        default:                                                              return "❓";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Handler izin
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::handlePermissionRequest(QWebEnginePermission permission) {
    QString icon   = permissionIcon(permission.permissionType());
    QString name   = permissionName(permission.permissionType());
    QString origin = permission.origin().host();
    if (origin.isEmpty()) origin = permission.origin().toString();

    QDialog *dlg = new QDialog(this, Qt::Tool | Qt::FramelessWindowHint);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowModality(Qt::NonModal);
    dlg->setStyleSheet(R"(
        QDialog {
            background: #ffffff;
            border: 1px solid #dadce0;
            border-radius: 8px;
        }
        QLabel#title { font-size:14px; font-weight:bold; color:#202124; }
        QLabel#sub   { font-size:12px; color:#5f6368; }
        QPushButton#allow {
            background:#1a73e8; color:white; border:none;
            border-radius:4px; padding:7px 18px;
            font-size:13px; font-weight:bold;
        }
        QPushButton#allow:hover { background:#1557b0; }
        QPushButton#deny {
            background:white; color:#444;
            border:1px solid #dadce0; border-radius:4px;
            padding:7px 18px; font-size:13px;
        }
        QPushButton#deny:hover { background:#f1f3f4; }
    )");

    QVBoxLayout *vl = new QVBoxLayout(dlg);
    vl->setContentsMargins(16, 14, 16, 14);
    vl->setSpacing(8);

    QLabel *lbTitle = new QLabel(
        QString("%1 %2 ingin mengakses %3").arg(icon).arg(origin).arg(name), dlg);
    lbTitle->setObjectName("title");
    lbTitle->setWordWrap(true);

    QLabel *lbSub = new QLabel("Izinkan situs ini mengakses perangkat Anda?", dlg);
    lbSub->setObjectName("sub");

    QHBoxLayout *hl  = new QHBoxLayout();
    QPushButton *allowBtn = new QPushButton("Izinkan", dlg);
    allowBtn->setObjectName("allow");
    QPushButton *denyBtn  = new QPushButton("Tolak", dlg);
    denyBtn->setObjectName("deny");
    hl->addStretch();
    hl->addWidget(denyBtn);
    hl->addWidget(allowBtn);

    vl->addWidget(lbTitle);
    vl->addWidget(lbSub);
    vl->addLayout(hl);

    dlg->adjustSize();
    QPoint pos = this->mapToGlobal(QPoint(10, this->height() - dlg->height() - 10));
    dlg->move(pos);

    connect(allowBtn, &QPushButton::clicked, [=]() mutable { permission.grant(); dlg->close(); });
    connect(denyBtn,  &QPushButton::clicked, [=]() mutable { permission.deny();  dlg->close(); });

    dlg->show();
}

// ─────────────────────────────────────────────────────────────────────────────
// Inisialisasi anti-detection script ke profil
// ─────────────────────────────────────────────────────────────────────────────
static void installAntiDetectionScript(QWebEngineProfile *profile) {
    const QList<QWebEngineScript> existing = profile->scripts()->find("AntiDetection");
    for (const QWebEngineScript &s : existing)
        profile->scripts()->remove(s);

    QWebEngineScript script;
    script.setName("AntiDetection");
    script.setSourceCode(kAntiDetectionScript);
    script.setInjectionPoint(QWebEngineScript::DocumentCreation);
    script.setWorldId(QWebEngineScript::MainWorld);
    script.setRunsOnSubFrames(true);
    profile->scripts()->insert(script);
}

// ─────────────────────────────────────────────────────────────────────────────
// OAuth2 helper: random string & PKCE SHA-256
// ─────────────────────────────────────────────────────────────────────────────
QString MainWindow::generateRandomString(int len) {
    const QString chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
    QString result;
    result.reserve(len);
    for (int i = 0; i < len; ++i)
        result += chars[QRandomGenerator::global()->bounded(chars.size())];
    return result;
}

QByteArray MainWindow::sha256Base64Url(const QString &input) {
    QByteArray hash = QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Sha256);
    return hash.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

// ─────────────────────────────────────────────────────────────────────────────
// OAuth2 — Langkah 1: buka browser sistem untuk login Google
//
// Alur:
//   1. startGoogleOAuth() → buka browser sistem ke accounts.google.com/o/oauth2/auth
//   2. User login di browser asli (Chrome/Firefox) → Google redirect ke localhost:9876
//   3. QTcpServer menangkap callback → handleOAuthCallback(code)
//   4. exchangeCodeForToken(code) → POST ke oauth2.googleapis.com/token
//   5. fetchGoogleUserInfo(access_token) → GET userinfo → update avatar + nama
//   6. injectWebEngineSession() → buka tab login Google di WebEngine
//
// CATATAN: OAuthLogin/MergeSession (cara lama) sudah diblokir Google sejak 2021.
// Solusi yang benar: setelah dapat token untuk avatar/nama, buka halaman Sign-In
// Google langsung di WebEngine — cookie persistent akan menyimpan sesi.
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::startGoogleOAuth() {
    // Bersihkan server lama jika ada
    if (m_oauthServer) {
        m_oauthServer->close();
        m_oauthServer->deleteLater();
        m_oauthServer = nullptr;
    }

    // Generate PKCE code_verifier dan code_challenge
    m_oauthVerifier = generateRandomString(64);
    QByteArray challenge = sha256Base64Url(m_oauthVerifier);
    m_oauthState = generateRandomString(16);

    // Mulai listener TCP di localhost:9876
    m_oauthServer = new QTcpServer(this);
    if (!m_oauthServer->listen(QHostAddress("127.0.0.1"), kOAuthPort)) {
        QMessageBox::warning(this, "OAuth Error",
            QString("Tidak bisa listen di port %1.\nCoba jalankan ulang browser.").arg(kOAuthPort));
        return;
    }

    connect(m_oauthServer, &QTcpServer::newConnection, this, [this]() {
        QTcpSocket *socket = m_oauthServer->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            QString request = QString::fromUtf8(socket->readAll());

            // Kirim respon HTML sukses ke browser sistem
            QString html = R"(<!DOCTYPE html><html><head><meta charset="UTF-8">
<style>body{font-family:sans-serif;display:flex;align-items:center;justify-content:center;height:100vh;margin:0;background:#f5f5f5;}
.box{text-align:center;padding:40px;background:white;border-radius:12px;box-shadow:0 2px 12px rgba(0,0,0,0.1);}
h2{color:#1a73e8;}p{color:#555;}</style></head>
<body><div class="box"><h2>&#9989; Login Berhasil!</h2><p>Anda telah masuk ke akun Google.<br>Silakan tutup tab ini dan kembali ke RaisaBrowser.</p></div></body></html>)";
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\n\r\n");
            socket->write(html.toUtf8());
            socket->flush();
            socket->disconnectFromHost();

            // Parse 'code' dari request line: GET /oauth2callback?code=...&state=...
            QRegularExpression re(R"(GET /oauth2callback\?([^ ]+) HTTP)");
            QRegularExpressionMatch m = re.match(request);
            if (m.hasMatch()) {
                QUrlQuery query(m.captured(1));
                QString code  = query.queryItemValue("code");
                QString state = query.queryItemValue("state");
                if (!code.isEmpty() && state == m_oauthState) {
                    QTimer::singleShot(200, this, [this, code]() {
                        handleOAuthCallback(code);
                    });
                }
            }

            m_oauthServer->close();
        });
    });

    // Buka URL OAuth di browser sistem asli
    QUrl authUrl("https://accounts.google.com/o/oauth2/v2/auth");
    QUrlQuery params;
    params.addQueryItem("client_id",             kClientId);
    params.addQueryItem("redirect_uri",          kRedirectUri);
    params.addQueryItem("response_type",         "code");
    params.addQueryItem("scope",                 "openid email profile");
    params.addQueryItem("state",                 m_oauthState);
    params.addQueryItem("code_challenge",        QString::fromUtf8(challenge));
    params.addQueryItem("code_challenge_method", "S256");
    params.addQueryItem("access_type",           "offline");
    params.addQueryItem("prompt",                "select_account");
    authUrl.setQuery(params);

    QDesktopServices::openUrl(authUrl);

    // Tampilkan info ke user
    QMessageBox *info = new QMessageBox(this);
    info->setWindowTitle("Login Google");
    info->setText("Browser sistem Anda telah dibuka untuk login Google.\n\n"
                  "Setelah login, avatar Anda akan diperbarui\n"
                  "dan tab baru akan dibuka untuk masuk ke akun Google.");
    info->setIcon(QMessageBox::Information);
    info->setStandardButtons(QMessageBox::Ok);
    info->exec();
}

// ─────────────────────────────────────────────────────────────────────────────
// OAuth2 — Langkah 2: tukar authorization code → access token
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::handleOAuthCallback(const QString &code) {
    exchangeCodeForToken(code);
}

void MainWindow::exchangeCodeForToken(const QString &code) {
    QNetworkAccessManager *nam = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("https://oauth2.googleapis.com/token"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery body;
    body.addQueryItem("code",          code);
    body.addQueryItem("client_id",     kClientId);
    body.addQueryItem("client_secret", kClientSecret);
    body.addQueryItem("redirect_uri",  kRedirectUri);
    body.addQueryItem("grant_type",    "authorization_code");
    body.addQueryItem("code_verifier", m_oauthVerifier);

    QNetworkReply *reply = nam->post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [=]() {
        QByteArray data = reply->readAll();
        reply->deleteLater();
        nam->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull()) {
            qWarning() << "Token exchange: response bukan JSON:" << data.left(200);
            return;
        }
        QJsonObject obj = doc.object();

        QString accessToken = obj.value("access_token").toString();
        if (!accessToken.isEmpty()) {
            fetchGoogleUserInfo(accessToken);
        } else {
            qWarning() << "Token exchange gagal:" << data;
            QMessageBox::warning(this, "Login Gagal",
                "Gagal mendapatkan access token dari Google.\n"
                "Pastikan tipe OAuth client di Google Cloud Console\n"
                "adalah 'Desktop app', bukan 'Web application'.");
        }
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// OAuth2 — Langkah 3: ambil info user & tampilkan avatar
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::fetchGoogleUserInfo(const QString &accessToken) {
    // Ambil foto profil & nama dari Google API
    QNetworkAccessManager *nam = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("https://www.googleapis.com/oauth2/v2/userinfo"));
    req.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());

    QNetworkReply *reply = nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        QByteArray data = reply->readAll();
        reply->deleteLater();
        nam->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            QJsonObject obj = doc.object();
            QString name    = obj.value("name").toString();
            QString picture = obj.value("picture").toString();
            QString email   = obj.value("email").toString();

            if (!picture.isEmpty()) {
                picture.replace(QRegularExpression("=s\\d+-c"), "=s96");
                picture.replace(QRegularExpression("=s\\d+"),   "=s96");
                setAvatarFromUrl(picture);
            }
            if (!name.isEmpty()) {
                avatarLabel->setToolTip("Masuk sebagai: " + name + "\n" + email);
                m_googleDisplayName = name;
            }

            // Setelah dapat info user, buka sesi Google di WebEngine
            injectWebEngineSession(accessToken);
        } else {
            qWarning() << "fetchGoogleUserInfo: response tidak valid:" << data.left(200);
        }
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Inject session Google ke WebEngine.
//
// PERUBAHAN DARI VERSI SEBELUMNYA:
// OAuthLogin + MergeSession sudah diblokir Google untuk semua third-party app
// (menghasilkan "Error=badauth 403"). Cara yang didukung Google untuk login
// di embedded browser adalah membuka halaman Sign-In secara langsung di
// WebEngineView. Cookie store sudah persistent (ForcePersistentCookies),
// sehingga sesi tetap tersimpan untuk sesi berikutnya.
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::injectWebEngineSession(const QString &accessToken) {
    Q_UNUSED(accessToken)  // Tidak digunakan — OAuthLogin/MergeSession deprecated

    QPointer<MainWindow> selfPtr = this;

    QTimer::singleShot(300, this, [selfPtr]() {
        if (!selfPtr) return;

        // Tambah tab baru untuk login Google
        selfPtr->addNewTab();

        QWidget *curTab = selfPtr->tabs->currentWidget();
        if (!curTab) return;

        QWebEngineView *v = curTab->findChild<QWebEngineView*>();
        if (!v) return;

        // Buka halaman Sign-In Google di WebEngine.
        // Google akan memproses login secara normal dan menyimpan cookie sesi.
        // Cookie persistent akan membuat user tetap login di sesi berikutnya.
        v->setUrl(QUrl("https://accounts.google.com/ServiceLogin"
                       "?hl=id"
                       "&continue=https://www.google.com/"
                       "&flowName=GlifWebSignIn"
                       "&flowEntry=ServiceLogin"));

        if (selfPtr->statusBar())
            selfPtr->statusBar()->showMessage(
                "Silakan selesaikan login Google di tab yang baru dibuka.", 6000);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      isDark(false),
      searchUrl("https://www.google.com/search?q="),
      currentSearchEngine("Google")
{
    resize(1280, 800);
    setWindowTitle("RaisaBrowser");

    myProfile = SessionManager::createProfile();
    installAntiDetectionScript(myProfile);

    tabs = new QTabWidget(this);
    tabs->setTabsClosable(true);
    tabs->setDocumentMode(true);
    tabs->setMovable(true);
    setCentralWidget(tabs);

    // ── Tombol + Tab Baru (di kiri) ──────────────────────────────────────────
    QToolButton *addTabBtn = new QToolButton(this);
    addTabBtn->setText("+");
    addTabBtn->setFixedSize(28, 28);
    addTabBtn->setStyleSheet(
        "QToolButton { font-size:16px; border:none; padding:0; margin:2px; }"
        "QToolButton:hover { background:#e8eaed; border-radius:4px; }");
    connect(addTabBtn, &QToolButton::clicked, this, &MainWindow::addNewTab);
    tabs->setCornerWidget(addTabBtn, Qt::TopLeftCorner);

    // ── Avatar (di kanan) ─────────────────────────────────────────────────────
    QWidget     *avatarWrapper = new QWidget(this);
    QHBoxLayout *avatarLayout  = new QHBoxLayout(avatarWrapper);
    avatarLayout->setContentsMargins(4, 2, 6, 2);
    avatarLayout->setSpacing(0);

    avatarLabel = new QLabel(avatarWrapper);
    avatarLabel->setFixedSize(28, 28);
    avatarLabel->setCursor(Qt::PointingHandCursor);
    avatarLabel->setToolTip("Klik untuk login dengan Google");
    setDefaultAvatar();

    avatarLabel->installEventFilter(this);

    avatarLayout->addWidget(avatarLabel);
    avatarWrapper->setLayout(avatarLayout);

    tabs->setCornerWidget(avatarWrapper, Qt::TopRightCorner);

    connect(tabs, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    connect(tabs, &QTabWidget::currentChanged,    this, &MainWindow::handleTabChange);

    addNewTab();
}

// Event filter untuk klik avatar
bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == avatarLabel && event->type() == QEvent::MouseButtonPress) {
        startGoogleOAuth();
        return true;
    }
    return QMainWindow::eventFilter(obj, event);
}

MainWindow::~MainWindow() {
    for (auto it = loadingTimers.begin(); it != loadingTimers.end(); ++it) {
        if (it.value()) it.value()->stop();
    }
    loadingTimers.clear();
    loadingDots.clear();
    savedTitles.clear();
}

// ─── Avatar default ───────────────────────────────────────────────────────────
void MainWindow::setDefaultAvatar() {
    QPixmap pix(28, 28);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor("#bdbdbd"));
    p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, 28, 28);
    p.setBrush(QColor("#f5f5f5"));
    p.drawEllipse(9, 5, 10, 10);
    QPainterPath body;
    body.moveTo(4, 28);
    body.quadTo(4, 18, 14, 18);
    body.quadTo(24, 18, 24, 28);
    body.closeSubpath();
    p.drawPath(body);
    p.end();
    avatarLabel->setPixmap(pix);
    avatarLabel->setToolTip("Klik untuk login dengan Google");
}

// ─── Set avatar dari URL ──────────────────────────────────────────────────────
void MainWindow::setAvatarFromUrl(const QString &imageUrl) {
    QNetworkAccessManager *nam = new QNetworkAccessManager(this);
    QNetworkRequest req{QUrl(imageUrl)};
    req.setRawHeader("User-Agent",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/125.0.0.0 Safari/537.36");
    QNetworkReply *reply = nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QPixmap src;
            if (src.loadFromData(data)) {
                QPixmap rounded(28, 28);
                rounded.fill(Qt::transparent);
                QPainter painter(&rounded);
                painter.setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addEllipse(0, 0, 28, 28);
                painter.setClipPath(path);
                painter.drawPixmap(0, 0, 28, 28, src);
                painter.end();
                avatarLabel->setPixmap(rounded);
            }
        }
        reply->deleteLater();
        nam->deleteLater();
    });
}

// ─── updateAvatarFromCookies ──────────────────────────────────────────────────
void MainWindow::updateAvatarFromCookies() {
    // Login sekarang via OAuth2, method ini tidak digunakan secara aktif.
}

// ─── Salam ────────────────────────────────────────────────────────────────────
QString MainWindow::getGreeting() {
    int hour = QDateTime::currentDateTime().time().hour();
    if (hour >= 5  && hour < 12) return "Selamat Pagi";
    if (hour >= 12 && hour < 15) return "Selamat Siang";
    if (hour >= 15 && hour < 19) return "Selamat Sore";
    return "Selamat Malam";
}

// ─── New tab page ─────────────────────────────────────────────────────────────
QUrl MainWindow::newTabUrl() {
    QString tmpPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                      + "/raisa_newtab.html";
    QString greeting = getGreeting();
    QString html = QString(R"(<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<style>
  * { margin:0; padding:0; box-sizing:border-box; }
  body {
    font-family: 'Segoe UI', sans-serif;
    background: #f5f5f5;
    display: flex;
    align-items: center;
    justify-content: center;
    height: 100vh;
    flex-direction: column;
    gap: 8px;
    user-select: none;
  }
  h1 { font-size:2.2em; font-weight:300; color:#333; letter-spacing:1px; }
  p  { font-size:1em; color:#999; }
</style>
</head>
<body>
  <h1>%1, User.</h1>
  <p>Masukkan URL atau kata kunci di atas untuk mulai menjelajah.</p>
</body>
</html>)").arg(greeting);
    QFile f(tmpPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(html.toUtf8());
        f.close();
    }
    return QUrl::fromLocalFile(tmpPath);
}

// ─── Animasi loading ──────────────────────────────────────────────────────────
void MainWindow::startLoadingAnimation(QWidget *tab) {
    if (!loadingTimers.contains(tab)) {
        QTimer *timer = new QTimer(this);
        loadingTimers[tab] = timer;
        loadingDots[tab]   = 0;
        connect(timer, &QTimer::timeout, [=]() {
            int idx = tabs->indexOf(tab);
            if (idx == -1) { loadingTimers[tab]->stop(); return; }
            loadingDots[tab] = (loadingDots[tab] + 1) % 4;
            int d = loadingDots[tab];
            QString base = savedTitles.value(tab, "Loading");
            if (base.length() > 12) base = base.left(12);
            tabs->setTabText(idx, base + QString(".").repeated(d == 0 ? 1 : d));
        });
    }
    loadingDots[tab] = 0;
    loadingTimers[tab]->start(400);
}

void MainWindow::stopLoadingAnimation(QWidget *tab, const QString &finalTitle) {
    if (loadingTimers.contains(tab)) loadingTimers[tab]->stop();
    int idx = tabs->indexOf(tab);
    if (idx == -1) return;
    QString t = finalTitle.length() > 15 ? finalTitle.left(15) + "..." : finalTitle;
    tabs->setTabText(idx, t.isEmpty() ? "Tab Baru" : t);
}

// ─── Tambah entri riwayat ─────────────────────────────────────────────────────
void MainWindow::addHistoryEntry(const QString &title, const QString &url) {
    if (url.isEmpty() || url.startsWith("file://") ||
        url == "about:blank" || url.startsWith("data:")) return;
    HistoryEntry entry;
    entry.title     = title.isEmpty() ? url : title;
    entry.url       = url;
    entry.timestamp = QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm");
    if (!history.isEmpty() && history.first().url == url) return;
    history.prepend(entry);
    if (history.size() > 200) history.removeLast();
}

// ─── Buat tab baru ────────────────────────────────────────────────────────────
void MainWindow::addNewTab() {
    QWidget     *tab    = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);
    QHBoxLayout *nav    = new QHBoxLayout();
    nav->setSpacing(4);

    QToolButton *backBtn = new QToolButton();
    backBtn->setText("←");
    backBtn->setFixedSize(30, 30);
    backBtn->setEnabled(false);
    backBtn->setToolTip("Kembali ke halaman sebelumnya");
    backBtn->setStyleSheet(
        "QToolButton { font-size:16px; border:none; border-radius:4px; padding:0; color:#444; }"
        "QToolButton:hover:enabled { background:#e8eaed; }"
        "QToolButton:disabled { color:#bbb; }");

    QToolButton *fwdBtn = new QToolButton();
    fwdBtn->setText("→");
    fwdBtn->setFixedSize(30, 30);
    fwdBtn->setEnabled(false);
    fwdBtn->setToolTip("Maju ke halaman berikutnya");
    fwdBtn->setStyleSheet(
        "QToolButton { font-size:16px; border:none; border-radius:4px; padding:0; color:#444; }"
        "QToolButton:hover:enabled { background:#e8eaed; }"
        "QToolButton:disabled { color:#bbb; }");

    QToolButton *reloadBtn = new QToolButton();
    reloadBtn->setText("↺");
    reloadBtn->setFixedSize(30, 30);
    reloadBtn->setToolTip("Muat ulang halaman");
    reloadBtn->setStyleSheet(
        "QToolButton { font-size:16px; border:none; border-radius:4px; padding:0; color:#444; }"
        "QToolButton:hover { background:#e8eaed; }");

    QLabel *ssl = new QLabel("  ");
    ssl->setFixedWidth(24);

    QLineEdit *urlBar = new QLineEdit();
    urlBar->setPlaceholderText("Cari atau masukkan URL...");

    QComboBox *engineCombo = new QComboBox();
    engineCombo->addItems({"Google", "Bing", "DuckDuckGo"});
    engineCombo->setCurrentText(currentSearchEngine);

    QToolButton *menuBtn = new QToolButton();
    menuBtn->setText("≡");
    menuBtn->setFixedSize(30, 30);
    menuBtn->setStyleSheet(
        "QToolButton { font-size:16px; border:none; border-radius:4px; padding:0; color:#444; }"
        "QToolButton:hover { background:#e8eaed; }");

    QMenu   *menu          = new QMenu(menuBtn);
    QAction *historyAction = menu->addAction("📋 Riwayat");
    QAction *themeAction   = menu->addAction("🌙 Ganti Tema");
    menu->addSeparator();
    QAction *loginAction   = menu->addAction("🔑 Login Google");
    QAction *newTabAction  = menu->addAction("➕ Tab Baru");
    menuBtn->setMenu(menu);
    menuBtn->setPopupMode(QToolButton::InstantPopup);

    connect(historyAction, &QAction::triggered, this, &MainWindow::showHistory);
    connect(themeAction,   &QAction::triggered, this, &MainWindow::toggleTheme);
    connect(loginAction,   &QAction::triggered, this, &MainWindow::startGoogleOAuth);
    connect(newTabAction,  &QAction::triggered, this, &MainWindow::addNewTab);

    QWebEngineView *view = new QWebEngineView(tab);
    QWebEnginePage *page = new QWebEnginePage(myProfile, view);
    view->setPage(page);

    connect(page, &QWebEnginePage::permissionRequested,
            this, &MainWindow::handlePermissionRequest);

    auto *s = view->settings();
    s->setAttribute(QWebEngineSettings::JavascriptEnabled,                   true);
    s->setAttribute(QWebEngineSettings::LocalStorageEnabled,                 true);
    s->setAttribute(QWebEngineSettings::AllowRunningInsecureContent,         false);
    s->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled,               false);
    s->setAttribute(QWebEngineSettings::FullScreenSupportEnabled,            true);
    s->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture,         false);
    s->setAttribute(QWebEngineSettings::WebGLEnabled,                        true);
    s->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled,          true);
    s->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, true);
    s->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows,            true);

    // ── Fullscreen handler ────────────────────────────────────────────────────
    connect(page, &QWebEnginePage::fullScreenRequested,
            this, [=](QWebEngineFullScreenRequest req) mutable {
        req.accept();
        if (req.toggleOn()) {
            for (int i = 0; i < nav->count(); ++i) {
                QWidget *w = nav->itemAt(i) ? nav->itemAt(i)->widget() : nullptr;
                if (w) w->hide();
            }
            layout->setContentsMargins(0, 0, 0, 0);
            window()->showFullScreen();
        } else {
            for (int i = 0; i < nav->count(); ++i) {
                QWidget *w = nav->itemAt(i) ? nav->itemAt(i)->widget() : nullptr;
                if (w) w->show();
            }
            layout->setContentsMargins(4, 4, 4, 0);
            window()->showNormal();
        }
    });

    // ── Navbar ───────────────────────────────────────────────────────────────
    nav->addWidget(backBtn);
    nav->addWidget(fwdBtn);
    nav->addWidget(reloadBtn);
    nav->addWidget(ssl);
    nav->addWidget(urlBar, 1);
    nav->addWidget(engineCombo);
    nav->addWidget(menuBtn);

    layout->addLayout(nav);
    layout->addWidget(view);
    layout->setContentsMargins(4, 4, 4, 0);
    layout->setSpacing(2);

    // ── Koneksi tombol navigasi ───────────────────────────────────────────────
    connect(backBtn,   &QToolButton::clicked, view, &QWebEngineView::back);
    connect(fwdBtn,    &QToolButton::clicked, view, &QWebEngineView::forward);
    connect(reloadBtn, &QToolButton::clicked, view, &QWebEngineView::reload);

    auto updateNavButtons = [=]() {
        backBtn->setEnabled(view->history()->canGoBack());
        fwdBtn->setEnabled(view->history()->canGoForward());
    };

    connect(engineCombo, &QComboBox::currentTextChanged, [=](const QString &text) {
        currentSearchEngine = text;
        if      (text == "Google")     searchUrl = "https://www.google.com/search?q=";
        else if (text == "Bing")       searchUrl = "https://www.bing.com/search?q=";
        else if (text == "DuckDuckGo") searchUrl = "https://duckduckgo.com/?q=";
    });

    connect(urlBar, &QLineEdit::returnPressed, [=]() {
        QString txt = urlBar->text().trimmed();
        if (txt.isEmpty()) return;
        QUrl url;
        if (txt.startsWith("http://") || txt.startsWith("https://"))
            url = QUrl(txt);
        else if (txt.contains(".") && !txt.contains(" "))
            url = QUrl("https://" + txt);
        else
            url = QUrl(searchUrl + QUrl::toPercentEncoding(txt));
        view->setUrl(url);
    });

    connect(view, &QWebEngineView::loadStarted, [=]() {
        savedTitles[tab] = "Loading";
        startLoadingAnimation(tab);
        reloadBtn->setText("✕");
        reloadBtn->setToolTip("Hentikan pemuatan");
        disconnect(reloadBtn, &QToolButton::clicked, view, &QWebEngineView::reload);
        connect(reloadBtn,    &QToolButton::clicked, view, &QWebEngineView::stop);
    });

    connect(view, &QWebEngineView::loadFinished, [=](bool) {
        QString title = view->title();
        savedTitles[tab] = title;
        stopLoadingAnimation(tab, title);
        addHistoryEntry(title, view->url().toString());
        updateNavButtons();

        reloadBtn->setText("↺");
        reloadBtn->setToolTip("Muat ulang halaman");
        disconnect(reloadBtn, &QToolButton::clicked, view, &QWebEngineView::stop);
        connect(reloadBtn,    &QToolButton::clicked, view, &QWebEngineView::reload);
    });

    connect(view, &QWebEngineView::urlChanged, [=](const QUrl &url) {
        updateNavButtons();
        if (url.scheme() == "file" || url.scheme() == "data" ||
            url.isEmpty() || url.toString() == "about:blank") {
            urlBar->clear();
            ssl->setText("  ");
            return;
        }
        urlBar->setText(url.toString());
        ssl->setText(url.scheme() == "https" ? "🔒" : "🔓");
    });

    connect(view, &QWebEngineView::titleChanged, [=](const QString &title) {
        savedTitles[tab] = title;
        if (!loadingTimers.contains(tab) || !loadingTimers[tab]->isActive())
            stopLoadingAnimation(tab, title);
    });

    connect(view, &QWebEngineView::iconChanged, [=](const QIcon &icon) {
        int idx = tabs->indexOf(tab);
        if (idx != -1) tabs->setTabIcon(idx, icon);
    });

    connect(tab, &QObject::destroyed, this, [this, tab]() {
        if (loadingTimers.contains(tab)) {
            loadingTimers[tab]->stop();
            loadingTimers[tab]->deleteLater();
            loadingTimers.remove(tab);
        }
        loadingDots.remove(tab);
        savedTitles.remove(tab);
    });

    view->setUrl(newTabUrl());

    int newIndex = tabs->addTab(tab, "Tab Baru");
    tabs->setCurrentIndex(newIndex);
}

// ─── Tutup tab ────────────────────────────────────────────────────────────────
void MainWindow::closeTab(int index) {
    if (tabs->count() <= 1) { close(); return; }
    QWidget *w = tabs->widget(index);
    tabs->removeTab(index);
    w->deleteLater();
}

// ─── Tampilkan panel riwayat ──────────────────────────────────────────────────
void MainWindow::showHistory() {
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle("📋 Riwayat Browsing");
    dlg->resize(640, 480);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vl = new QVBoxLayout(dlg);

    QLineEdit *searchBar = new QLineEdit(dlg);
    searchBar->setPlaceholderText("🔍 Cari riwayat...");
    searchBar->setStyleSheet("padding:6px; border-radius:4px; border:1px solid #ccc; font-size:13px;");

    QListWidget *list = new QListWidget(dlg);
    list->setAlternatingRowColors(true);
    list->setStyleSheet(
        "QListWidget { border:none; font-size:13px; }"
        "QListWidget::item { padding:8px 12px; border-bottom:1px solid #eee; }"
        "QListWidget::item:hover { background:#e8f0fe; }"
        "QListWidget::item:selected { background:#1a73e8; color:white; }"
        "QListWidget::item:alternate { background:#fafafa; }");

    auto populateList = [&](const QString &filter) {
        list->clear();
        if (history.isEmpty()) {
            QListWidgetItem *empty = new QListWidgetItem("Riwayat kosong.");
            empty->setFlags(Qt::NoItemFlags);
            list->addItem(empty);
            return;
        }
        for (const HistoryEntry &e : history) {
            if (!filter.isEmpty() &&
                !e.title.contains(filter, Qt::CaseInsensitive) &&
                !e.url.contains(filter, Qt::CaseInsensitive)) continue;
            QString display = QString("%1\n%2  —  %3")
                              .arg(e.title.left(60))
                              .arg(e.url.left(70))
                              .arg(e.timestamp);
            QListWidgetItem *item = new QListWidgetItem(display);
            item->setData(Qt::UserRole, e.url);
            item->setToolTip(e.url);
            list->addItem(item);
        }
        if (list->count() == 0) {
            QListWidgetItem *empty = new QListWidgetItem("Tidak ada hasil yang cocok.");
            empty->setFlags(Qt::NoItemFlags);
            list->addItem(empty);
        }
    };

    populateList("");

    connect(searchBar, &QLineEdit::textChanged, [=](const QString &text) {
        populateList(text);
    });

    connect(list, &QListWidget::itemDoubleClicked, [=](QListWidgetItem *item) {
        QString url = item->data(Qt::UserRole).toString();
        if (url.isEmpty()) return;
        QWidget *curTab = tabs->currentWidget();
        if (!curTab) { addNewTab(); curTab = tabs->currentWidget(); }
        QWebEngineView *v = curTab->findChild<QWebEngineView*>();
        if (v) v->setUrl(QUrl(url));
        dlg->close();
    });

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *clearBtn  = new QPushButton("🗑 Hapus Semua Riwayat", dlg);
    QPushButton *closeBtn  = new QPushButton("Tutup", dlg);

    clearBtn->setStyleSheet(
        "padding:6px 14px; color:#c0392b; border:1px solid #c0392b; border-radius:4px; background:white;");
    closeBtn->setStyleSheet(
        "padding:6px 14px; background:#1a73e8; color:white; border:none; border-radius:4px;");

    connect(clearBtn, &QPushButton::clicked, [=]() { history.clear(); populateList(""); });
    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);

    btnLayout->addWidget(clearBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);

    vl->addWidget(searchBar);
    vl->addWidget(list);
    vl->addLayout(btnLayout);
    dlg->setLayout(vl);

    if (isDark) {
        dlg->setStyleSheet(
            "QDialog, QWidget { background:#1e1e2e; color:#cdd6f4; }"
            "QLineEdit { background:#313244; color:#cdd6f4; border:1px solid #45475a; }");
    }

    dlg->exec();
}

// ─── Toggle tema ──────────────────────────────────────────────────────────────
void MainWindow::toggleTheme() {
    isDark = !isDark;
    if (isDark) {
        qApp->setStyleSheet(
            "QMainWindow, QWidget { background-color:#1e1e2e; color:#cdd6f4; }"
            "QLineEdit { background-color:#313244; color:#cdd6f4; border:1px solid #45475a; border-radius:4px; padding:2px 6px; }"
            "QTabBar::tab { background:#313244; color:#cdd6f4; padding:4px 12px; }"
            "QTabBar::tab:selected { background:#45475a; }"
            "QToolButton { background:#313244; color:#cdd6f4; border:none; padding:4px; }"
            "QComboBox { background:#313244; color:#cdd6f4; border:1px solid #45475a; }");
    } else {
        qApp->setStyleSheet("");
    }
}

void MainWindow::setEngineGoogle()  { searchUrl = "https://www.google.com/search?q="; currentSearchEngine = "Google"; }
void MainWindow::setEngineBing()    { searchUrl = "https://www.bing.com/search?q=";   currentSearchEngine = "Bing"; }
void MainWindow::setEngineDuck()    { searchUrl = "https://duckduckgo.com/?q=";       currentSearchEngine = "DuckDuckGo"; }
void MainWindow::handleTabChange(int index) { Q_UNUSED(index); }