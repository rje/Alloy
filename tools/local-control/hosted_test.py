#!/usr/bin/env python3
"""Manual milestone evidence for Qt-only local transport; no desktop/GPU claim."""
import argparse,hashlib,json,platform,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--build',required=True);p.add_argument('--output',required=True);a=p.parse_args();build=Path(a.build).resolve();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
helper=build/('Release/alloy-local-client.exe' if platform.system()=='Windows' else 'alloy-local-client')
with (out/'ctest.log').open('w') as log:
 result=subprocess.run(['ctest','--test-dir',str(build),'-C','Release','--output-on-failure'],stdout=log,stderr=subprocess.STDOUT)
version=subprocess.check_output([str(helper),'--version'],text=True).strip()
report={'result':'non-GUI-transport-pass' if result.returncode==0 else 'failed','sourceCommit':subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip(),'platform':platform.platform(),'machine':platform.machine(),'python':platform.python_version(),'helperVersion':version,'helperSha256':hashlib.sha256(helper.read_bytes()).hexdigest(),'scope':'Native Qt local server/helper byte protocol, deadline/bounds/correlation and owned directory ACL tests only. No Alloy desktop app, cross-user denial, GPU or CAD evidence.'}
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');raise SystemExit(result.returncode)
