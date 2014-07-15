chordimpl
=========

Implementation of Chord


This currernt version is being tested in only a local machine so that a chord 
node should have to only one unique port number. 

In Chord, the default port such as 6000 is used for the join operation, but 
because this version is for only local machine and each chord node has to have 
a unique port number, every node first tries to connect the main chord node via 
IP sockets for the join operation.

