
class bit_representation:

	def __init__(self, n):
		self.decimal = n

	def __getitem__(self, key):
		if isinstance(key, slice):
			start = key.start
			stop = key.stop
			result = self.decimal >> start
			result = (result & ((1 << stop)-1))

			return bit_representation(result)
		else:
			result = (self.decimal & (1 << key))
			return (result > 0)

	def __setitem__(self, index, src):
		if src == 1:
			self.decimal |= (1 << index)
		elif src == 0:
			self.decimal &= (~(1 << index))

	def get_int(self):
		return self.decimal
		

def deXor_LSWithAnd(n, y, e):	#n ^ ((n << y) & e)
	n = bit_representation(n)
	e = bit_representation(e)

	deXored = n[0:y]
	for i in xrange(y, 32):
		if e[i] == 0:
			deXored[i] = n[i]
		else:
			deXored[i] = n[i] ^ deXored[i-y]

	
	return deXored.get_int()


def deXor_RSWithAnd(n, y, e):	#n ^ ((n >> y) & e)
	n = bit_representation(n)
	e = bit_representation(e)
	
	deXored = bit_representation(0)	
	for i in xrange(31-y, 32):
		deXored[i] = n[i]

	for i in xrange(31-y, -1, -1):
		if e[i] == 0:
			deXored[i] = n[i]
		else:
			deXored[i] = n[i] ^ deXored[i+y]

	
	return deXored.get_int()
















