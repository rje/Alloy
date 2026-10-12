#!/usr/bin/env python3
"""Download pinned public solver sources into ignored evidence; compile a shared-library spike."""
import argparse
import hashlib
import json
import pathlib
import platform
import re
import subprocess
import urllib.request

REVISION = 'ec6852e6caadf55045881a6003cc7ad038c8d2a7'
BASE = f'https://raw.githubusercontent.com/FreeCAD/FreeCAD/{REVISION}/'
FILES = [f'src/Mod/Sketcher/App/planegcs/{n}' for n in
         ('GCS.cpp', 'GCS.h', 'Geo.cpp', 'Geo.h', 'Constraints.cpp', 'Constraints.h',
          'SubSystem.cpp', 'SubSystem.h', 'Util.h', 'qp_eq.cpp', 'qp_eq.h')]
FILES += ['src/Mod/Sketcher/SketcherGlobal.h', 'src/boost_graph_adjacency_list.hpp']

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', default='build/evidence/FND-005')
    parser.add_argument('--prefix', default='/opt/homebrew')
    args = parser.parse_args()
    out = pathlib.Path(args.output).resolve(); out.mkdir(parents=True, exist_ok=True)
    source = out/'source'; shims = out/'shims'; (shims/'Base').mkdir(parents=True, exist_ok=True)
    prefix=pathlib.Path(args.prefix)
    eigen_version=prefix/'include/eigen3/Eigen/Version'
    boost_version=prefix/'include/boost/version.hpp'
    eigen_numbers=[int(re.search(r'#define EIGEN_'+part+r'_VERSION (\d+)',eigen_version.read_text())[1]) for part in ('MAJOR','MINOR','PATCH')]
    if eigen_numbers != [5,0,1] or '#define BOOST_LIB_VERSION "1_90"' not in boost_version.read_text():
        raise RuntimeError('This bounded fixture pins Eigen 5.0.1 and Boost 1.90.0 headers')
    upstream = {}
    for name in FILES:
        path = source/name; path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(urllib.request.urlopen(BASE+name).read())
        upstream[name] = {'url': BASE+name, 'sha256': sha(path)}
    # C++20 portability only: replace the C++23 impossible-enum hint by fail-fast.
    # This preserves defined execution and changes no numerical equations.
    gcs_source = source/'src/Mod/Sketcher/App/planegcs/GCS.cpp'
    original = gcs_source.read_text()
    if original.count('std::unreachable();') != 1:
        raise RuntimeError('Pinned portability patch no longer applies exactly once')
    gcs_source.write_text('#include <cstdlib>\n'+original.replace('std::unreachable();', 'std::abort();'))
    # Explicit standalone logging/config/export boundary.
    (shims/'FCGlobal.h').write_text('#pragma once\n#define FREECAD_DECL_EXPORT\n#define FREECAD_DECL_IMPORT\n')
    (shims/'FCConfig.h').write_text('#pragma once\n')
    (shims/'Base/Tools.h').write_text('#pragma once\n#include <chrono>\nnamespace Base { struct TimeElapsed { std::chrono::steady_clock::time_point value=std::chrono::steady_clock::now(); static double diffTimeF(const TimeElapsed& a,const TimeElapsed& b){return std::chrono::duration<double>(b.value-a.value).count();} }; }\n')
    (shims/'Base/Console.h').write_text('#pragma once\nnamespace Base { struct Sink { template<class... T> void log(const char*,T&&...) {} template<class... T> void warning(const char*,T&&...) {} }; inline Sink& Console(){static Sink sink;return sink;} }\n')
    gcs=source/'src/Mod/Sketcher/App/planegcs'
    includes=[f'-I{gcs}',f'-I{source/"src"}',f'-I{shims}',f'-I{args.prefix}/include/eigen3',f'-I{args.prefix}/include']
    library=out/('liballoy_planegcs_spike.dylib' if platform.system()=='Darwin' else 'liballoy_planegcs_spike.so')
    fixture=pathlib.Path(__file__).resolve().with_name('rectangle.cpp'); binary=out/'rectangle'
    commands=[['c++','-std=c++20','-DEIGEN_MPL2_ONLY','-O1','-shared','-fPIC',*includes,*map(str,sorted(gcs.glob('*.cpp'))),'-o',str(library)],
              ['c++','-std=c++20','-DEIGEN_MPL2_ONLY','-O1',*includes,str(fixture),str(library),'-o',str(binary)]]
    with (out/'build.log').open('w') as log:
        for command in commands:
            log.write(json.dumps(command)+'\n');log.flush()
            subprocess.run(command,stdout=log,stderr=log,check=True)
    result=subprocess.run([str(binary)],capture_output=True,text=True)
    (out/'result.json').write_text(result.stdout);(out/'stderr.log').write_text(result.stderr)
    receipt={'schema':'alloy-solver-spike-build/1','revision':REVISION,'host':platform.platform(),
             'compiler':subprocess.check_output(['c++','--version'],text=True),'commands':commands,
             'upstream':upstream,'patch':{'file':'GCS.cpp','change':'C++23 std::unreachable to C++20 std::abort; added cstdlib','compiledSha256':sha(gcs_source)},
             'shims':{str(p.relative_to(shims)):sha(p) for p in shims.rglob('*') if p.is_file()},
             'headerDependencies':{'Eigen':{'version':'5.0.1','versionHeaderSha256':sha(eigen_version),'mode':'EIGEN_MPL2_ONLY'},
                'Boost':{'version':'1.90.0','versionHeaderSha256':sha(boost_version)}},
             'fixtureSha256':sha(fixture),'runnerSha256':sha(pathlib.Path(__file__)),
             'binarySha256':sha(binary),'librarySha256':sha(library),'resultSha256':sha(out/'result.json'),
             'returncode':result.returncode}
    (out/'build.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(result.stdout);print(result.stderr)
    raise SystemExit(result.returncode)

if __name__=='__main__': main()
