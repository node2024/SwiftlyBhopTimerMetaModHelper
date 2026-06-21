# vim: set sts=2 ts=8 sw=2 tw=99 et:
import sys

try:
  from ambuild2 import run
except:
  sys.stderr.write('AMBuild 2.2 or newer is required.\n')
  sys.exit(1)

ambuild_version = getattr(run, 'CURRENT_API', '2.1')
if ambuild_version.startswith('2.1'):
  sys.stderr.write('AMBuild 2.2 or newer is required.\n')
  sys.exit(1)

parser = run.BuildParser(sourcePath=sys.path[0], api='2.2')
parser.options.add_argument('-n', '--plugin-name', type=str, dest='plugin_name', default=None)
parser.options.add_argument('-a', '--plugin-alias', type=str, dest='plugin_alias', default=None)
parser.options.add_argument('--hl2sdk-root', type=str, dest='hl2sdk_root', default=None)
parser.options.add_argument('--hl2sdk-manifests', type=str, dest='hl2sdk_manifests', default='hl2sdk-manifests/')
parser.options.add_argument('--mms_path', type=str, dest='mms_path', default=None)
parser.options.add_argument('--enable-debug', action='store_const', const='1', dest='debug')
parser.options.add_argument('--enable-optimize', action='store_const', const='1', dest='opt')
parser.options.add_argument('-s', '--sdks', default='cs2', dest='sdks')
parser.options.add_argument('--targets', type=str, dest='targets', default='x86_64')
parser.Configure()

