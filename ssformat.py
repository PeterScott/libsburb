# Snarfstrip input data format: a collection of patches in the form
#
# chains len+
# atom*
#
# Where atom = char-code pred-yarn pred-off id-yarn id-off\n

import os, sys

def shorthand(str):
    """Convert a shorthand patch to a real patch."""
    def yarn(c):
        if c == '0': return 0
        else: return ord(c) - ord('a') + 1
        
    chains = []
    atoms = []
    curtype = None
    for i in range(0, len(str), 5):
        c, pred, id = str[i], str[i+1:i+3], str[i+3:i+5]
        pred = (yarn(pred[0]), int(pred[1]))
        id   = (yarn(id[0]),   int(id[1]))
        if c == '^':
            if curtype and curtype != '^':
                chains.append(atoms); atoms = []
            curtype = '^'
            atoms.append((0xE002, pred[0], pred[1], id[0], id[1]))
        elif c == '*':
            if curtype and curtype != '*':
                chains.append(atoms); atoms = []
            curtype = '*'
            atoms.append((0xE003, pred[0], pred[1], id[0], id[1]))
        else:
            if curtype and curtype != 'i':
                chains.append(atoms); atoms = []
            curtype = 'i'
            atoms.append((ord(c), pred[0], pred[1], id[0], id[1]))
    if len(atoms) > 0:
        chains.append(atoms)
    return chains

p1 = shorthand('T01a1ea1a2sa2a3ta3a4')
p2 = shorthand('^a3b1xa2b2')
p3 = shorthand('*b2a5')

def write_patch(patch, file=sys.stdout):
    """Write a patch to a file, stdout by default."""
    file.write('%d ' % len(patch))
    file.write(' '.join(str(len(chain)) for chain in patch))
    file.write('\n')

    for chain in patch:
        for atom in chain:
            file.write('%d %d %d %d %d\n' % atom)

