#include "patriciaTrie.h"

extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>        
}

#include "logger.h"

unsigned int MSB = 1l << 31;
unsigned int MAX_BIT_LEN = 32;

/**
 * Returns a bit mask where the given bit is set
 */
long mask(int bit) {
	return MSB >> bit;
}

/**
 * Returns the bit at the i'th index in key.
 */
unsigned int bit_i(unsigned int key, int i) {
	return key & mask(i);
}

/**
 * Returns a bitmask of all 1's from the MSB.
 */
unsigned int bitMask(unsigned int bits) {
	unsigned int bmask = 0;
	for (int i = 0; i < bits; i++) {
		bmask |= mask(i);
	}
	return bmask;
}

char *address_to_str(address_t *addr, char *addr_str)
{
      sprintf(addr_str, "%d.%d.%d.%d", (int) addr->bytes[0], (int) addr->bytes[1],
           (int) addr->bytes[2], (int) addr->bytes[3]);
      return addr_str;
}

patriciaTrieKey::patriciaTrieKey() :
				key(0), bitLen(0), bitIdxBegin(0) {
}

patriciaTrieKey::patriciaTrieKey(unsigned int key_a, int bitLen_a) :
				key(key_a), bitLen(bitLen_a), bitIdxBegin(0) {
}

patriciaTrieKey::patriciaTrieKey(unsigned int key_a, int bitLen_a,
		int bitIdxBegin_a) :
				key(key_a), bitLen(bitLen_a), bitIdxBegin(bitIdxBegin_a) {
}

patriciaTrieKey::~patriciaTrieKey() {
}

/*
 * Compares the otherKey and this key, returns the bitIndex at which
 * the 2 keys differ.
 */
int patriciaTrieKey::bitIndex(unsigned int otherKey) {
	if (bitLen == 0) {
		return NULL_BIT_KEY ;
	}

	if (key != otherKey) {
		unsigned int xorValue = key ^ otherKey;
		for (int i = bitIdxBegin; i < bitIdxBegin + bitLen; i++) {
			if ((xorValue & mask(i)) != 0) {
				return i;
			}
		}
	}

	return EQUAL_BIT_KEY ;
}

unsigned int patriciaTrieKey::prefix(unsigned int maskLen) {

	unsigned int bmask = 0;
	for (int i = bitIdxBegin; i < bitIdxBegin + maskLen; i++) {
		bmask |= bit_i(key, i);
	}

	return bmask;
}

unsigned int patriciaTrieKey::longestPrefixBitLen(patriciaTrieKey *otherKey) {
	int prefix;

	if (otherKey->bitLen < bitLen) {
		prefix = otherKey->bitIndex(this->key);
		if (prefix == EQUAL_BIT_KEY) {
			return otherKey->bitLen;
		}
	} else {
		prefix = bitIndex(otherKey->key);
		if (prefix == EQUAL_BIT_KEY) {
			return bitLen;
		}
	}

	if (prefix == NULL_BIT_KEY) {
		return NULL_BIT_KEY ;
	}

	return prefix - bitIdxBegin;
}

int patriciaTrieKey::trimPrefix(int prefix_len, patriciaTrieKey *pkey) {
	unsigned int pkey_val = 0;
	int i;

	pkey_val = prefix(prefix_len);

	unsigned int newKey = 0;
	for (i = bitIdxBegin + prefix_len; i < bitLen + bitIdxBegin; i++) {
		newKey |= bit_i(key, i);
	}

	// new prefix
	if (pkey != NULL) {
		pkey->bitLen = prefix_len;
		pkey->bitIdxBegin = bitIdxBegin;
		pkey->key = pkey_val;
	}

	// modify
	bitLen -= prefix_len;
	key = newKey;
	bitIdxBegin += prefix_len;

	return bitLen;
}

patriciaTrieKey *
patriciaTrieKey::mergeKey(patriciaTrieKey *child) {
	unsigned int newKey = key;
	int begin = child->bitIdxBegin;
	int end = child->bitLen + child->bitIdxBegin;

	for (int i = begin; i < end; i++) {
		newKey |= bit_i(child->key, i);
	}
	this->bitLen += child->bitLen;
	this->key = newKey;

	return this;
}

void patriciaTrieKey::print() {
	fprintf(stdout, "key 0x%x bitLen %d bitIdxBegin %d\n", key, bitLen, bitIdxBegin);
}

