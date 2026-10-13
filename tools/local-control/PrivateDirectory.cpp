// SPDX-License-Identifier: MIT
#include "PrivateDirectory.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <aclapi.h>
#include <QByteArray>
#endif
bool alloyPrivateDirectory(const QString& path,bool create,QString* error) {
 auto fail=[&](const QString& message){if(error)*error=message;return false;};
 if(!QDir::isAbsolutePath(path))return fail("Private directory requires an absolute path");
 const bool existed=QFileInfo::exists(path);
 if(!existed && (!create || !QDir().mkpath(path)))return fail("Cannot create private directory");
 if(!QFileInfo(path).isDir() || QFileInfo(path).isSymLink())return fail("Private directory must be a directory, not a symbolic link");
#ifdef Q_OS_WIN
 HANDLE token=nullptr;if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))return fail("Cannot inspect current-user token");
 DWORD length=0;GetTokenInformation(token,TokenUser,nullptr,0,&length);QByteArray storage(length,0);
 const bool tokenOk=GetTokenInformation(token,TokenUser,storage.data(),length,&length);CloseHandle(token);
 if(!tokenOk)return fail("Cannot inspect current-user SID");
 const auto sid=reinterpret_cast<TOKEN_USER*>(storage.data())->User.Sid;
 auto native=QDir::toNativeSeparators(path).toStdWString();
 if(!existed) {
  EXPLICIT_ACCESSW access{};access.grfAccessPermissions=FILE_ALL_ACCESS;access.grfAccessMode=SET_ACCESS;access.grfInheritance=SUB_CONTAINERS_AND_OBJECTS_INHERIT;
  access.Trustee.TrusteeForm=TRUSTEE_IS_SID;access.Trustee.TrusteeType=TRUSTEE_IS_USER;access.Trustee.ptstrName=static_cast<LPWSTR>(sid);
  PACL acl=nullptr;const auto built=SetEntriesInAclW(1,&access,nullptr,&acl);
  const auto applied=built==ERROR_SUCCESS ? SetNamedSecurityInfoW(native.data(),SE_FILE_OBJECT,OWNER_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION,sid,nullptr,acl,nullptr) : built;
  if(acl)LocalFree(acl);if(applied!=ERROR_SUCCESS)return fail("Cannot protect private directory ACL");
 }
 PSECURITY_DESCRIPTOR descriptor=nullptr;PSID owner=nullptr;PACL acl=nullptr;
 const auto status=GetNamedSecurityInfoW(native.data(),SE_FILE_OBJECT,OWNER_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION,&owner,nullptr,&acl,nullptr,&descriptor);
 if(status!=ERROR_SUCCESS)return fail("Cannot inspect private directory ACL");
 SECURITY_DESCRIPTOR_CONTROL control=0;DWORD revision=0;
 const bool protectedAcl=GetSecurityDescriptorControl(descriptor,&control,&revision) && (control&SE_DACL_PROTECTED);
 bool safe=protectedAcl && owner && EqualSid(owner,sid) && acl && acl->AceCount>0;bool ownAccess=false;
 for(DWORD i=0;safe && i<acl->AceCount;++i){void* entry=nullptr;if(!GetAce(acl,i,&entry)){safe=false;break;}const auto header=static_cast<ACE_HEADER*>(entry);
  if(header->AceType==ACCESS_ALLOWED_ACE_TYPE){const auto allowed=static_cast<ACCESS_ALLOWED_ACE*>(entry);const auto trustee=reinterpret_cast<PSID>(&allowed->SidStart);if(!EqualSid(trustee,sid))safe=false;else if(!(header->AceFlags&INHERIT_ONLY_ACE) && (header->AceFlags&(OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE))==(OBJECT_INHERIT_ACE|CONTAINER_INHERIT_ACE) && (allowed->Mask&FILE_ALL_ACCESS)==FILE_ALL_ACCESS)ownAccess=true;}
  else safe=false; // Unknown/complex inherited policies are not privacy evidence.
 }
 if(descriptor)LocalFree(descriptor);
 return safe && ownAccess ? true : fail("Private directory must be owned by and grant access only to the current user");
#else
 if(!existed && !QFile::setPermissions(path,QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner))return fail("Cannot protect private directory");
 const auto unsafe=QFileDevice::ReadGroup|QFileDevice::WriteGroup|QFileDevice::ExeGroup|QFileDevice::ReadOther|QFileDevice::WriteOther|QFileDevice::ExeOther;
 return !(QFileInfo(path).permissions()&unsafe) ? true : fail("Private directory requires 0700 permissions");
#endif
}
