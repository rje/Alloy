// Independent OCCT STEP writer/reader used to qualify Alloy interchange.
// This executable deliberately does not link or call Alloy's interchange code.
#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <Bnd_Box.hxx>
#include <DESTEP_Parameters.hxx>
#include <GProp_GProps.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <TColStd_SequenceOfAsciiString.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <UnitsMethods_LengthUnit.hxx>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
void printJson(const QJsonObject& object) {std::cout<<QJsonDocument(object).toJson(QJsonDocument::Compact).toStdString()<<'\n';}
}

int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        const auto args=app.arguments();
        require(args.size()>=3,"usage: step_fixture write|read path [mm|in] [single|multi]");
        const auto path=args[2].toUtf8();
        if(args[1]=="write") {
            require(args.size()==5,"write requires units and single|multi");
            require(args[3]=="mm" || args[3]=="in","unsupported writer unit");
            require(args[4]=="single" || args[4]=="multi","unsupported fixture shape count");
            STEPControl_Writer writer;DESTEP_Parameters parameters;
            parameters.WriteUnit=args[3]=="in" ? UnitsMethods_LengthUnit_Inch : UnitsMethods_LengthUnit_Millimeter;
            require(writer.Transfer(BRepPrimAPI_MakeBox(25.4,50.8,12.7).Shape(),STEPControl_ManifoldSolidBrep,parameters)==IFSelect_RetDone,"first transfer failed");
            if(args[4]=="multi")
                require(writer.Transfer(BRepPrimAPI_MakeBox(gp_Pnt(100,0,0),10,10,10).Shape(),STEPControl_ManifoldSolidBrep,parameters)==IFSelect_RetDone,"second transfer failed");
            require(writer.Write(path.constData())==IFSelect_RetDone,"STEP write failed");
            printJson({{"written",true},{"units",args[3]},{"solidCount",args[4]=="multi" ? 2 : 1}});
            return 0;
        }
        require(args[1]=="read" && args.size()==3,"read takes one path");
        STEPControl_Reader reader;require(reader.ReadFile(path.constData())==IFSelect_RetDone,"STEP read failed");
        TColStd_SequenceOfAsciiString length,angle,solidAngle;reader.FileUnits(length,angle,solidAngle);
        QJsonArray units;for(Standard_Integer index=1;index<=length.Length();++index)units.append(QString::fromUtf8(length.Value(index).ToCString()));
        reader.SetSystemLengthUnit(1.0);const auto roots=reader.NbRootsForTransfer();
        const auto transferred=reader.TransferRoots();require(transferred==roots,"incomplete transfer");
        const auto shape=reader.OneShape();TopTools_IndexedMapOfShape solids;TopExp::MapShapes(shape,TopAbs_SOLID,solids);
        require(solids.Extent()==1,"expected exactly one independent solid");
        const auto solid=solids(1);Bnd_Box bounds;BRepBndLib::Add(solid,bounds);
        Standard_Real x1,y1,z1,x2,y2,z2;bounds.Get(x1,y1,z1,x2,y2,z2);
        GProp_GProps mass;BRepGProp::VolumeProperties(solid,mass);
        printJson({{"valid",BRepCheck_Analyzer(solid).IsValid()},{"solidCount",solids.Extent()},
            {"fileLengthUnits",units},{"rootCount",roots},{"transferredRoots",transferred},
            {"boundsMm",QJsonArray{x1,y1,z1,x2,y2,z2}},{"volumeMm3",mass.Mass()}});
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
