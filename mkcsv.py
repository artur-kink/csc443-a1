import sys
import random

letters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
try:
    filename, n_tuples = sys.argv[1:3]
    n = int(n_tuples)
except:
    print 'Usage: %s <outfile> <number of tuples>' % sys.argv[0]
    sys.exit(0)

with open(filename, 'w') as f:
    for _ in xrange(n):
        row = []
        for a in range(100):
            row.append(''.join([random.choice(letters) for j in range(10)]))
        print >>f, ",".join(row)

print "Generated %d random tuples in %s." % (n, filename)        
