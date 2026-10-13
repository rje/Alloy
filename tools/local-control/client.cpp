// SPDX-License-Identifier: MIT
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QFile>
#include <QLocalSocket>
#include <cstdio>
#include <algorithm>
#ifdef Q_OS_WIN
#include <fcntl.h>
#include <io.h>
#endif
int main(int argc,char** argv) {
 QCoreApplication app(argc,argv);app.setApplicationName("alloy-local-client");app.setApplicationVersion(QT_VERSION_STR);
 QCommandLineParser parser;parser.addHelpOption();parser.addVersionOption();parser.addOption({"endpoint","Local endpoint address","address"});parser.addOption({"timeout-ms","Overall socket deadline","milliseconds","30000"});parser.process(app);
 auto fail=[](const char* message){std::fprintf(stderr,"%s\n",message);return 2;};
 bool valid=false;const auto timeout=parser.value("timeout-ms").toInt(&valid);if(!valid || timeout<1)return fail("Deadline must be a positive integer");
 const auto endpoint=parser.value("endpoint");
#ifdef Q_OS_WIN
 if(endpoint.size()<=9 || !endpoint.startsWith(QStringLiteral("\\\\.\\pipe\\")) || endpoint.mid(9).contains('\\') || endpoint.mid(9).contains('/'))return fail("Only local Windows named pipes are supported");
#else
 if(!endpoint.startsWith('/'))return fail("Unix endpoint requires an absolute path");
#endif
 #ifdef Q_OS_WIN
 if(_setmode(_fileno(stdin),_O_BINARY)==-1 || _setmode(_fileno(stdout),_O_BINARY)==-1)return fail("Cannot configure byte-stream input/output");
#endif
 QFile input;if(!input.open(stdin,QIODevice::ReadOnly))return fail("Cannot read request input");const auto message=input.readLine(65537);
 if(message.isEmpty() || message.size()>65536 || !message.endsWith('\n'))return fail("Request must be one newline frame at most 64 KiB");
 QElapsedTimer clock;clock.start();auto remaining=[&](){return static_cast<int>(std::max<qint64>(0,timeout-clock.elapsed()));};
 QLocalSocket socket;socket.setReadBufferSize(1048577);socket.connectToServer(endpoint);
 if(!socket.waitForConnected(remaining()))return fail("Local connection failed or deadline expired");
 if(socket.write(message)!=message.size())return fail("Cannot queue complete request");
 while(socket.bytesToWrite()>0)if(remaining()==0 || !socket.waitForBytesWritten(remaining()))return fail("Request write failed or deadline expired");
 QByteArray response;
 while(true){response+=socket.readAll();const auto end=response.indexOf('\n');if(response.size()>1048576)return fail("Response exceeds 1 MiB");
  if(end>=0){const auto frame=response.first(end+1);if(std::fwrite(frame.constData(),1,frame.size(),stdout)!=static_cast<size_t>(frame.size()))return fail("Cannot emit response");return 0;}
  if(remaining()==0 || !socket.waitForReadyRead(remaining()))return fail("Response incomplete or deadline expired");
 }
}
