#include "sessionmanager.h"
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineUrlRequestInterceptor>
#include <QDir>

QWebEngineProfile* SessionManager::s_profile     = nullptr;
QString            SessionManager::s_pendingToken = QString();

// ─────────────────────────────────────────────────────────────────────────────
// Request Interceptor
//
// Fungsi utama:
//   1. Override User-Agent agar konsisten sebagai Chrome desktop
//
// CATATAN: Sec-Fetch-* headers TIDAK di-override untuk SubFrame.
// GitHub (dan banyak situs modern) menggunakan <turbo-frame> / lazy-loading
// subframe untuk memuat konten parsial saat scroll. Jika SubFrame mendapat
// Sec-Fetch-Site: none + Sec-Fetch-Mode: navigate, server GitHub akan
// memperlakukannya sebagai navigasi top-level baru → merespons dengan
// full-page redirect → subframe me-load ulang → looping tak terhenti.
//
// Hanya MainFrame yang boleh mendapat Sec-Fetch-* override, itupun hanya
// ketika benar-benar navigasi langsung dari user (bukan redirect chain).
// ─────────────────────────────────────────────────────────────────────────────
class RequestInterceptor : public QWebEngineUrlRequestInterceptor {
public:
    explicit RequestInterceptor(QObject *parent = nullptr)
        : QWebEngineUrlRequestInterceptor(parent) {}

    void interceptRequest(QWebEngineUrlRequestInfo &info) override {
        // Override UA agar konsisten dengan flag --user-agent di main.cpp
        info.setHttpHeader("User-Agent",
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
            "AppleWebKit/537.36 (KHTML, like Gecko) "
            "Chrome/125.0.0.0 Safari/537.36");

        // Jangan set X-Requested-With sama sekali — string kosong ("") tetap
        // dikirim sebagai header kosong dan bisa membingungkan server.
        // Header ini hanya perlu dihapus jika Chromium menambahkannya secara
        // default, yang tidak terjadi pada versi QtWebEngine modern.

        // Hanya tambahkan Sec-Fetch-* pada navigasi MainFrame (top-level).
        // SubFrame TIDAK boleh disentuh — turbo-frame, lazy-load iframe, dll.
        // mengandalkan nilai Sec-Fetch-* yang dihitung browser secara otomatis.
        auto resType = info.resourceType();
        if (resType == QWebEngineUrlRequestInfo::ResourceTypeMainFrame) {
            info.setHttpHeader("Sec-Fetch-Mode", "navigate");
            info.setHttpHeader("Sec-Fetch-Dest", "document");
            info.setHttpHeader("Sec-Fetch-Site", "none");
            info.setHttpHeader("Sec-Fetch-User", "?1");
            info.setHttpHeader("Upgrade-Insecure-Requests", "1");
        }
        // SubFrame, XHR, Fetch, dll. → biarkan Chromium menghitung
        // Sec-Fetch-* secara otomatis sesuai konteks request yang sesungguhnya.
    }
};

// Dipanggil setelah OAuth2 — tidak digunakan lagi, dipertahankan untuk kompatibilitas
void SessionManager::setOAuthToken(const QString &token) {
    s_pendingToken = token;
}

void SessionManager::clearOAuthToken() {
    s_pendingToken.clear();
}

QWebEngineProfile* SessionManager::createProfile() {
    if (s_profile) return s_profile;

    QString path = QDir::homePath() + "/.raisa_browser_data";
    QDir().mkpath(path);
    QDir().mkpath(path + "/cache");

    s_profile = new QWebEngineProfile("RaisaBrowserProfile");
    s_profile->setPersistentStoragePath(path);
    s_profile->setCachePath(path + "/cache");

    s_profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
    s_profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    s_profile->setHttpCacheMaximumSize(150 * 1024 * 1024);

    // UA harus konsisten dengan flag --user-agent di main.cpp
    s_profile->setHttpUserAgent(
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/125.0.0.0 Safari/537.36"
    );

    auto *settings = s_profile->settings();
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled,                    true);
    settings->setAttribute(QWebEngineSettings::LocalStorageEnabled,                  true);
    settings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled,                false);
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled,             true);
    settings->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript,  true);
    settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows,             true);
    settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard,         true);
    settings->setAttribute(QWebEngineSettings::WebGLEnabled,                         true);
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled,           true);
    settings->setAttribute(QWebEngineSettings::AutoLoadImages,                       true);
    settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled,                   true);
    settings->setAttribute(QWebEngineSettings::PdfViewerEnabled,                     true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls,      true);

    s_profile->setUrlRequestInterceptor(new RequestInterceptor(s_profile));

    return s_profile;
}