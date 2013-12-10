from data import data
import collections

readers = collections.defaultdict(set)
writers = collections.defaultdict(set)

for t in data:
    if t['type'] == 'transition':
        if t['does_write']:
            writers[t['address']].add(t['thread'])
        else:
            readers[t['address']].add(t['thread'])

import re
address_pat = re.compile("(0x[0-9a-f]*)")

for t in data:
    if t['type'] == 'transition':
        t['relevant'] = (len(writers[t['address']]) > 1 or
            len(writers[t['address']]) == 1 and len(readers[t['address']] - writers[t['address']]) > 0)

        t['description'] = address_pat.sub(r'<span class="addr p\1">\1</span>', t['description'])

files = {}

for t in data:
    if t['type'] == 'transition':
        for l in t['trace']:
            if l['file'] not in files:
                files[l['file']] = open(l['file'], 'r').readlines()
            a = files[l['file']][l['line'] - 1]

            b = l['column'] - 1
            if b < len(a):
                c = a[:b] #+ "<i>"
                while b < len(a) and a[b] in "abcdefghijkmnlopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_1234567890:.->":
                    c += a[b]
                    b += 1
                #c += "</i>" + a[b:]
                c += a[b:]
                a = c

            l['contents'] = a

        t['trace'] = [l for l in t['trace'] if '::atomic' not in l['function']]
        t['trace'] = [l for l in t['trace'] if '__atomic_base' not in l['function']]
        t['trace'] = [l for l in t['trace'] if 'boost::lockfree::CAS' not in l['function']]
        t['trace'] = [l for l in t['trace'] if 'boost::lockfree::tagged_ptr' not in l['function']]

data = [t for t in data if t['type'] == 'annotation' or all('guards.protect' not in l['contents'] for l in t['trace'])]
data = [t for t in data if t['type'] == 'annotation' or all('AllocateHPRec' not in l['function'] for l in t['trace'])]
data = [t for t in data if t['type'] == 'annotation' or all('HPAllocator' not in l['function'] for l in t['trace'])]
data = [t for t in data if t['type'] == 'annotation' or all('Guard' not in l['function'] for l in t['trace'])]
#data = [t for t in data if t['type'] == 'annotation']

from flask import Flask, render_template
app = Flask(__name__, template_folder='.')

@app.route('/')
def hello_world():
    return render_template('dump.html', transitions=data)

if __name__ == '__main__':
    app.run(debug=True)

