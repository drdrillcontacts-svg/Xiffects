#!/usr/bin/env python3
"""Inspect extracted .app/.appex bundles. On macOS also inspect signatures/profiles.
No installation or GarageBand success is inferred from these static checks.
"""
import argparse,datetime,hashlib,json,platform,plistlib,struct,subprocess
from pathlib import Path

PLATFORMS={1:'macOS',2:'iOS device',6:'Mac Catalyst',7:'iOS Simulator'}
def macho(data):
    magic=struct.unpack_from('>I',data)[0]
    if magic in (0xcafebabe,0xcafebabf):
        count=struct.unpack_from('>I',data,4)[0]
        if count>32:raise ValueError('Invalid fat slice count')
        out=[];stride=20 if magic==0xcafebabe else 32
        for i in range(count):
            pos=8+i*stride
            offset,size=struct.unpack_from('>II' if stride==20 else '>QQ',data,pos+8)
            out.extend(macho(data[offset:offset+size]))
        return out
    formats={b'\xcf\xfa\xed\xfe':('<',32),b'\xfe\xed\xfa\xcf':('>',32),b'\xce\xfa\xed\xfe':('<',28),b'\xfe\xed\xfa\xce':('>',28)}
    endian,header=formats[data[:4]]
    cpu=struct.unpack_from(endian+'I',data,4)[0]
    ncmds=struct.unpack_from(endian+'I',data,16)[0]
    if ncmds>10000:raise ValueError('Invalid load command count')
    result={'cpu_type':hex(cpu),'architecture':{0x100000c:'arm64',0x1000007:'x86_64'}.get(cpu,'other'),'platform':'not identified'}
    offset=header
    for _ in range(ncmds):
        cmd,size=struct.unpack_from(endian+'II',data,offset)
        if size<8 or offset+size>len(data):raise ValueError('Invalid load command')
        if cmd==0x32:
            code,minimum,sdk=struct.unpack_from(endian+'III',data,offset+8)
            version=lambda n:f'{n>>16}.{(n>>8)&255}.{n&255}'
            result.update(platform=PLATFORMS.get(code,str(code)),minimum_os=version(minimum),sdk=version(sdk))
        elif cmd==0x25:result['platform']='iOS (legacy version command; distinguish simulator using CPU/SDK)'
        elif cmd==0x24:result['platform']='macOS'
        offset+=size
    return [result]

def run(args):
    p=subprocess.run(args,capture_output=True)
    return p.returncode,p.stdout,p.stderr

def inspect(bundle):
    info_file=bundle/'Info.plist'
    if not info_file.exists():info_file=bundle/'Contents/Info.plist'
    info=plistlib.loads(info_file.read_bytes())
    executable=info_file.parent/info['CFBundleExecutable']
    if not executable.exists():executable=info_file.parent/'MacOS'/info['CFBundleExecutable']
    data=executable.read_bytes()
    report={'path':str(bundle),'bundle_id':info.get('CFBundleIdentifier'),
      'executable_sha256':hashlib.sha256(data).hexdigest(),'mach_o':macho(data),
      'supported_platforms':info.get('CFBundleSupportedPlatforms'),
      'minimum_os':info.get('MinimumOSVersion'),'device_families':info.get('UIDeviceFamily'),
      'microphone_usage_description':info.get('NSMicrophoneUsageDescription'),
      'extension':info.get('NSExtension'),'profile_present':(bundle/'embedded.mobileprovision').exists(),
      'signature_check':'NOT RUN (requires macOS codesign)'}
    if platform.system()=='Darwin':
        code,stdout,stderr=run(['codesign','--verify','--strict','--verbose=2',str(bundle)])
        report['signature_check']={'exit_code':code,'output':(stdout+stderr).decode(errors='replace')}
        code,stdout,stderr=run(['codesign','-d','--verbose=4',str(bundle)])
        report['signature_details']={'exit_code':code,'output':(stdout+stderr).decode(errors='replace')}
        code,stdout,stderr=run(['codesign','-d','--entitlements',':-',str(bundle)])
        report['signed_entitlements']={'exit_code':code,'output':(stdout+stderr).decode(errors='replace')}
        if report['profile_present']:
            code,stdout,stderr=run(['security','cms','-D','-i',str(bundle/'embedded.mobileprovision')])
            if code==0:
                profile=plistlib.loads(stdout)
                # Never include certificate blobs, device UDIDs, private keys or credentials.
                report['profile']={k:profile.get(k) for k in ['Name','ExpirationDate','TeamIdentifier']}
                report['profile']['registered_device_count']=len(profile.get('ProvisionedDevices',[]))
                report['profile']['entitlements']={k:profile.get('Entitlements',{}).get(k) for k in ['application-identifier','com.apple.developer.team-identifier','get-task-allow']}
            else:report['profile_decode_error']=stderr.decode(errors='replace')
    return report

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('root',type=Path);p.add_argument('--output',type=Path,default=Path('ios-bundle-inspection.json'));a=p.parse_args()
    root=a.root.resolve()
    bundles=sorted(set([x for x in root.rglob('*') if x.is_dir() and x.suffix in ['.app','.appex']]+([root] if root.suffix in ['.app','.appex'] else [])))
    result={'garageband_tested':False,'bundles':[]}
    for b in bundles:
        if '__MACOSX' in b.parts:continue
        try:result['bundles'].append(inspect(b))
        except Exception as e:result['bundles'].append({'path':str(b),'inspection_error':str(e)})
    if not bundles:raise SystemExit('No extracted .app/.appex found. This tool does not extract RAR archives.')
    a.output.write_text(json.dumps(result,indent=2,default=str)+'\n');print(a.output)
if __name__=='__main__':main()
