found = 0
notFound = 0

if __name__ == "__main__":
	fp = open("output/sim_10000","r")

	while True:
		line = fp.readline()
		if not line:
			break
		fp2 = open("key/" + line, "r")
		while True:
			line2 = fp2.readline()
			if not line:
				notFound += 1
				break
			if line2 is line:
				found += 1

print notFound, found, found/notFound