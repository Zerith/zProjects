import os, sys
import random
import socket
import bit_class

##
##	This script solves some challenge in some CTF where it is needed to predict a Mersenne Twister LCG
##

#
#	getOriginalMTValue - converts an output of Mersenne Twistter to the original entry in the MT state.
#
def getOriginalMTValue(mt):

	#orig = mt ^ (mt >> 18)
	#orig = orig ^ ((orig << 15) & 0xEFC60000)
	orig = test.deXor_RSWithAnd(mt, 18, 0xFFFFFFFF)
	orig = test.deXor_LSWithAnd(orig, 15, 0xEFC60000)
	orig = test.deXor_LSWithAnd(orig, 7, 0x9D2C5680)
	orig = test.deXor_RSWithAnd(orig, 11, 0xFFFFFFFF)
		
	return orig

#
# Connect to guessing server
#

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('88.198.89.194', 8888))

s.recv(1024)	#prologue
s.recv(1024)
r = random.Random()

table = []
entries = 0
while entries < 312:

	s.send('123\n')
	text = s.recv(1024)
	s.recv(1024)	#nextguesstext
	n = int(text[text.find('been') + len('been '):text.find('...')])

	table.append(getOriginalMTValue(n & 0xFFFFFFFF))
	table.append(getOriginalMTValue(n >> 32))

	entries = entries + 1
	if entries%10 == 0:
		print '%u' % (entries)
	
table.append(624)
table = tuple(table)
r.setstate( (3, table, None) )	#(3(??), table, index)
for i in range(11):
	guess =  str(r.getrandbits(64))
	s.send(guess +'\n')
	print s.recv(1024)


prints.recv(1024)




	




