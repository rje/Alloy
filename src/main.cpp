#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ControlServer.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTimer>
#include <cstdio>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("Alloy");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Alloy");
    AlloyTheme::apply(app);
    QCommandLineParser parser;
    parser.setApplicationDescription("Alloy native CAD shell — modeling is not implemented yet.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"control-socket", "Local socket path in an existing private directory.", "path"});
    parser.addOption({"ready-file", "Write endpoint discovery JSON when the real window is ready.", "path"});
    parser.addOption({"instance-dir", "Private instance discovery directory (default ~/.alloy/instances).", "path", QDir::homePath() + "/.alloy/instances"});
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
    const auto instanceDirectory = parser.value("instance-dir");
    const bool existed = QFileInfo::exists(instanceDirectory);
    if (!QDir::isAbsolutePath(instanceDirectory) || !QDir().mkpath(instanceDirectory)
        || (!existed && !QFile::setPermissions(instanceDirectory, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner))
        || (QFileInfo(instanceDirectory).permissions() & unsafe)) {
        std::fprintf(stderr, "Cannot create private instance discovery directory.\n"); return 2;
    }
    const auto instancePath = instanceDirectory + "/" + window.identity()["instanceId"].toString() + ".json";
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [instancePath] { QFile::remove(instancePath); });
    QTimer::singleShot(0, &window, [&] {
        auto ready = window.identity(); ready["socket"] = server.fullServerName(); ready["status"] = "ready";
        const auto bytes = QJsonDocument(ready).toJson(QJsonDocument::Compact) + '\n';
        { QSaveFile registry(instancePath);
          if (!registry.open(QIODevice::WriteOnly)) { app.exit(2); return; }
          registry.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
          if (registry.write(bytes) != bytes.size() || !registry.commit()) { app.exit(2); return; }
        }
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
