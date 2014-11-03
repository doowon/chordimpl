found = 0
notFound = 0

if __name__ == "__main__":
	fp = open("output/sim_10000","r")

	while True:
		line = fp.readline()
		if not line:
			break
		words = line.split()
		if len(words) != 3:
			continue
		
		# if words[2] == "10185":
		# 	continue
		fp2 = open("key/" + words[2], "r")
		while True:
			line2 = fp2.readline()
			if not line2:
				notFound += 1
				# print line.strip()
				break
			
			l = len(words[0])
			if l == 39:
				words[0] = "0" + words[0]
			elif l == 38:
				words[0] = "00" + words[0]
			
			if line2.strip() == words[0]:
				found += 1
				break

print notFound, found, notFound+found, float(notFound)/float((notFound+found))