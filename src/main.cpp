#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ControlServer.h"
#include "PrivateDirectory.h"
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
    parser.setApplicationDescription("Alloy parametric CAD with local agent control.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"control-socket", "Unix socket path in an existing private directory; Windows generates UUID pipes.", "path"});
    parser.addOption({"ready-file", "Write endpoint discovery JSON when the real window is ready.", "path"});
    parser.addOption({"instance-dir", "Private instance discovery directory (default ~/.alloy/instances).", "path", QDir::homePath() + "/.alloy/instances"});
    parser.process(app);

    const auto instanceDirectory = parser.value("instance-dir");
    QString privacyError;
    if(!alloyPrivateDirectory(instanceDirectory,true,&privacyError)) {
        std::fprintf(stderr,"Cannot use private discovery directory: %s\n",qPrintable(privacyError));return 2;
    }
#ifdef Q_OS_WIN
    if(parser.isSet("ready-file") && !alloyPrivateDirectory(QFileInfo(parser.value("ready-file")).absolutePath(),true,&privacyError)) {
        std::fprintf(stderr,"Cannot use private ready-file directory: %s\n",qPrintable(privacyError));return 2;
    }
#endif
    AlloyWindow window;
    QTemporaryDir privateDirectory(QDir::tempPath() + "/alloy-XXXXXX");
    if (!privateDirectory.isValid()) return 2;
    QString endpoint;
#ifdef Q_OS_WIN
    // Qt allows duplicate pipe listeners; bounded startup always uses a fresh UUID.
    if(parser.isSet("control-socket")) {
        std::fprintf(stderr,"Custom Windows endpoints are unsupported; use the generated local pipe in discovery.\n");return 2;
    }
    endpoint="alloy-"+window.identity()["instanceId"].toString();
    const QString transport="windows-named-pipe";
#else
    endpoint=parser.isSet("control-socket") ? parser.value("control-socket") : privateDirectory.path()+"/control.sock";
    if(!QDir::isAbsolutePath(endpoint) || !alloyPrivateDirectory(QFileInfo(endpoint).absolutePath(),false,&privacyError)) {
        std::fprintf(stderr,"Control socket requires an absolute path in an existing private directory: %s\n",qPrintable(privacyError));return 2;
    }
    const QString transport="unix-socket";
#endif
    ControlServer server(window);
    if (!server.listen(endpoint)) {
        std::fprintf(stderr, "Cannot listen: %s\n", qPrintable(server.errorString()));return 2;
    }
    window.show();
    const auto instancePath = instanceDirectory + "/" + window.identity()["instanceId"].toString() + ".json";
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [instancePath] { QFile::remove(instancePath); });
    QTimer::singleShot(0, &window, [&] {
        auto ready = window.identity(); ready["socket"] = server.fullServerName(); ready["endpoint"]=server.fullServerName();ready["transport"]=transport; ready["status"] = "ready";
        const auto bytes = QJsonDocument(ready).toJson(QJsonDocument::Compact) + '\n';
        { QSaveFile registry(instancePath);
          if (!registry.open(QIODevice::WriteOnly)) { app.exit(2); return; }
#ifndef Q_OS_WIN
          registry.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
#endif
          if (registry.write(bytes) != bytes.size() || !registry.commit()) { app.exit(2); return; }
        }
        if (parser.isSet("ready-file")) {
            QSaveFile file(parser.value("ready-file"));
            if (!file.open(QIODevice::WriteOnly)) {
                std::fprintf(stderr, "Cannot write ready file.\n");
                app.exit(2);
                return;
            }
#ifndef Q_OS_WIN
            file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
#endif
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
