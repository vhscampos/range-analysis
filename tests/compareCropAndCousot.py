#! /usr/bin/env python

import re
import subprocess

def readIntervals(fileName):
	f = open(fileName)
	intervals = {}
	intersections = []
	for line in f:
		m = re.search('(?<=label=\").+ \[.*\](?=\"])', line)
		if m != None:
			x = m.group(0).split(' ')
			intervals[x[0]] = "".join(x[1:3])
		else:
			m = re.search('(?<=label=\")\[.*\](?=\"])', line)
			if m != None:
				intersections.append(m.group(0))
	return {'vars': intervals, 'intersections': set(intersections)}

def checkVariables(cropIntervals, cousotIntervals):
	print "Checking var intervals:"
	variables = {}
	for v in cropIntervals:
		variables[v] = {'crop':cropIntervals[v], 'cousot':None}
	for v in cousotIntervals:
		if v in variables:
			variables[v]['cousot'] = cousotIntervals[v]
		else:
			variables[v] = {'cousot':cousotIntervals[v], 'crop':None}

	equal = True
	for v, i in variables.iteritems():
		if i['crop'] != i['cousot']:
			if equal:
				print '%-30s - %25s - %s' % ("VAR","CROP","COUSOT")
			print '%-30s - %25s - %s' % (v,str(i['crop']), str(i['cousot']))
			equal = False
	if equal:
		print "\tSame result"

def checkIntersections(cropIntervals, cousotIntervals):
	print "Checking intersections:"
	cropDiff = set(cropIntervals)-set(cousotIntervals)
	cousotDiff = set(cousotIntervals)-set(cropIntervals)
	
	if len(cropDiff) > 0 or len(cousotDiff) > 0:
		print "Differences between them: "
		print "\tCrop have: "#, cropDiff
		for r in cropDiff:
			print "\t", r, "  "
		print "\tCousot have: ", cousotDiff
	else:
		print "\tSame result"


def checkIntervals(cropFileName, cousotFileName):
	cropIntervals = readIntervals(cropFileName)
	cousotIntervals = readIntervals(cousotFileName)
	checkVariables(cropIntervals['vars'], cousotIntervals['vars'])
	checkIntersections(cropIntervals['intersections'], cousotIntervals['intersections'])


tests = ['t1.c','t2.c','t3.c','t4.c','t5.c','t6.c','t7.c']
for test in tests:
	subprocess.call("./compile.py -ra-intra-crop "+test, shell=True, stdout=open('/tmp/log',"a"),stderr=open('/tmp/log',"a"))
	subprocess.call("cp /tmp/foocgpos.dot /tmp/foocgpos."+test+".crop.dot", shell=True)
	subprocess.call("./compile.py -ra-intra-cousot "+test, shell=True, stdout=open('/tmp/log',"a"),stderr=open('/tmp/log',"a"))
	subprocess.call("cp /tmp/foocgpos.dot /tmp/foocgpos."+test+".cousot.dot", shell=True)
	
	print "Checking [",test,"]: "
	checkIntervals('/tmp/foocgpos.'+test+'.crop.dot', '/tmp/foocgpos.'+test+'.cousot.dot')
	print "--------------------------------------------------------------------"


