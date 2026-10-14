// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QSet>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <iostream>
#include <stdexcept>
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static QSet<QString> boundary(const CadDocument& doc){
 for(const auto& b:doc.bodies())for(const auto& f:b.supportFaces)if(f.role=="split.negative.cap"){
  TopTools_IndexedMapOfShape actual;TopExp::MapShapes(f.face,TopAbs_EDGE,actual);QSet<QString> ids;TopTools_IndexedMapOfShape published;int generated=0;
  check(actual.Extent()==18,"actual trimmed seam must have18 edges");
  for(const auto& e:f.boundaryEdges){check(actual.Contains(e.edge),"published boundary must be actual trim");check(!ids.contains(e.edgeId),"unique retained IDs");check(!published.Contains(e.edge),"unique physical edges");published.Add(e.edge);ids.insert(e.edgeId);if(e.provenance["lineage"]=="topological_anchor_chain")++generated;}
  check(ids.size()==actual.Extent() && generated==12,"complete anchored seam publication");return ids;
 }throw std::runtime_error("missing actual split seam");
}
int main(int argc,char**argv){QCoreApplication app(argc,argv);try{
 check(argc==2 || argc==3,"supply actual final enclosure fixture");CadDocument doc;check(doc.open(argv[1]).isEmpty(),"open actual fixture");const auto ids=boundary(doc);
 auto prefix=doc.authored();auto all=prefix["features"].toArray();QJsonArray features;for(const auto& v:all){features.append(v);if(v.toObject()["type"]=="splitBody")break;}prefix["features"]=features;
 check(doc.commit(prefix,"Inspect Split prefix").isEmpty(),"evaluate actual prefix");check(boundary(doc)==ids,"later finishes preserve seam identities");if(argc==3)check(doc.save(argv[2]).isEmpty(),"save prefix evidence");
 auto resized=prefix;bool changed=false;for(int i=0;i<features.size();++i){auto f=features[i].toObject();auto p=f["parameters"].toObject();if(f["type"]=="extrude" && !changed){p["distanceMm"]=31;f["parameters"]=p;features[i]=f;changed=true;}}
 check(changed,"stock extrusion found");resized["features"]=features;check(doc.commit(resized,"Raise stock height").isEmpty(),"upstream height edit");check(boundary(doc)==ids,"IDs survive source size edit");
 check(doc.history().canUndo(),"undo available");doc.history().undo();check(boundary(doc)==ids,"undo retains IDs");check(doc.history().canRedo(),"redo available");doc.history().redo();
 QTemporaryDir dir;auto path=dir.filePath("seam.alloy");check(doc.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty(),"reopen");check(boundary(reopened)==ids,"reopen retains IDs");
 std::cout<<"SPLIT_SEAM_BOUNDARY_PASS edges=18 anchored=12 source_edit=1 history=1 reopen=1\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<std::endl;return 1;}}
