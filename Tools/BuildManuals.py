"""Build two embedded-font PDFs. pip install -r Manuals/requirements.txt.
RTL paragraphs are shaped and bidi-reordered per wrapped line, not reversed as raw text.
"""
import json
from pathlib import Path
import arabic_reshaper
from bidi.algorithm import get_display
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor
ROOT=Path(__file__).resolve().parents[1]
FONT=Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')
pdfmetrics.registerFont(TTFont('Manual',str(FONT)))
data=json.loads((ROOT/'Manuals/content.json').read_text())
W,H=595.28,841.89
for lang,pages in data.items():
    rtl=lang=='fa'; size=10.8 if rtl else 10.5; leading=18 if rtl else 16
    path=ROOT/'Manuals'/('Xiffects-Live-User-Manual-'+('FA' if rtl else 'EN')+'.pdf')
    c=canvas.Canvas(str(path),pagesize=(W,H),pageCompression=1)
    c.setTitle('Xiffects Live — '+('راهنمای فارسی' if rtl else 'English User Manual'))
    c.setAuthor('Xivan / Daniyal Azimi');c.setSubject('User manual — interface revision 90b6ecf')
    def visual(text):return get_display(arabic_reshaper.reshape(text), base_dir='R' if any('\u0600'<=ch<='\u06ff' for ch in text) else 'L') if rtl else text
    def line(text,y,fs,color):
        c.setFont('Manual',fs);c.setFillColor(HexColor(color))
        if rtl:c.drawRightString(W-48,y,visual(text))
        else:c.drawString(48,y,text)
    def block(text,y,fs,colour,step):
        for paragraph in text.split('\n'):
            current=''
            for word in paragraph.split():
                candidate=(current+' '+word).strip()
                if pdfmetrics.stringWidth(visual(candidate),'Manual',fs)>W-96 and current:
                    line(current,y,fs,colour);y-=step;current=word
                else:current=candidate
            if current:line(current,y,fs,colour);y-=step
        return y
    for page,item in enumerate(pages,1):
        c.setFillColor(HexColor('#101116'));c.rect(0,H-104,W,104,fill=1,stroke=0)
        c.setFillColor(HexColor('#df2549'));c.rect(0,H-107,W,3,fill=1,stroke=0)
        c.setFont('Manual',10);c.setFillColor(HexColor('#c0c2c8'));c.drawString(48,H-30,'XIFFECTS LIVE  /  USER GUIDE')
        line(item['title'],H-70,24,'#ffffff');y=H-137
        if page==1:
            c.drawImage(str(ROOT/'Assets/hero.jpg'),48,y-105,width=W-96,height=105);y-=130
        for title,text in item['sections']:
            y=block(title,y,13,'#b61c3a',19)-3
            y=block(text,y,size,'#282b32',leading)-13
        if y<56:raise RuntimeError(f'Page overflow {lang} {page}: {y}')
        c.setStrokeColor(HexColor('#d8dae0'));c.line(48,43,W-48,43)
        c.setFont('Manual',8);c.setFillColor(HexColor('#666a74'));c.drawString(48,28,'Xiffects Live | 25 Oct 2026 | '+lang.upper());c.drawRightString(W-48,28,f'{page} / {len(pages)}')
        c.showPage()
    c.save();print(path)
