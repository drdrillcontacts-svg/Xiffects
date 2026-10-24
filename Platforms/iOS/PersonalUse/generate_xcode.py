#!/usr/bin/env python3
"""Generate an iOS-only build in a fresh COPY. Never patch the original project."""
import argparse, hashlib, json, platform, re, shlex, shutil, subprocess
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bundle-id',required=True,help='Unique reverse-DNS container ID you can sign')
    p.add_argument('--team',help='Optional public 10-character Apple Team ID; not a certificate/key')
    p.add_argument('--output',required=True,type=Path,help='NEW working directory (must not already exist)')
    p.add_argument('--stage-only',action='store_true',help='Copy/validate only; do not generate or build Xcode')
    a=p.parse_args()
    if not re.fullmatch(r'[A-Za-z][A-Za-z0-9-]*(?:\.[A-Za-z][A-Za-z0-9-]*)+',a.bundle_id):p.error('Invalid reverse-DNS bundle ID')
    if a.team and not re.fullmatch(r'[A-Z0-9]{10}',a.team):p.error('Team ID must have 10 uppercase letters/digits')
    if not a.stage_only and platform.system()!='Darwin':p.error('Xcode generation requires macOS; --stage-only is not a build')
    root=Path(__file__).resolve().parents[3]
    out=a.output.expanduser().resolve()
    if out.exists():p.error('Refusing to overwrite an existing directory; choose a fresh --output')
    cmake=(root/'CMakeLists.txt').read_text()
    marker='BUNDLE_ID "com.xivan.xiffects.live"'
    if cmake.count(marker)!=1:p.error('Unexpected source CMake configuration; refusing to patch')
    source=out/'source';source.mkdir(parents=True)
    for name in ['Source','Assets','Tests','Tools']:
        shutil.copytree(root/name,source/name,ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
    # Only the isolated iOS build copy gets a personally signable bundle identity.
    (source/'CMakeLists.txt').write_text(cmake.replace(marker,'BUNDLE_ID "'+a.bundle_id+'"'))
    command=['cmake','-S',str(source),'-B',str(out/'build'),'-G','Xcode',
        '-DCMAKE_SYSTEM_NAME=iOS','-DCMAKE_OSX_SYSROOT=iphoneos',
        '-DCMAKE_OSX_ARCHITECTURES=arm64','-DCMAKE_OSX_DEPLOYMENT_TARGET=15.0',
        '-DCMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY=1,2',
        '-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE=Automatic',
        '-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=YES',
        '-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY=Apple Development',
        '-DBUILD_TESTING=OFF','-DXIFFECTS_BUILD_LIVE=ON']
    if a.team:command.append('-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM='+a.team)
    info={'source_cmake_sha256':hashlib.sha256((root/'CMakeLists.txt').read_bytes()).hexdigest(),
          'bundle_id':a.bundle_id,'command':command,'files':{str(f.relative_to(source)):hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(source.rglob('*')) if f.is_file()}}
    (out/'generation.json').write_text(json.dumps(info,indent=2)+'\n')
    print(shlex.join(command))
    if a.stage_only:print('STAGED ONLY: no Xcode generation, compilation, signing or device test performed.');return
    subprocess.run(command,check=True)
    print('Open:',out/'build/Xiffects.xcodeproj')
    print('Choose XiffectsLive_Standalone, select a physical device, and configure the SAME Team for the standalone and AUv3 targets.')
    print('Generation is not compilation, signing verification, installation or GarageBand validation.')
if __name__=='__main__':main()
