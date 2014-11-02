import random
import socket

defaultPort = 10000
fraction = 0.05
totalNodeNum = 2**8
failNodeNum = int(totalNodeNum * fraction)

node = set(range(defaultPort, defaultPort+totalNodeNum))
failNode = []


## UDP socket
UDP_IP = "127.0.0.1"

msg = bytearray([0xC0,101])

print msg

if __name__ == "__main__":
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
	sock.sendto(msg, (UDP_IP, defaultPort))
	"""
	for i in range(failNodeNum):
		pick = random.choice(list(node))
		node.remove(pick)
		failNode.append(pick)

	for i in failNode:
		sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
		sock.sendto(msg, (UDP_IP, i))

print failNode
"""
