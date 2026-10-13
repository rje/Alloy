// SPDX-License-Identifier: MIT
#include "PrivateDirectory.h"
#include <QCoreApplication>
#include <QTest>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTemporaryDir>
#include <QProcess>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <aclapi.h>
#else
#include <QFile>
#endif
QString helper,python,clientSource;
class TransportTest final:public QObject {
 Q_OBJECT
private slots:
 void directoryPrivacy(){QTemporaryDir tmp;QString error;const auto directory=tmp.path()+"/private";QVERIFY(alloyPrivateDirectory(directory,true,&error));QVERIFY(alloyPrivateDirectory(directory,false,&error));QVERIFY(!alloyPrivateDirectory("relative",true,&error));
#ifdef Q_OS_WIN
  // An own-only but noninheritable policy cannot protect staged child files.
  auto privatePath=directory.toStdWString();PSECURITY_DESCRIPTOR descriptor=nullptr;PACL ownAcl=nullptr;
  QCOMPARE(GetNamedSecurityInfoW(privatePath.data(),SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,nullptr,nullptr,&ownAcl,nullptr,&descriptor),DWORD(ERROR_SUCCESS));
  void* ownEntry=nullptr;QVERIFY(GetAce(ownAcl,0,&ownEntry));static_cast<ACE_HEADER*>(ownEntry)->AceFlags=0;
  QCOMPARE(SetNamedSecurityInfoW(privatePath.data(),SE_FILE_OBJECT,DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION,nullptr,nullptr,ownAcl,nullptr),DWORD(ERROR_SUCCESS));LocalFree(descriptor);
  QVERIFY(!alloyPrivateDirectory(directory,false,&error));
  SID_IDENTIFIER_AUTHORITY authority=SECURITY_WORLD_SID_AUTHORITY;PSID everyone=nullptr;QVERIFY(AllocateAndInitializeSid(&authority,1,SECURITY_WORLD_RID,0,0,0,0,0,0,0,&everyone));EXPLICIT_ACCESSW access{};access.grfAccessPermissions=FILE_ALL_ACCESS;access.grfAccessMode=SET_ACCESS;access.grfInheritance=SUB_CONTAINERS_AND_OBJECTS_INHERIT;access.Trustee.TrusteeForm=TRUSTEE_IS_SID;access.Trustee.ptstrName=static_cast<LPWSTR>(everyone);PACL acl=nullptr;QCOMPARE(SetEntriesInAclW(1,&access,nullptr,&acl),DWORD(ERROR_SUCCESS));auto path=directory.toStdWString();QCOMPARE(SetNamedSecurityInfoW(path.data(),SE_FILE_OBJECT,DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION,nullptr,nullptr,acl,nullptr),DWORD(ERROR_SUCCESS));LocalFree(acl);FreeSid(everyone);
#else
  QFile::setPermissions(directory,QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner|QFileDevice::ReadOther);
#endif
  QVERIFY(!alloyPrivateDirectory(directory,false,&error));
 }
 void actualByteTransport(){QTemporaryDir tmp;QLocalServer server;server.setSocketOptions(QLocalServer::UserAccessOption);
#ifdef Q_OS_WIN
  const auto endpoint="alloy-test-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
#else
  const auto endpoint=tmp.path()+"/control.sock";
#endif
  QVERIFY(server.listen(endpoint));connect(&server,&QLocalServer::newConnection,&server,[&]{while(auto* socket=server.nextPendingConnection()){connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);connect(socket,&QLocalSocket::readyRead,socket,[socket]{if(!socket->canReadLine())return;QJsonParseError error;const auto request=QJsonDocument::fromJson(socket->readLine(),&error).object();const auto mode=request["command"].toString();if(mode=="silent")return;if(mode=="partial"){socket->write("{");socket->disconnectFromServer();return;}if(mode=="large"){socket->write(QByteArray(1048577,'x')+'\n');return;}QJsonObject response{{"id",mode=="wrong" ? QJsonValue("other") : request["id"]},{"ok",error.error==QJsonParseError::NoError}};if(error.error!=QJsonParseError::NoError)response["error"]=QJsonObject{{"code","invalid_request"}};const auto bytes=QJsonDocument(response).toJson(QJsonDocument::Compact)+'\n';socket->write(bytes.first(4));socket->flush();QTimer::singleShot(20,socket,[socket,bytes]{socket->write(bytes.mid(4));socket->flush();});});}});
  auto run=[&](const QByteArray& bytes,int expected,const QString& timeout="1000"){QProcess process;process.start(helper,{"--endpoint",server.fullServerName(),"--timeout-ms",timeout});QVERIFY(process.waitForStarted());process.write(bytes);process.closeWriteChannel();QTRY_COMPARE_WITH_TIMEOUT(process.state(),QProcess::NotRunning,3000);QCOMPARE(process.exitCode(),expected);if(expected==0){const auto output=process.readAllStandardOutput();if(bytes.contains("echo"))QCOMPARE(output,QByteArray("{\"id\":\"native\",\"ok\":true}\n"));else QVERIFY(output.endsWith('\n'));}else QVERIFY(!process.readAllStandardError().isEmpty());};
  run("{\"id\":\"native\",\"command\":\"echo\"}\n",0);run("not JSON\n",0);run("{\"command\":\"partial\"}\n",2);run("{\"command\":\"silent\"}\n",2,"100");run("{\"command\":\"large\"}\n",2);run(QByteArray(65536,'x')+'\n',2);
  // Python, not the byte bridge, owns response correlation validation.
  QProcess correlation;const auto script=QStringLiteral("import importlib.util,sys; s=importlib.util.spec_from_file_location('alloyctl',sys.argv[1]); m=importlib.util.module_from_spec(s); s.loader.exec_module(m)\nassert m.request(sys.argv[2],'echo',timeout=2,helper=sys.argv[3])['ok']\ntry: m.request(sys.argv[2],'wrong',timeout=2,helper=sys.argv[3])\nexcept RuntimeError as e: sys.exit(0 if str(e)=='Response ID does not match request' else 2)\nsys.exit(1)");
  correlation.start(python,{"-c",script,clientSource,server.fullServerName(),helper});QVERIFY(correlation.waitForStarted());QTRY_COMPARE_WITH_TIMEOUT(correlation.state(),QProcess::NotRunning,5000);QCOMPARE(correlation.exitCode(),0);
  QProcess missing;missing.start(helper,{"--endpoint",server.fullServerName()+"-missing","--timeout-ms","100"});QVERIFY(missing.waitForStarted());missing.write("{}\n");missing.closeWriteChannel();QTRY_COMPARE_WITH_TIMEOUT(missing.state(),QProcess::NotRunning,3000);QCOMPARE(missing.exitCode(),2);
#ifdef Q_OS_WIN
  QProcess remote;remote.start(helper,{"--endpoint","\\\\remote\\pipe\\bad"});QVERIFY(remote.waitForStarted());remote.write("{}\n");remote.closeWriteChannel();QTRY_COMPARE_WITH_TIMEOUT(remote.state(),QProcess::NotRunning,3000);QCOMPARE(remote.exitCode(),2);
#endif
 }
};
int main(int argc,char** argv){QCoreApplication app(argc,argv);if(argc<4)return 2;helper=QString::fromLocal8Bit(argv[1]);python=QString::fromLocal8Bit(argv[2]);clientSource=QString::fromLocal8Bit(argv[3]);TransportTest test;return QTest::qExec(&test,1,argv);}
#include "transport_test.moc"
