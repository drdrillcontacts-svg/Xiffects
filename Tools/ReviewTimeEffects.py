"""Review saved native test logs and preview images; no new audio is rendered here."""
import base64,io,json
from pathlib import Path
import xml.etree.ElementTree as ET
from PIL import Image
root=Path('downloaded');reports=[]
for xml in root.rglob('tests.xml'):
    cases=list(ET.parse(xml).getroot().iter('testcase'))
    assert len(cases)==11 and all(c.find('failure') is None and c.find('error') is None for c in cases)
    reports.append({'path':str(xml),'passed':[c.attrib['name'] for c in cases]})
assert len(reports)==2
rows=[]
for log in root.rglob('LastTest.log'):
    rows += [{'log':str(log),'result':line} for line in log.read_text(errors='replace').splitlines() if line.startswith('PRODUCT_PITCH ')]
assert len(rows)==48
result={'tests':reports,'pitch':rows,'method':'Saved actual processor test logs; synthetic tones only'}
Path('review.json').write_text(json.dumps(result,indent=2))
print('::notice title=NativeTests::'+json.dumps(reports,separators=(',',':')))
for i in range(0,len(rows),12):print('::notice title=ProductPitch'+str(i//12)+'::'+json.dumps(rows[i:i+12],separators=(',',':')))
image=Image.open(next((root/'Tests-windows-latest').rglob('Xiffects-Live-compact.png'))).convert('RGB')
for quality in [80,70,60,50,40,30]:
    b=io.BytesIO();image.save(b,format='JPEG',quality=quality,optimize=True)
    if len(b.getvalue())<48000:break
assert len(b.getvalue())<48000
Path('compact-preview.jpg').write_bytes(b.getvalue())
Path('preview.base64').write_text(base64.b64encode(b.getvalue()).decode())
