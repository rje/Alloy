"""Transport metadata/selector checks only; no substitute CAD state."""
import json,sys,tempfile
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from alloyctl import record_endpoint,choose_endpoint,exchange
address,transport=record_endpoint({'protocol':'alloy-control/1','endpoint':'/private/socket','transport':'unix-socket','helper':'/untrusted/program'})
assert address.transport==transport=='unix-socket'
with tempfile.TemporaryDirectory() as directory:
 path=Path(directory)/'ready.json';path.write_text(json.dumps({'endpoint':'/private/socket','transport':'unix-socket'}))
 with patch('alloyctl.verify_instance'):
  selected=choose_endpoint(ready_file=path)
 assert selected=='/private/socket' and selected.transport=='unix-socket'
for record in ({'endpoint':'/private/socket','transport':'remote'}, {'endpoint':[],'transport':'unix-socket'}):
 try:record_endpoint(record)
 except RuntimeError:pass
 else:raise AssertionError('Bad transport metadata accepted')
try:exchange(r'\\remote\pipe\bad',b'{}\n',transport='windows-named-pipe')
except (ValueError,RuntimeError):pass
else:raise AssertionError('Remote pipe accepted')
print('PASS: explicit transport survives selection; untrusted helper metadata ignored; invalid/remote records refused')
