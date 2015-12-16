# -*- mode: python -*-
import sys,os
sourcedir=os.path.abspath('../')
#print sourcedir

def fdspath(p):
    return sourcedir + '/' + p

block_cipher = pyi_crypto.PyiBlockCipher(key='FD5BADA55BABE')

a = Analysis([fdspath('tools/fdsconsole/console.py')],
             pathex=[fdspath('tools/fdsconsole'),fdspath('test'),fdspath('test/fdslib'),fdspath('test/fdslib/pyfdsp')],
             binaries=None,
             datas=None,
             hiddenimports=[],
             hookspath=None,
             runtime_hooks=None,
             excludes=None,
             win_no_prefer_redirects=None,
             win_private_assemblies=None,
             cipher=block_cipher)
pyz = PYZ(a.pure, a.zipped_data,
             cipher=block_cipher)
a.binaries=[b for b in a.binaries if not b[0].startswith('libreadline')]
exe = EXE(pyz,
          a.scripts,
          a.binaries,
          a.zipfiles,
          a.datas,
          name='debugtool',
          debug=False,
          strip=None,
          upx=True,
          console=True )
