import glob
import numpy as np

results = []

flists = glob.glob('output/sim_*')
for f in flists:
	file = open(f,'r')
	while True:
		line = file.readline()
		if not line:
			break
		results.append(int(line.strip()))

# print results
a = np.array(results)
# print np.std(a)
print np.mean(a)
print np.percentile(a, 1)
print np.percentile(a, 99)
