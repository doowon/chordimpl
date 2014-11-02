import glob
import numpy as np

# max = 0
results = []
flists = glob.glob('output/sim_*')
for f in flists:
	# r = []
	file = open(f,'r')
	while True:
		line = file.readline()
		if not line:
			break
		# if not "START Simulation" in line:
		results.append(int(line.strip()))
	# 	r.append(int(line.strip()))
	# tmp = np.array(r)
	# print "mean "  + str(np.mean(tmp))
	# if max < np.mean(tmp):
	# 	max = np.mean(tmp)	

# print results
a = np.array(results)
# print np.std(a)
print np.mean(a)
print np.percentile(a, 1)
print np.percentile(a, 99)
# print "max " + max 
