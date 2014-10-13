import os
import hashlib
import operator
import random
import subprocess
import time

numNode = 2**10
defaultPort = 10000
unsortedNodes = {}

def generateNodes():
	for i in range(defaultPort, defaultPort+numNode):
		m = hashlib.sha1()
		m.update(str(i))
		unsortedNodes[str(i)] = m.hexdigest()

	nodes = sorted(unsortedNodes.items(), key=operator.itemgetter(1))

	if not os.path.exists('node'):
		os.makedirs('node')
	os.popen('rm -f ./node/*')

	for n, h in nodes:
		file = open('node/' + n, 'w')
		file.write(h)

	print nodes
	return nodes

def generateRandomkeys(nodes):
	if not os.path.exists('key'):
		os.makedirs('key')
	os.popen('rm -f ./key/*')

	keys = []
	for i in range(100 * numNode):
		r = random.random()
		m = hashlib.sha1()
		m.update(str(r))
		digest = m.hexdigest()
		keys.append(digest)

		for n, h in nodes:
			if digest <= h:
				file = open('key/'+n, 'a')
				file.write(digest+'\n')
				break;

		file = open('key/'+ nodes[0][0], 'a')
		file.write(digest+'\n')

	return keys

def generateRandomLookupKeys(nodes, keys):
	if not os.path.exists('lookupKey'):
		os.makedirs('lookupKey')
	os.popen('rm -f ./lookupKey/*')

	for n, h in nodes:
		r = random.randint(1,100)
		if len(keys) < r:
			lookupKeys = random.sample(keys, len(keys))
		else:
			lookupKeys = random.sample(keys, r)
		file = open('lookupKey/'+ n, 'a')
		for k in lookupKeys:
			file.write(k+'\n')



if __name__ == "__main__":
	nodes = generateNodes()
	keys = generateRandomkeys(nodes)
	generateRandomLookupKeys(nodes, keys)

	if not os.path.exists('output'):
		os.makedirs('output')

	os.popen('rm -f ./output/*')

	tmpFileOutput = open('output/10000', 'w')
	simFileOutput = open('output/sim_10000', 'w')
	p = subprocess.Popen(['./chord', '-p10000', '-fnode/10000', '-kkey/10000', '-llookupKey/10000'], shell=False, stdout=simFileOutput, stderr=tmpFileOutput, preexec_fn=os.setsid)
	print 'Node 10000'  + ' starts running, pid:' + str(p.pid)

	for n, h in unsortedNodes.iteritems():
		time.sleep(1)
		if n != str(defaultPort):
			tmpFileOutput = open('output/' + str(n), 'w')
			simFileOutput = open('output/sim_' + str(n), 'w')
			arg = ['-p' + str(n), '-fnode/' + str(n), '-kkey/' + str(n), '-llookupKey/' + str(n)]
			# print arg
			p = subprocess.Popen(['./chord', arg[0], arg[1], arg[2], arg[3]], shell=False, stdout=simFileOutput, stderr=tmpFileOutput, preexec_fn=os.setsid)
			print 'Node ' + str(n) + ' starts running, pid:' + str(p.pid)
