// SPDX-License-Identifier: MIT
#include "ZebraPresentation.h"
#include <AIS_ColoredDrawer.hxx>
#include <Graphic3d_ShaderObject.hxx>

Handle(Graphic3d_ShaderProgram) ZebraPresentation::shader() {
    // One resource shared across faces and camera changes. OCCT supplies its
    // standard attributes/matrices and interpolated B-rep tessellation normals.
    static const Handle(Graphic3d_ShaderProgram) program=[] {
        Handle(Graphic3d_ShaderProgram) result=new Graphic3d_ShaderProgram;
        result->SetHeader("#version 150");
        result->AttachShader(Graphic3d_ShaderObject::CreateFromSource(Graphic3d_TOS_VERTEX,R"GLSL(
THE_SHADER_OUT vec3 zebraNormal;
THE_SHADER_OUT vec3 zebraPosition;
THE_SHADER_OUT vec4 zebraWorld;
void main() {
    zebraWorld = occModelWorldMatrix * occVertex;
    vec4 eye = occWorldViewMatrix * zebraWorld;
    zebraPosition = eye.xyz / eye.w;
    zebraNormal = (occWorldViewMatrixInverseTranspose * occModelWorldMatrixInverseTranspose * vec4(occNormal, 0.0)).xyz;
    gl_Position = occProjectionMatrix * eye;
}
)GLSL"));
        result->AttachShader(Graphic3d_ShaderObject::CreateFromSource(Graphic3d_TOS_FRAGMENT,R"GLSL(
THE_SHADER_IN vec3 zebraNormal;
THE_SHADER_IN vec3 zebraPosition;
THE_SHADER_IN vec4 zebraWorld;
void main() {
    if (occFragEarlyReturn()) return;
    for (int i = 0; i < occClipPlaneCount; ++i) {
        if (dot(occClipPlaneEquations[i], vec4(zebraWorld.xyz / zebraWorld.w, 1.0)) < 0.0) discard;
    }
    vec3 normal = normalize(zebraNormal);
    vec3 incident = occProjectionMatrix[3][3] == 0.0 ? normalize(zebraPosition) : vec3(0.0, 0.0, -1.0);
    // A view-fixed striped environment. Reflection is unchanged when the
    // sheet's normal reverses, so open sheets have useful bands on both sides.
    vec3 reflected = reflect(incident, normal);
    float wave = sin(12.0 * atan(reflected.x, reflected.z));
    float feather = max(fwidth(wave), 0.015);
    float band = smoothstep(-feather, feather, wave);
    vec3 stripe = mix(vec3(0.025, 0.032, 0.043), vec3(0.94, 0.91, 0.84), band);
    vec4 material = occMaterial_Diffuse(gl_FrontFacing);
    occSetFragColor(vec4(mix(material.rgb, stripe, 0.9), material.a));
}
)GLSL"));
        return result;
    }();
    return program;
}
void ZebraPresentation::apply(const Handle(AIS_ColoredShape)& presentation,const TopoDS_Face& face) {
    presentation->CustomAspects(face)->SetShaderProgram(shader(),Graphic3d_ASPECT_FILL_AREA,true);
}
void ZebraPresentation::remove(const Handle(AIS_ColoredShape)& presentation,const TopoDS_Face& face) {
    presentation->UnsetCustomAspects(face,true);
}
