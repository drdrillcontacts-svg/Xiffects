"""Generate complete deterministic factory snapshots; no runtime randomisation."""
import json,re
from pathlib import Path
s=Path('Source/PluginProcessor.cpp').read_text()
defaults={m[0]:float(m[1].rstrip('f')) for m in re.findall(r'addFloat\("([A-Z_]+)",\s*"[^"]+",\s*[^,]+,\s*[^,]+,\s*([\d.\-]+f?)',s)}
for module in ['OCT','COMP','REV','DLY','DIST','EQ','PITCH','PHASER','TUNE']: defaults[module+'_ON']=0
# A complete musical snapshot, including every manual permission.
defaults.update(POWER=1,OUTPUT=-3,TUNE_KEY=0,TUNE_SCALE=0,TUNE_SCALE_EXT=0)
for n in range(12):defaults['TUNE_NOTE_'+str(n)]=1
families=[
 ('Sax intimate',['Dry Studio','Velvet Room','Close Ballad','Dark Ribbon','Breath Detail']),
 ('Sax lead',['Crimson Lead','Stage Focus','Bright Solo','Warm Projection','Edge Solo']),
 ('Voice natural',['Speech Clean','Soft Alto','Airy Tenor','Warm Baritone','Intimate Vocal']),
 ('Voice tuned',['Gentle Pop','Tight Pop','Fast Chromatic','Soft Minor','Major Lift']),
 ('Space',['Small Chamber','Wooden Room','Concert Bloom','Long Halo','Distant Hall']),
 ('Echo',['Short Slap','Double Slap','Eighth Echo','Quarter Echo','Long Answer']),
 ('Octave',['Subtle Sub','Deep Foundation','Upper Spark','Low Duo','Two Octaves Down']),
 ('Harmony',['Fifth Above','Fourth Above','Third Colour','Fifth Below','Octave Above']),
 ('Motion',['Slow Orbit','Gentle Sweep','Liquid Reed','Wide Motion','Fast Pulse']),
 ('Texture',['Soft Tape','Warm Grit','Crimson Crunch','Dark Drive','Parallel Edge'])]
recipes=[]
for family,names in families:
 for i,name in enumerate(names):
  p=defaults.copy();p.update(COMP_ON=1,COMP_THRESHOLD=-18-i,COMP_RATIO=2+i*.3,COMP_MAKEUP=1,EQ_ON=1,EQ_LOW=0,EQ_MID=0,EQ_HIGH=0,EQ_AIR=0)
  if family=='Sax intimate':p.update(EQ_LOW=1+i*.2,EQ_HIGH=-1-i*.5,REV_ON=int(i>0),REV_MIX=6+i*2,REV_SIZE=20+i*4,REV_DECAY=.4+i*.15)
  elif family=='Sax lead':p.update(EQ_MID=1+i*.5,EQ_HIGH=2,EQ_AIR=i*.5,DLY_ON=1,DLY_TIME=.18+i*.04,DLY_MIX=8+i*2,DLY_FEEDBACK=15+i*3)
  elif family=='Voice natural':p.update(COMP_THRESHOLD=-22-i,COMP_RATIO=2.5,EQ_LOW=-2+i*.5,EQ_HIGH=i*.4,REV_ON=int(i>0),REV_MIX=5+i*2,REV_DECAY=.5+i*.2)
  elif family=='Voice tuned':p.update(TUNE_ON=1,TUNE_STRENGTH=[35,70,100,50,65][i],TUNE_SPEED=[45,20,1,35,25][i],TUNE_MIX=100,TUNE_SCALE=[0,0,0,2,1][i],REV_ON=1,REV_MIX=8+i,REV_DECAY=1)
  elif family=='Space':p.update(REV_ON=1,REV_MIX=[10,15,23,32,40][i],REV_SIZE=20+i*18,REV_DECAY=[.5,.9,2.2,4,6][i],EQ_HIGH=-i*.5)
  elif family=='Echo':p.update(DLY_ON=1,DLY_TIME=[.08,.12,.25,.5,.75][i],DLY_MIX=12+i*4,DLY_FEEDBACK=10+i*10)
  elif family=='Octave':p.update(OCT_ON=1,OCT_SHIFT=[-1,-1,1,-1,-2][i],OCT_MIX=[12,30,18,45,25][i],EQ_LOW=-i*.5)
  elif family=='Harmony':p.update(PITCH_ON=1,PITCH_SEMITONES=[7,5,4,-7,12][i],PITCH_MIX=20+i*4,REV_ON=1,REV_MIX=10,REV_DECAY=1.2)
  elif family=='Motion':p.update(PHASER_ON=1,PHASER_RATE=[.08,.2,.45,.9,2][i],PHASER_DEPTH=20+i*12,PHASER_FEEDBACK=10+i*8)
  elif family=='Texture':p.update(DIST_ON=1,DIST_DRIVE=10+i*15,DIST_TONE=65-i*10,DIST_MIX=12+i*8,EQ_HIGH=-i*.5,OUTPUT=-4-i)
  recipes.append(dict(name=name,category=family,parameters=p))
assert len(recipes)==50
Path('Assets/factory_presets.json').write_text(json.dumps({'schema':1,'presets':recipes},indent=2)+'\n')
