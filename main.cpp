#include "mainwindow.h"
#include <QApplication>
#include <QLoggingCategory>

int main(int argc, char *argv[]) {
    // Flag Chromium — harus di-set SEBELUM QApplication dibuat
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
        " --enable-features=NetworkService,NetworkServiceInProcess"
        " --disable-features=SameSiteByDefaultCookies,CookiesWithoutSameSiteMustBeSecure"
        " --auth-server-whitelist=accounts.google.com"
        " --auth-negotiate-delegate-whitelist=accounts.google.com"
        " --user-agent=Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36"
        " --disable-blink-features=AutomationControlled"
        " --no-first-run"
        " --no-default-browser-check"
        " --enable-gpu-rasterization"
        " --enable-zero-copy"
    );

    // logic
    QLoggingCategory::setFilterRules(
        "qt.webenginecontext.info=false\n"
        "qt.webengine.profile.warning=false\n"
    );

    QApplication a(argc, argv);
    a.setApplicationName("RaisaBrowser");
    a.setApplicationVersion("1.0");

    // ide jelek tapi yaudah
    {
        // MainWindow dibungkus dalam scope tersendiri agar destruktornya
        // dipanggil SEBELUM QApplication hancur. Ini memastikan semua
        // QWebEngineView dan timer sudah bersih sebelum font cache Qt
        // dihapus — mencegah crash ABRT di QFontEngineFT::QGlyphSet::clear.
        MainWindow w;
        w.show();
        return a.exec();
    }

    // perbaiki sebelum deploy
}