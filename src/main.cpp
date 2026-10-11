#include "AlloyWindow.h"
#include "ControlServer.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStyleFactory>
#include <QTemporaryDir>
#include <QTimer>
#include <cstdio>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("Alloy");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Alloy");
    app.setStyle(QStyleFactory::create("Windows")); // Public Qt widget style.
    QCommandLineParser parser;
    parser.setApplicationDescription("Alloy native CAD shell — modeling is not implemented yet.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"control-socket", "Local socket path in an existing private directory.", "path"});
    parser.addOption({"ready-file", "Write endpoint discovery JSON when the real window is ready.", "path"});
    parser.process(app);

    QTemporaryDir privateDirectory(QDir::tempPath() + "/alloy-XXXXXX");
    if (!privateDirectory.isValid()) return 2;
    const QString endpoint = parser.isSet("control-socket") ? parser.value("control-socket")
        : privateDirectory.path() + "/control.sock";
    const QFileInfo parent(QFileInfo(endpoint).absolutePath());
    const auto unsafe = QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup
        | QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther;
    if (!QDir::isAbsolutePath(endpoint) || !parent.isDir() || (parent.permissions() & unsafe)) {
        std::fprintf(stderr, "Control socket requires an absolute path in an existing private (0700) directory.\n");
        return 2;
    }
    AlloyWindow window;
    ControlServer server(window);
    if (!server.listen(endpoint)) {
        std::fprintf(stderr, "Cannot listen: %s\n", qPrintable(server.errorString()));
        return 2;
    }
    window.show();
    QTimer::singleShot(0, &window, [&] {
        const QJsonObject ready{{"protocol", "alloy-control/1"}, {"socket", server.fullServerName()},
            {"pid", QCoreApplication::applicationPid()}, {"status", "ready"}};
        const auto bytes = QJsonDocument(ready).toJson(QJsonDocument::Compact) + '\n';
        if (parser.isSet("ready-file")) {
            QSaveFile file(parser.value("ready-file"));
            if (!file.open(QIODevice::WriteOnly)) {
                std::fprintf(stderr, "Cannot write ready file.\n");
                app.exit(2);
                return;
            }
            file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            if (file.write(bytes) != bytes.size() || !file.commit()) {
                app.exit(2);
                return;
            }
        }
        std::fwrite(bytes.constData(), 1, static_cast<size_t>(bytes.size()), stdout);
        std::fflush(stdout);
    });
    return app.exec();
}
