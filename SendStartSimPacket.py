import socket
from time import sleep

UDP_IP = "127.0.0.1"
UDP_PORT = 10000

msg = bytearray([0xC0,100])
numNode = 2**8

print msg

for i in range(UDP_PORT, UDP_PORT+numNode):
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
	sock.sendto(msg, (UDP_IP, i))
	print i
	# sleep(0.05)

print "Done with sending REQ_START_SIM"