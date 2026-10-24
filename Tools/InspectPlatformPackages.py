"""Inspect produced files, not a substitute for launching on physical devices."""
from pathlib import Path
import hashlib,json,os,plistlib,struct,subprocess,xml.etree.ElementTree as ET,zipfile
result={'device_audio_tested':False,'files':[],'tests':[]}
for test in Path('desktop').rglob('tests.xml'):
    cases=[]
    for node in ET.parse(test).getroot().iter('testcase'):
        assert node.find('failure') is None and node.find('error') is None
        cases.append(node.attrib.get('name'))
    assert len(cases)==9
    result['tests'].append({'path':str(test),'passed':cases})
assert len(result['tests'])==2
for binary in Path('desktop').rglob('*.vst3'):
    if not binary.is_file():continue
    data=binary.read_bytes()
    assert data[:2]==b'MZ'
    pe=struct.unpack_from('<I',data,0x3c)[0]
    assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==0x8664
    result['files'].append({'path':str(binary),'architecture':'Windows x64','sha256':hashlib.sha256(data).hexdigest()})
assert any(f.get('architecture')=='Windows x64' for f in result['files'])
mac=next(Path('desktop').rglob('*macOS-universal-unsigned.zip'))
with zipfile.ZipFile(mac) as z:
    binaries=[n for n in z.namelist() if '/Contents/MacOS/' in n and not n.endswith('/') and not n.startswith('__MACOSX/')]
    assert len(binaries)>=3
    for name in binaries:
        data=z.read(name);magic,count=struct.unpack_from('>II',data)
        assert magic in [0xcafebabe,0xcafebabf]
        stride=20 if magic==0xcafebabe else 32
        cpus=[struct.unpack_from('>I',data,8+i*stride)[0] for i in range(count)]
        assert 0x1000007 in cpus and 0x100000c in cpus
        result['files'].append({'path':name,'architecture':'macOS arm64+x86_64','sha256':hashlib.sha256(data).hexdigest()})
apk=next(Path('android').rglob('*.apk'))
with zipfile.ZipFile(apk) as z:
    libs=[n for n in z.namelist() if n.startswith('lib/arm64-v8a/') and n.endswith('.so')]
    assert libs
    result['android_native_libraries']=libs
sdk=Path(os.environ['ANDROID_HOME'])
aapt=sorted((sdk/'build-tools').glob('*/aapt'))[-1]
permissions=subprocess.check_output([str(aapt),'dump','permissions',str(apk)],text=True)
assert 'android.permission.RECORD_AUDIO' in permissions
result['android_permissions']=permissions
result['android_badging']=subprocess.check_output([str(aapt),'dump','badging',str(apk)],text=True)
result['files'].append({'path':str(apk),'sha256':hashlib.sha256(apk.read_bytes()).hexdigest(),'kind':'development APK, not device-tested'})
with zipfile.ZipFile(next(Path('ios').rglob('*.zip'))) as z:
    apps=[n for n in z.namelist() if n.endswith('.app/Info.plist') and not n.startswith('__MACOSX/')]
    extensions=[n for n in z.namelist() if n.endswith('.appex/Info.plist') and not n.startswith('__MACOSX/')]
    assert apps and extensions and any(".app/PlugIns/" in n for n in extensions)
    for name in extensions:
        extension=plistlib.loads(z.read(name))
        assert extension["NSExtension"]["NSExtensionPointIdentifier"] in ["com.apple.AudioUnit-UI","com.apple.AudioUnit"]
    info=plistlib.loads(z.read(apps[0]))
    assert info.get('NSMicrophoneUsageDescription') and set(info['UIDeviceFamily'])=={1,2}
    result['ios']={'device_families':info['UIDeviceFamily'],'microphone_description':info['NSMicrophoneUsageDescription'],'extensions':extensions,'signed_for_installation':False}
Path('platform-inspection.json').write_text(json.dumps(result,indent=2))
print('::notice title=PackageInspection::'+json.dumps({'tests':result['tests'],'files':result['files'],'android_native_libraries':result['android_native_libraries'],'ios':result['ios']},separators=(',',':')))
