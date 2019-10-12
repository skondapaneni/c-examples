#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>        
#include "ipv4_addr.h"
#include "logger.h"

patriciaTrieNode<address_t> *root = new patriciaTrieNode<address_t>();

inline unsigned int getValue(address_t *addr) {
	unsigned int ip = 0;

	ip = (unsigned int) addr->bytes[0];
	ip = (ip << 8) | (unsigned int) addr->bytes[1];
	ip = (ip << 8) | (unsigned int) addr->bytes[2];
	ip = (ip << 8) | (unsigned int) addr->bytes[3];

	return ip;
}

unsigned int parseAddress(char *ip, address_t *addr) {
	const char *s = ".";
	char *token;
	int i = 0;

	/* get the first token */
	token = strtok(ip, s);
	addr->bytes[i++] = (unsigned char) atoi(token);

	/* walk through other tokens */
	while (token != NULL) {
		token = strtok(NULL, s);
		if (i > 3 && token == NULL) {
			break;
		} else {
			addr->bytes[i++] = (unsigned char) atoi(token);
		}
	}
	return getValue(addr);
}

address_t *insertIp(const char *ipstr) {
	address_t *ip = (address_t *) malloc(sizeof(address_t));
	char *ipstr_dup = strndup(ipstr, strlen(ipstr));
	unsigned int key = parseAddress(ipstr_dup, ip);
	free(ipstr_dup);

	log_info("###### insertIp for %s\n", ipstr);
	patriciaTrieKey *ptk = new patriciaTrieKey((key & bitMask(32)), 32);
	root->insertNode(ptk, ip);
        return ip;
}

address_t *allocIp(const char *subnet, int mask) {
	address_t ip = { '0' };
	char *subnet_dup = strndup(subnet, strlen(subnet));
	unsigned int key = parseAddress(subnet_dup, &ip);
	free(subnet_dup);

	log_info("###### allocIp for %s\n", subnet);
	patriciaTrieKey *ptk = new patriciaTrieKey((key & bitMask(mask)), mask);
	patriciaTrieNode<address_t> *r = root->lookup(ptk);
	if (r != NULL) {
		int i = 0;
		while (true) {
			i++;
			unsigned int newIp = (key & bitMask(mask)) + i;
			log_info("new Ip %d - %u\n", i, newIp);
			patriciaTrieKey *child = new patriciaTrieKey(newIp,
					32 - r->GetKey()->getBitIdxBegin(),
					r->GetKey()->getBitIdxBegin());
			if (child != NULL) {
				patriciaTrieNode<address_t> *found = r->lookup(child);
				if (found != NULL) {
					free(child);
					child = NULL;
				} else {
					patriciaTrieKey *child = new patriciaTrieKey(newIp, 32);
					address_t *ipv4 = (address_t *) malloc(sizeof(address_t));
					ipv4->bytes[0] = (newIp >> 24) & 0xFF;
					ipv4->bytes[1] = (newIp >> 16) & 0xFF;
					ipv4->bytes[2] = (newIp >> 8) & 0xFF;
					ipv4->bytes[3] = (newIp & 0xFF);
					root->insertNode(child, ipv4);
					log_info("inserted child\n");
					child->print();
					log_info("========== inserted child done =========== \n");
					return ipv4;
					break;
				}
			}
		}
	} else {
		log_info("subnet %s not found!!\n", subnet);
		int i = 1;
		unsigned int newIp = (key & bitMask(mask)) + i;
		patriciaTrieKey *child = new patriciaTrieKey(newIp, 32);
		if (child != NULL) {
			address_t *ipv4 = (address_t *) malloc(sizeof(address_t));
			ipv4->bytes[0] = (newIp >> 24) & 0xFF;
			ipv4->bytes[1] = (newIp >> 16) & 0xFF;
			ipv4->bytes[2] = (newIp >> 8) & 0xFF;
			ipv4->bytes[3] = (newIp & 0xFF);
			root->insertNode(child, ipv4);
			return ipv4;
		}
	}
	return NULL;
}

patriciaTrieNode<address_t> *
findIp(const char *ipstr) {
	address_t ip = { '0' };
	char *ipdup = strndup(ipstr, strlen(ipstr));
	unsigned int key = parseAddress(ipdup, &ip);
	free(ipdup);

	patriciaTrieKey *ptk = new patriciaTrieKey(key, 32);
	patriciaTrieNode<address_t> *r = root->lookup(ptk);
	delete ptk;
	return r;
}

patriciaTrieNode<address_t> *
deleteIp(const char *ipstr) {
	address_t ip = { '0' };
	char *ipdup = strndup(ipstr, strlen(ipstr));
	unsigned int key = parseAddress(ipdup, &ip);
	free(ipdup);

	patriciaTrieKey *ptk = new patriciaTrieKey(key, 32);
	patriciaTrieNode<address_t> *r = root->deleteNode(ptk);
	delete ptk;
	return r;
}

patriciaTrieNode<address_t> *
deleteIp(address_t *ip) {
        if (!ip) return NULL;
        unsigned int key = getValue(ip);
	patriciaTrieKey *ptk = new patriciaTrieKey(key, 32);
	patriciaTrieNode<address_t> *r = root->deleteNode(ptk);
	delete ptk;
	return r;
}

void printIpList() {
	log_info("========== trie =========== \n");
	root->print(0);
	log_info("========== end of trie =========== \n");
}

