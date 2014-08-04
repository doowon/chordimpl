import random 
import time
import subprocess
import os
import psutil
import signal

finTime = 30
failProcs = [] # a list of failure processes 
numNodes = 10
# failTime = 
failNumNodes = int(numNodes * 0.1)
kbits = 9
firstNode = 1 #first node 0
nodes = random.sample(xrange(2, ((2**kbits)-1)), numNodes)
failNodes = random.sample(nodes, failNumNodes)

# nodes = [45, 264, 287, 288, 471]
# failNodes = [471, 287]

directory = 'output'
if not os.path.exists(directory):
	os.makedirs(directory)

os.popen('rm -f ./output/*')

''' Running the first node '''
fNdOutput = open(directory + '/' + str(firstNode) + '.out', 'w')
p = subprocess.Popen(["./chord", "-n1"], shell=False, stdout=fNdOutput, stderr=fNdOutput)
print 'Node 1 starts running, pid: ' + str(p.pid)


''' Running other nodes '''
for i in nodes:
	time.sleep(3)
	tmpFileOutput = open(directory + '/' + str(i) + '.out', 'w')
	# if i in failNodes:
		# p = subprocess.Popen(["chord", "-n"+ str(i), "-f"+str(finTime)], shell=True, stdout=tmpFileOutput, stderr=tmpFileOutput);
	# else:
	p = subprocess.Popen(["./chord", "-n"+ str(i)], shell=False, stdout=tmpFileOutput, stderr=tmpFileOutput, preexec_fn=os.setsid);
	print 'Node ' + str(i) + ' starts running, pid:' + str(p.pid)
	if i in failNodes:
		failProcs.append(p)

nodes.sort()
print "Running nodes: "
print nodes

print "Testing for " + str(finTime) + " seconds"

time.sleep(finTime)

print "Starting to test failures"
print "Failure nodes are"
print failNodes


for i in failProcs:
	os.killpg(i.pid, signal.SIGTERM)
	# p = psutil.Process(i.pid);
	# p.terminate()
	# p = psutil.Process(i.pid+1);
	# p.kill()
	# os.system("kill -9 " + str(i.pid))
	# os.system("kill -9 " + str(i.pid+1))
	time.sleep(2)

failNodes.sort()
print failNodes

print "Final nodes"
for i in failNodes:
	nodes.remove(i)

print nodes

time.sleep(finTime)

os.system("killall -9 chord")
