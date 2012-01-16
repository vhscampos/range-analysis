#! /usr/bin/env python

import sys
import os

if len(sys.argv) == 2:
    fileName, fileExtension = os.path.splitext(sys.argv[1])
    print "dot -Tpdf "+sys.argv[1]+" -o "+fileName+".pdf"
    os.system("dot -Tpdf "+sys.argv[1]+" -o "+fileName+".pdf")
    print "evince "+fileName+".pdf"
    os.system("evince "+fileName+".pdf &")
else:
    print "./debug <DOT-FILE>"


