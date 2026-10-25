"""Review saved native processor tests, not a new listening or rendering test."""
import base64, io, json, hashlib
from pathlib import Path
import xml.etree.ElementTree as ET
from PIL import Image
root=Path('downloaded');reports=[];pitch=[];insert=[]
for xml in root.rglob('tests.xml'):
    cases=list(ET.parse(xml).getroot().iter('testcase'))
    assert len(cases)==12 and all(c.find('failure') is None and c.find('error') is None for c in cases)
    reports.append({'path':str(xml),'passed':[c.attrib['name'] for c in cases]})
assert len(reports)==2
for log in root.rglob('LastTest.log'):
    for line in log.read_text(errors='replace').splitlines():
        if line.startswith('PRODUCT_PITCH '):pitch.append({'log':str(log),'result':line})
        if line.startswith('INSERT_QUALITY '):insert.append({'log':str(log),'result':line})
assert len(pitch)==48 and len(insert)==24
binaries=[{'path':str(p),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in Path('windows-package').rglob('*.vst3') if p.is_file()]
assert binaries and all(p['bytes']<=50*1024*1024 for p in binaries)
result={'tests':reports,'pitch':pitch,'insert':insert,'binaries':binaries,'method':'Saved actual processor test logs; synthetic inputs, not sax recordings'}
Path('review.json').write_text(json.dumps(result,indent=2))
for key in ['tests','binaries']:print('::notice title='+key+'::'+json.dumps(result[key],separators=(',',':')))
for key in ['pitch','insert']:
    for i in range(0,len(result[key]),12):print('::notice title='+key+str(i//12)+'::'+json.dumps(result[key][i:i+12],separators=(',',':')))
image=Image.open(next((root/'Tests-windows-latest').rglob('Xiffects-Live-compact.png'))).convert('RGB')
for quality in [80,70,60,50,40,30]:
    b=io.BytesIO();image.save(b,format='JPEG',quality=quality,optimize=True)
    if len(b.getvalue())<48000:break
assert len(b.getvalue())<48000
Path('compact-preview.jpg').write_bytes(b.getvalue())
Path('preview.base64').write_text(base64.b64encode(b.getvalue()).decode())
