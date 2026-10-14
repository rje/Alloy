#!/usr/bin/env python3
"""Bounded real-entrypoint user-local canvas, calibration and persistence check."""
import argparse
import json
from pathlib import Path
from PIL import Image, ImageDraw
from app_harness import application

parser=argparse.ArgumentParser()
parser.add_argument('--app',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args()
out=Path(args.output).resolve();out.mkdir(parents=True,exist_ok=True)
source=out/'independent-canvas.png'
image=Image.new('RGB',(400,200),'white');draw=ImageDraw.Draw(image)
draw.rectangle((0,0,399,199),outline='black',width=4)
draw.rectangle((0,0,55,55),fill='red');draw.rectangle((345,0,399,55),fill='blue')
draw.line((50,100,350,100),fill='black',width=4)
for x in (50,350):draw.ellipse((x-8,92,x+8,108),fill='green')
draw.text((80,150),'ALLOY LOCAL REFERENCE',fill='black');image.save(source)
with application(args.app,out) as (call,ledger):
    call('window.resize',{'width':1400,'height':1000})
    call('canvas.set',{'path':str(source),'planeSource':{'plane':'XY'},'name':'Independent reference','widthMm':160,'opacity':.8})
    original=call('document.inspect')['authored'];row=call('canvas.inspect')['canvases'][0];identity=row['id']
    assert row['widthMm']==160 and row['heightMm']==80
    call('view.home');call('view.orientation',{'name':'top'})
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    assert call('canvas.inspect')['gpu'][0]['textureDisplayed']
    call('capture.screenshot',{'path':str(out/'01-local-canvas.png')})
    session=call('canvas.dialog',{'canvasId':identity})
    call('command.update',{'session':session['id'],'inputs':{'centerUMm':12,'angleDegrees':25,'opacity':.4}})
    assert call('document.inspect')['authored']==original
    call('capture.screenshot',{'path':str(out/'02-placement-preview.png')})
    call('command.cancel',{'session':session['id']})
    assert call('document.inspect')['authored']==original
    calibrated=call('canvas.calibrate',{'canvasId':identity,'firstPixel':[50,100],'secondPixel':[350,100],'distanceMm':120})
    assert calibrated['widthMm']==160
    before=call('document.inspect')['authored']
    call('canvas.calibrate',{'canvasId':identity,'firstPixel':[50,100],'secondPixel':[350,100],'distanceMm':90})
    assert call('canvas.inspect')['canvases'][0]['widthMm']==120
    call('history.undo');assert call('document.inspect')['authored']==before
    call('history.redo');edited=call('document.inspect')['authored']
    call('canvas.calibrate',{'canvasId':identity,'firstPixel':[50,100],'secondPixel':[50,100],'distanceMm':90},error='invalid_canvas')
    assert call('document.inspect')['authored']==edited
    call('canvas.set',{'canvasId':identity,'visible':False});assert not call('canvas.inspect')['gpu'][0].get('textureDisplayed',False)
    call('history.undo');assert call('document.inspect')['authored']==edited
    call('document.save',{'path':str(out/'local-reference.alloy')})
    source.rename(out/'moved-independent-canvas.png')
    call('document.open',{'path':str(out/'local-reference.alloy'),'discard':True})
    assert call('document.inspect')['authored']==edited
    assert call('canvas.inspect')['gpu'][0]['textureDisplayed']
    call('view.home');call('view.orientation',{'name':'top'})
    call('capture.screenshot',{'path':str(out/'03-portable-reopen.png')})
    assert call('document.inspect')['evaluation']['bodies']==[]
    # Author a real sketch line over the calibrated independent reference marks.
    first=call('canvas.projectPixel',{'canvasId':identity,'pixelX':50,'pixelY':100})
    second=call('canvas.projectPixel',{'canvasId':identity,'pixelX':350,'pixelY':100})
    call('sketch.begin',{'plane':'XY','name':'Independent traced guide','units':'mm'})
    call('sketch.line',{'x1':first['uMm'],'y1':first['vMm'],'x2':second['uMm'],'y2':second['vMm'],'units':'mm'})
    call('sketch.finish')
    model=call('document.inspect')
    assert len(model['authored']['features'][0]['parameters']['entities'])==1
    assert call('app.state')['viewport']['gpu']['displayedSketches']==1
    call('capture.screenshot',{'path':str(out/'04-real-sketch-reference.png')})
    call('document.save',{'path':str(out/'local-reference-traced.alloy')})
    (out/'commands.json').write_text(json.dumps(ledger,indent=2)+'\n')
    (out/'summary.json').write_text(json.dumps({'calls':len(ledger),'canvasId':identity,'geometryFeatures':len(model['authored']['features']),'alloyDocument':str(out/'local-reference-traced.alloy')},indent=2)+'\n')
print('reference_canvas_api pass')
