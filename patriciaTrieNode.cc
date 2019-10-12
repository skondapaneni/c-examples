#include "patriciaTrie.h"

extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>        
}

#include "logger.h"


template<typename T>
patriciaTrieNode<T>::patriciaTrieNode() :
key(NULL), data(NULL), left(NULL), right(NULL) {
}

template<typename T>
patriciaTrieNode<T>::patriciaTrieNode(patriciaTrieKey *ptk, T* data,
		patriciaTrieNode<T> * left, patriciaTrieNode<T> * right) :
		key(ptk), data(data), left(left), right(right) {
}

template<typename T>
patriciaTrieNode<T>::~patriciaTrieNode() {
	if (left) {
		delete left;
	}
	if (right) {
		delete right;
	}
}

template<typename T> T*
patriciaTrieNode<T>::GetData() {
	return data;
}

template<typename T> patriciaTrieKey *
patriciaTrieNode<T>::GetKey() {
	return key;
}

template<typename T> patriciaTrieNode<T> *
patriciaTrieNode<T>::GetLeft() {
	return left;
}

template<typename T> patriciaTrieNode<T> *
patriciaTrieNode<T>::GetRight() {
	return right;
}

template<typename T> void patriciaTrieNode<T>::SetRight(
		patriciaTrieNode<T> *right) {
	this->right = right;
}

template<typename T> void patriciaTrieNode<T>::SetLeft(
		patriciaTrieNode<T> *left) {
	this->left = left;
}

template<typename T> void patriciaTrieNode<T>::insertNode(patriciaTrieKey *pkey,
		T *addr) {
	if (key == NULL) {
		unsigned int bit = bit_i(pkey->getKey(), pkey->getBitIdxBegin());
		if (bit >= 1) {
			if (right != NULL) {
				right->insertNode(pkey, addr);
				return;
			}
			right = new patriciaTrieNode<T>(pkey, addr, NULL, NULL);
			return;
		} else {
			if (left != NULL) {
				left->insertNode(pkey, addr);
				return;
			}
			left = new patriciaTrieNode<T>(pkey, addr, NULL, NULL);
			return;
		}
	}

	int prefixBitLen = key->longestPrefixBitLen(pkey);

	if (prefixBitLen > 0) {
		if (prefixBitLen < key->getBitLen()) {
			patriciaTrieKey *root_key_new = new patriciaTrieKey();

			// Break down this node into two nodes - parent and child.
			// trim the prefix from both the keys
			key->trimPrefix(prefixBitLen, root_key_new);
			pkey->trimPrefix(prefixBitLen, NULL);

			patriciaTrieKey *child_key1 = key; // modified key after dropping prefix
			patriciaTrieKey *child_key2 = pkey; // modified key  after dropping prefix

			key = root_key_new; // update the key in this node
			T *child_key1_data = data; // save the data from this node and give it to child_node1
			data = NULL; // update data to NULL. This node is no longer a leaf node.

			/* insert the 2 child keys as nodes to left and right */
			patriciaTrieNode<T> *grand_child_right = right;
			patriciaTrieNode<T> *grand_child_left = left;

			unsigned int bit = bit_i(child_key1->getKey(),
					child_key1->getBitIdxBegin());
			if (bit >= 1) {
				right = new patriciaTrieNode<T>(child_key1, child_key1_data,
						grand_child_left, grand_child_right);
			} else {
				left = new patriciaTrieNode<T>(child_key1, child_key1_data,
						grand_child_left, grand_child_right);
			}

			bit = bit_i(child_key2->getKey(), child_key2->getBitIdxBegin());
			if (bit >= 1) {
				right = new patriciaTrieNode<T>(child_key2, addr, NULL, NULL);
			} else {
				left = new patriciaTrieNode<T>(child_key2, addr, NULL, NULL);
			}

		} else {
			pkey->trimPrefix(prefixBitLen, NULL);
			patriciaTrieKey *child_key2 = pkey; // modified key  after dropping prefix

			// node is already present.
			if (pkey->getBitLen() == 0) {
				return;
			}

			unsigned int bit = bit_i(child_key2->getKey(),
					child_key2->getBitIdxBegin());
			if (bit >= 1) {
				if (right != NULL) {
					right->insertNode(child_key2, addr);
				} else {
					right = new patriciaTrieNode<T>(child_key2, addr, NULL,
							NULL);
				}
			} else {
				if (left != NULL) {
					left->insertNode(child_key2, addr);
				} else {
					left = new patriciaTrieNode<T>(child_key2, addr, NULL,
							NULL);
				}
			}
		}
	} else {
		log_err("Unexpected error.. prefixBitLen is %d\n", prefixBitLen);
	}
}

template<typename T> bool patriciaTrieNode<T>::findNode(patriciaTrieKey *pkey,
		patriciaTrieNode<T> **parent, patriciaTrieNode<T> **node) {
	if (key == NULL) {
		unsigned int bit = bit_i(pkey->getKey(), pkey->getBitIdxBegin());
		*parent = this;
		if (bit >= 1) {
			if (right != NULL) {
				return right->findNode(pkey, parent, node);
			}
		} else {
			if (left != NULL) {
				return left->findNode(pkey, parent, node);
			}
		}
		return false;
	}

	int prefixBitLen = key->longestPrefixBitLen(pkey);
	if (prefixBitLen > 0) {
		if (prefixBitLen == key->getBitLen()
				|| prefixBitLen == pkey->getBitLen()) {

			pkey->trimPrefix(prefixBitLen, NULL);

			if (pkey->getBitLen() == 0) {
				*node = this;
				return true;
			}

			unsigned int bit = bit_i(pkey->getKey(), pkey->getBitIdxBegin());
			*parent = this;
			if (bit >= 1) {
				if (right != NULL) {
					return right->findNode(pkey, parent, node);
				}
			} else {
				if (left != NULL) {
					return left->findNode(pkey, parent, node);
				}
			}
		}
	}
	return false;
}

template<typename T> patriciaTrieNode<T> *
patriciaTrieNode<T>::deleteNode(patriciaTrieKey *pkey) {
	patriciaTrieNode<T> *parent = NULL, *target = NULL;
	patriciaTrieNode<T> *child = NULL;

	bool found = findNode(pkey, &parent, &target);
	if (!found)
		return NULL;

	if (parent->left == target) {
		parent->left = NULL;
	} else if (parent->right == target) {
		parent->right = NULL;
	}

	if (parent->right != NULL) {
		child = parent->right;
	} else if (parent->left != NULL) {
		child = parent->left;
	}

	if (child != NULL) {
		parent->data = child->data;
		parent->left = child->left;
		parent->right = child->right;
		parent->key->mergeKey(child->key);
	}

	return target;
}

template<typename T> patriciaTrieNode<T> *
patriciaTrieNode<T>::lookup(patriciaTrieKey *pkey) {
	if (key == NULL) {
		unsigned int bit = bit_i(pkey->getKey(), pkey->getBitIdxBegin());
		if (bit >= 1) {
			if (right != NULL) {
				return right->lookup(pkey);
			}
		} else {
			if (left != NULL) {
				return left->lookup(pkey);
			}
		}
		return NULL;
	}

	int prefixBitLen = key->longestPrefixBitLen(pkey);
	if (prefixBitLen > 0) {
		if (prefixBitLen == key->getBitLen()
				|| prefixBitLen == pkey->getBitLen()) {

			pkey->trimPrefix(prefixBitLen, NULL);

			if (pkey->getBitLen() == 0) {
				return this;
			}

			unsigned int bit = bit_i(pkey->getKey(), pkey->getBitIdxBegin());
			if (bit >= 1) {
				if (right != NULL) {
					return right->lookup(pkey);
				}
			} else {
				if (left != NULL) {
					return left->lookup(pkey);
				}
			}
		}
	}
	return NULL;
}

static void printLevel(int level) {
	for (int i = 0; i < level; i++) {
		fprintf(stdout, "\t");
	}
}

template<typename T> void patriciaTrieNode<T>::print(int level) {
	if (key != NULL) {
		key->print();
		if (data != NULL) {
			printLevel(level);
			fprintf(stdout, "   address=%d.%d.%d.%d\n", (int) data->bytes[0],
					(int) data->bytes[1], (int) data->bytes[2],
					(int) data->bytes[3]);
		}
	}

	if (left != NULL) {
		printLevel(level);
		fprintf(stdout, "left ");
		left->print(level + 1);
	}

	if (right != NULL) {
		printLevel(level);
		fprintf(stdout, "right ");
		right->print(level + 1);
	}
}

