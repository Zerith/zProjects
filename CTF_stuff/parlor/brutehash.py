import socket
import hashlib
import time
import random
import struct

from ctypes import *

md5_state = cdll.LoadLibrary('./find_state.so')

IP = '54.197.195.247'
PORT = 4321
s = socket.socket()
s.connect((IP, PORT))
s.recv(10000) #receive the 'hello' message

def md5_pad(length): 
	""" 
	generate an md5 padding for a string 
		 
	@parameter  length  length of the string 
	""" 
	pad = '' 
					  
	n0 = ((56 - (length + 1) % 64) % 64) 
	pad += '\x80' 
	pad += '\x00'*n0 + struct.pack('Q', length*8) 
	   
	return pad 


def get_md5_partial_state(x):
	"""
	Connect to server and retrieve partial MD5 state for (nonce + s)
	"""
	s.send('1\n')		#set odds
	s.recv(1024)
	s.send('100\n')		#2**100
	s.recv(1024)
	s.recv(1024)
	s.send('3\n')		#play round
	s.recv(1024)
	s.send(x)			#send nonce
	s.recv(1024)

	result = s.recv(1024)
	result = int(result.split(' ')[4][:-1])
	result = hex(result)[2:-1]
	print 'received partial state: ' + result + ' from server'

	return '0' + result


def generate_random_string(n):
	"""
	Generate a random string of length n
	"""
	string = ''
	for i in xrange(n):
		string += chr(random.getrandbits(8))

	return string

def bruteforce_upper_28_bits(start_state, target_state, appendum):
	"""
	Bruteforces the 28 upper bits of [start_state] to find the state which
	when updated with [appendum], produces [target_state].

	The actual bruteforce occurs in an imported C function for speed considerations.
	"""

	#
	# Initialize an MD5 context to send to find_md5_state
	#
	start_state = int(start_state, 16)
	md5_ctx = [((start_state >> (x * 32)) & 0xFFFFFFFF) for x in range(4)]
	md5_ctx.reverse()	

	start_state = (c_uint32 * 4) (*md5_ctx)
	target_state = create_string_buffer(target_state.decode('hex'))
	full_starting_state = create_string_buffer(32)

	state = md5_state.find_md5_state(byref(start_state), 
									byref(target_state),
									appendum,
									byref(full_starting_state))

	result = full_starting_state.value.encode('hex')

	return result
	
def md5_update_state(state, appendum):
	"""
	Takes an initial MD5 state and updates it with appendum

	Actual functionality is implemented in an imported C function for 
	speed considerations.
	"""
	start_state = int(state, 16)
	md5_ctx = [((start_state >> (x * 32)) & 0xFFFFFFFF) for x in range(4)]
	md5_ctx.reverse()	
	start_state = (c_uint32 * 4) (*md5_ctx)
	
	result = create_string_buffer(32)
	md5_state.update_md5_state(byref(start_state), appendum, byref(result))

	return result.value

def main():
	"""
	1. A = ( Retrieve 100 low bits of md5(nonce + 'a') by sending 'a'
	2. B = ( Retrieve 100 low bits of md5(pad(nonce + 'a') + 'b')
		by sending 'a' + padding + 'b' 

	[+] Bruteforce upper 28 bits of md5 state by length extension attack:
	3. for bits in xrange(0, 2**28):
			state = A + bits
			
			digest = md5_extend_with_state(state, 'b')
			
			if ((digest % (2**100)) == B):
				break # we found the state!

	[+] Find a winning appendum to the full state [Length extension attack, again..]
	4. for appendum in range(0, 2**20):
			digest = md5_extend_with_state(state, appendum)
			
			if ((digest % (2**20)) == 0)
				return appendum

	5. bet $990 and send 'a' + padding + appendum

	"""

	nonce = 'A'*16 	#dummy nonce: for padding purposes only the length matters..

	original_state = get_md5_partial_state('a')
	state_plus_b = get_md5_partial_state('a' + md5_pad(len(nonce + 'a')) + 'b')

	print 'Bruteforcing upper 28 bits of the first MD5 state..'
	full_state = bruteforce_upper_28_bits(original_state, state_plus_b, 'b')
	print full_state
	print 'looking for a winning appendum..'

	#
	#	Find an extending appendum to the full md5 state that 
	#	produces an MD5 % (2**16) == 0
	#

	bets = [64880648, 990]

	for i in range(2):
		appendum = ''
		while True:
			appendum = generate_random_string(3) #24 bits of randz	
			md = md5_update_state(full_state, appendum)
			if int(md, 16) % (2**16) == 0:
				print md
				break


		print 'Found a winning appendum!: ' + appendum.encode('hex') + ' or ' + appendum
		#
		# set odds to 2**16
		# send 'a' + md5_pad(nonce + 'a') + appendum
		#
		#
		s.send('1\n')
		print s.recv(1024)
		s.send('16\n')
		print s.recv(1024)
		print s.recv(1024)
		#set bet
		s.send('2\n')
		print s.recv(1024)
		s.send(str(bets.pop()) + '\n')
		print s.recv(1024)
		print s.recv(1024)
		#play round
		s.send('3\n')
		print s.recv(1024)

		s.send('a' + md5_pad(len(nonce + 'a')) + appendum)
		print s.recv(1024)
		print s.recv(4096)


		print s.send('4\n')
		print s.recv(1024)
		print s.recv(1024)



if '__main__' == __name__:
	main()

