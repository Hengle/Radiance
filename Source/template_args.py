# template_args.py
# Copyright (c) 2010 Sunside Inc., All Rights Reserved
# Generates templated function arguments
# Author: Joe Riedel
# See Radiance/LICENSE for licensing terms.

# $T = typename A1,... typename AN
# $A = A1 &a1, ... AN &an
# $P = a1, ... an

import os
import math

def _t(expression, num):
    x = "typename A1"
    for n in range(1, num):
        x = x + ", typename A%d" % (n+1)
    return expression.replace("$T", x)

def _a(prefixes, indexes, expression, num):
    p = prefixes[indexes[0]]
    if p != '':
        x = "%s A1 &a1" % p
    else:
        x = "A1 &a1"
        
    for n in range(1, num):
        p = prefixes[indexes[n]]
        if p != '':
            x = x + ", %s A%d &a%d" % (p, n+1, n+1)
        else:
            x = x + ", A%d &a%d" % (n+1, n+1)
    return expression.replace("$A", x)

def _p(expression, num):
    x = "a1"
    for n in range(1, num):
        x = x + ", a%d" % (n+1)
    return expression.replace("$P", x)

def _x(prefixes, expression, num, stream):
    p = [0 for x in range(0, num)] # closures modify this index list
    z = None
    for n in range(0, num):
        def closure(sub, n):
            def f():
                for x in range(0, len(prefixes)):
                    p[num-n-1] = x
                    if sub:
                        for y in sub():
                            yield y
                    else:
                        yield xrange
            return f
        z = closure(z, n) # closure over z
    for n in z():
        x = _t(_p(_a(prefixes, p, expression, num), num), num)
        #print x
        stream.write(unicode(x))    

def gen(prefixes, expression, num, stream):
    #expression = expression.replace('$R', os.linesep)    
    for n in range(0, num):
        _x(prefixes, expression, n+1, stream)

# the number of variations emitted:
# p = len(prefixes)
# n = num arguments
# f = p^n + p^(n-1) + ... p^1
def num(prefixes, num):
    l = len(prefixes)
    p = [math.pow(l, x) for x in range(1, num+1)]
    return sum(p)
    
