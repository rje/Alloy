#!/usr/bin/env python3
"""Actual paired paper-clip fillets with constrained seed; not typed-line tutorial evidence."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app',required=True)
    parser.add_argument('--output',required=True)
    args=parser.parse_args();output=Path(args.output).resolve();captures=[]
    vertices=[(0,0),(0,40),(14,40),(14,-20),(3,-20),(3,24),(11,24),(11,0)]
    with application(args.app,output) as (call,transcript):
        session=call('sketch.begin',{'plane':'XY','name':'Paired returns'})
        def draft():return call('command.preview',{'session':session['id']})['authoredDraft']
        for a,b in zip(vertices,vertices[1:]):call('sketch.line',{'x1':a[0],'y1':a[1],'x2':b[0],'y2':b[1],'units':'mm'})
        edges=draft()['parameters']['entities'];ids=[e['id'] for e in edges];widths=[]
        call('sketch.constraint',{'type':'fixed','pointIds':[edges[0]['startPointId']]})
        for i,(a,b) in enumerate(zip(vertices,vertices[1:])):
            kind='horizontal' if i%2 else 'vertical'
            call('sketch.constraint',{'type':kind,'entityIds':[ids[i]]})
            call('sketch.dimension',{'kind':kind,'entityId':ids[i],'value':math.dist(a,b),'units':'mm'})
            if i%2:widths.append(draft()['parameters']['dimensions'][-1]['id'])
        # Fit source chain and retain room for the upper dimension witnesses.
        call('view.zoom',{'factor':1.2})
        p=call('view.projectSketchPoint',{'plane':'XY','uMm':7,'vMm':10});viewport=call('app.state')['viewport']
        call('view.pan',{'x':viewport['width']/2-p['x'],'y':viewport['height']/2-p['y']})
        def choose(pair):
            call('sketch.tool',{'tool':'fillet'})
            for u,v in vertices[pair*2+1:pair*2+3]:
                p=call('view.projectSketchPoint',{'plane':'XY','uMm':u,'vMm':v})
                call('sketch.placePoint',{'x':p['x'],'y':p['y']})
        for pair,radius in enumerate([7,5.5,4]):
            before=draft();choose(pair);history=call('app.state')['activeSession']['draftHistory']
            call('sketch.filletPreview',{'value':radius+0.1,'units':'mm'},error='sketch_fillet_failed')
            assert draft()==before
            preview=call('sketch.filletPreview',{'value':radius,'units':'mm'})
            assert preview['diagnostics']['convertedReferenceDimensionIds']==[widths[pair]]
            assert draft()==before and call('app.state')['activeSession']['draftHistory']==history
            captures.append(call('capture.screenshot',{'path':str(output/f'pair-{pair+1}-preview.png')}))
            call('sketch.filletCancel');assert draft()==before
            choose(pair);result=call('sketch.filletAccept',{'value':radius,'units':'mm'})
            assert result['convertedReferenceDimensionIds']==[widths[pair]]
            accepted=draft();call('history.undo');assert draft()==before
            call('history.redo');assert draft()==accepted
        original=draft();params=original['parameters'];arcs=[e for e in params['entities'] if e['type']=='arc']
        assert len(arcs)==6 and {e['id'] for e in params['entities'] if e['type']=='line'}=={ids[i] for i in [0,2,4,6]}
        points={p['id']:(p['uMm'],p['vMm']) for p in params['points']}
        length=sum(math.dist(points[e['startPointId']],points[e['endPointId']]) if e['type']=='line' else e['radiusMm']*abs(e['sweepAngleDeg'])*math.pi/180 for e in params['entities'])
        assert math.isclose(length,135+16.5*math.pi,abs_tol=1e-6)
        for width in widths:
            d=next(d for d in params['dimensions'] if d['id']==width);assert not d['driving'] and d['kind']=='horizontal'
        captures.append(call('capture.screenshot',{'path':str(output/'all-returns.png')}))
        radius=next(d for d in params['dimensions'] if d['kind']=='radius')
        call('sketch.dimensionUpdate',{'dimensionId':radius['id'],'value':6.5,'units':'mm'})
        edited=draft();assert [e['id'] for e in edited['parameters']['entities']]==[e['id'] for e in params['entities']]
        call('history.undo');assert draft()==original
        call('sketch.finish');authored=call('document.inspect')['authored'];path=output/'paired-returns.alloy'
        call('document.save',{'path':str(path)});call('document.new',{'discard':False});call('document.open',{'path':str(path)})
        assert call('document.inspect')['authored']==authored
    report={'result':'passed','scope':'API setup with actual lengths/HV; shared corner picks, preview/Cancel/accept/history/edit/storage; native typed radius/Enter separately qualified. No full tutorial claim.', 'pathLengthMm':length,'captures':captures,'authored':authored,'transcript':transcript}
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS: constrained shared-line fillets, spans, refusal, edit/history and exact reopen')


if __name__=='__main__':main()
