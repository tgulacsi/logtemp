#!/usr/bin/env python
import os
import subprocess
import logging
LOG = logging.getLogger(__name__)

DIR = '/var/local/lib/logtemp'
FN = DIR + '/temperature.rrd'
PNG = '/var/www/logtemp/temp1.png'
MAP = {'10B6EAED18020': 'vissza', '104EA7ED180F6': 'elore'}
ORDER = ('elore', 'vissza')
COLORS = {'elore': 'ff0000', 'vissza': '0000ff'}


def call(cmd, *args):
    if len(args) == 1 and isinstance(args, (tuple, list)):
        args = args[0]
    LOG.debug('calling %s %s', cmd, ' '.join(args))
    return subprocess.call(['rrdtool', cmd] + list(args))


def init():
    if not os.path.exists(DIR):
        os.makedirs(DIR)
    if not os.path.exists(FN):
        call('create', FN, '-s', '60',
             *(['DS:%s:GAUGE:120:-10:60' % k for k in ORDER]
               + ['RRA:AVERAGE:0.5:1:10080', 'RRA:AVERAGE:0.5:5:52560', 
                  'RRA:MIN:0.5:5:52560', 'RRA:MAX:0.5:5:52560']))


def get():
    import httplib
    conn = httplib.HTTPConnection('gthomas.gotdns.org', 8081)
    conn.request('GET', '/index.html')
    resp = conn.getresponse()
    if not 200 == resp.status:
        raise ValueError, resp.status + ' ' + resp.reason
    return resp.read()


def main():
    init()
    text = get()
    LOG.debug('text=%s', text)
    rows = (line.strip() for line in text.splitlines() if line.strip())
    elts = (line.split('=', 1) for line in rows if '=' in line)
    data = dict((MAP[tup[0]], tup[1]) for tup in elts)
    assert data, text
    assert set(data) & set(ORDER) == set(ORDER), (data, text)

    LOG.info('data=%s', data)

    update(data)
    graph(PNG)


def update(data):
    call('update', FN, 'N:' + ':'.join(data[k] for k in ORDER))


def graph(fn):
    call('graph', 
         [fn] + '-D --width 800 --height 600 -z'.split()
         + ['DEF:%s=%s:%s:AVERAGE' % (k, FN, k) for k in ORDER]
         + ['LINE1:%s#%s:%s' % (k, COLORS[k], k) for k in ORDER])


if '__main__' == __name__:
    import sys
    logging.basicConfig(level=logging.DEBUG if '-v' in sys.argv[1:] else logging.WARN, 
        stream=sys.stdout)
    main()
