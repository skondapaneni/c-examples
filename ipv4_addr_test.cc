#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>
#include "ipv4_addr.h"
#include "logger.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;

int _main(int argc, char **argv) {

	char addr1[] = "172.35.254.2";
	char addr2[] = "172.35.127.2";
	char addr3[] = "172.36.2.5";
	char addr4[] = "172.36.254.5";
	char addr5[] = "172.37.1.2";
	char addr6[] = "172.37.1.3";
	char addr7[] = "172.37.1.4";
	char addr8[] = "172.35.127.3";
	char addr9[] = "172.35.127.0";
	char addr10[] = "172.35.254.2";

	insertIp(addr1);
	insertIp(addr2);
	insertIp(addr3);
	insertIp(addr4);
	insertIp(addr5);
	insertIp(addr6);
	insertIp(addr7);
	insertIp(addr8);
	insertIp(addr9);
	insertIp(addr10);

	insertIp(addr1);
	insertIp(addr3);
/*
	for (int i = 0; i < 255; i++) {
		allocIp("172.25.25.0", 24);
	}

	for (int i = 0; i < 255; i++) {
		allocIp("172.25.1.0", 24);
	}

	char _addr1[] = "172.25.25.8";
	patriciaTrieNode<address_t> * p1 = deleteIp(_addr1);
	if (p1)
		delete p1;

	char _addr2[] = "172.25.25.5";
	patriciaTrieNode<address_t> * p2 = deleteIp(_addr2);
	if (p2)
		delete p2;

	for (int i = 0; i < 10; i++) {
		string ip = "172.25.25." + std::to_string(i);
		fprintf(stdout, "findIp %s\n", ip.c_str());
		patriciaTrieNode<address_t> * p = findIp(ip.c_str());
		if (p != NULL) {
			p->print(0);
		} else {
			fprintf(stdout, "#### %s Not Found ### \n", ip.c_str());
		}
	}

*/
	printIpList();

}
