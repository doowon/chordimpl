import random 
import time
import subprocess
import os
import shutil

finTime = 300

procs = [] # a list of processes 

numNodes = 100
kbits = 9
firstNode = 1 #first node 0
nodes = random.sample(xrange(2, ((2**kbits)-1)), numNodes)
# nodes = [84, 111, 121, 34, 53, 367, 64, 100, 163, 419]

os.popen('rm -f ./output/*')

directory = 'output'
if not os.path.exists(directory):
	os.makedirs(directory)

fNdOutput = open(directory + '/' + str(firstNode) + '.out', 'w')
p = subprocess.Popen(['./chord 1'], shell=True, stdout=fNdOutput, stderr=fNdOutput)
print 'Node 1 starts running'
procs.append(p)

for i in nodes:
	time.sleep(3)
	tmpFileOutput = open(directory + '/' + str(i) + '.out', 'w')
	p = subprocess.Popen(['./chord ' + str(i)], shell=True, stdout=tmpFileOutput, stderr=tmpFileOutput);
	print 'Node ' + str(i) + ' starts running'
	procs.append(p)

nodes.sort()
print nodes

time.sleep(finTime)



# for i in procs:
# 	print str(i.pid) + ' is terminated'
# 	i.terminate()
os.system("killall -9 chord")
