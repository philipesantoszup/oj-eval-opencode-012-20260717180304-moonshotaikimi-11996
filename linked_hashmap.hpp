/**
 * implement a container like std::linked_hashmap
 */
#ifndef SJTU_LINKEDHASHMAP_HPP
#define SJTU_LINKEDHASHMAP_HPP

// only for std::equal_to<T> and std::hash<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<
	class Key,
	class T,
	class Hash = std::hash<Key>, 
	class Equal = std::equal_to<Key>
> class linked_hashmap {
public:
	typedef pair<const Key, T> value_type;

private:
	// Node structure for doubly-linked list + hash table
	struct Node {
		value_type* data;
		Node* next;      // next in hash bucket
		Node* prev;      // prev in hash bucket
		Node* link_next; // next in insertion order
		Node* link_prev; // prev in insertion order
		
		Node() : data(nullptr), next(nullptr), prev(nullptr), link_next(nullptr), link_prev(nullptr) {}
	};

	Node** buckets;       // hash table buckets
	Node* head;           // dummy head of insertion order list
	Node* tail;           // dummy tail of insertion order list
	size_t bucket_count;
	size_t num_elements;
	Hash hash_func;
	Equal equal_func;
	
	static const size_t DEFAULT_BUCKET_COUNT = 16;
	static constexpr float MAX_LOAD_FACTOR = 0.75f;

	// Hash table helpers
	size_t get_bucket_index(const Key& key) const {
		return hash_func(key) % bucket_count;
	}

	void resize(size_t new_bucket_count) {
		Node** new_buckets = new Node*[new_bucket_count];
		for (size_t i = 0; i < new_bucket_count; ++i) {
			new_buckets[i] = nullptr;
		}

		// Rehash all existing nodes
		for (size_t i = 0; i < bucket_count; ++i) {
			Node* curr = buckets[i];
			while (curr != nullptr) {
				Node* next = curr->next;
				size_t new_idx = hash_func(curr->data->first) % new_bucket_count;
				// Insert at front of new bucket
				curr->next = new_buckets[new_idx];
				curr->prev = nullptr;
				if (new_buckets[new_idx] != nullptr) {
					new_buckets[new_idx]->prev = curr;
				}
				new_buckets[new_idx] = curr;
				curr = next;
			}
		}

		delete[] buckets;
		buckets = new_buckets;
		bucket_count = new_bucket_count;
	}

	void check_load_factor() {
		if (num_elements > bucket_count * MAX_LOAD_FACTOR) {
			resize(bucket_count * 2);
		}
	}

	// Link list helpers
	void link_node(Node* node) {
		// Insert at the end of the linked list (before tail)
		node->link_prev = tail->link_prev;
		node->link_next = tail;
		tail->link_prev->link_next = node;
		tail->link_prev = node;
	}

	void unlink_node(Node* node) {
		if (node->link_prev != nullptr) {
			node->link_prev->link_next = node->link_next;
		}
		if (node->link_next != nullptr) {
			node->link_next->link_prev = node->link_prev;
		}
		node->link_prev = nullptr;
		node->link_next = nullptr;
	}

	void init() {
		bucket_count = DEFAULT_BUCKET_COUNT;
		buckets = new Node*[bucket_count];
		for (size_t i = 0; i < bucket_count; ++i) {
			buckets[i] = nullptr;
		}
		num_elements = 0;

		// Create dummy head and tail for insertion order
		head = new Node();
		tail = new Node();
		head->link_next = tail;
		tail->link_prev = head;
	}

	void clear_internal() {
		// Clear all nodes
		Node* curr = head->link_next;
		while (curr != tail) {
			Node* next = curr->link_next;
			delete curr->data;
			delete curr;
			curr = next;
		}
		delete[] buckets;
		delete head;
		delete tail;
	}

public:
	class const_iterator;
	class iterator {
	private:
		Node* node;
		const linked_hashmap* map;

	public:
		using difference_type = std::ptrdiff_t;
		using value_type = typename linked_hashmap::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::bidirectional_iterator_tag;

		iterator() : node(nullptr), map(nullptr) {}
		iterator(Node* n, const linked_hashmap* m) : node(n), map(m) {}
		iterator(const iterator &other) : node(other.node), map(other.map) {}

		iterator operator++(int) {
			if (node == nullptr || node == map->tail) {
				throw invalid_iterator();
			}
			iterator tmp(*this);
			node = node->link_next;
			return tmp;
		}

		iterator & operator++() {
			if (node == nullptr || node == map->tail) {
				throw invalid_iterator();
			}
			node = node->link_next;
			return *this;
		}

		iterator operator--(int) {
			if (node == nullptr || node == map->head->link_next || (node == map->head && map->num_elements > 0)) {
				throw invalid_iterator();
			}
			if (node == map->head) {
				throw invalid_iterator();
			}
			Node* prev = node->link_prev;
			if (prev == map->head) {
				throw invalid_iterator();
			}
			iterator tmp(*this);
			node = prev;
			return tmp;
		}

		iterator & operator--() {
			if (node == nullptr || node == map->head->link_next || (node == map->head && map->num_elements > 0)) {
				throw invalid_iterator();
			}
			if (node == map->head) {
				throw invalid_iterator();
			}
			Node* prev = node->link_prev;
			if (prev == map->head) {
				throw invalid_iterator();
			}
			node = prev;
			return *this;
		}

		value_type & operator*() const {
			if (node == nullptr || node->data == nullptr) {
				throw invalid_iterator();
			}
			return *(node->data);
		}

		bool operator==(const iterator &rhs) const {
			return node == rhs.node;
		}

		bool operator==(const const_iterator &rhs) const {
			return node == rhs.get_node();
		}

		bool operator!=(const iterator &rhs) const {
			return node != rhs.node;
		}

		bool operator!=(const const_iterator &rhs) const {
			return node != rhs.get_node();
		}

		value_type* operator->() const noexcept {
			return node->data;
		}

		Node* get_node() const { return node; }
		const linked_hashmap* get_map() const { return map; }
		friend class const_iterator;
		friend class linked_hashmap;
	};

	class const_iterator {
	private:
		Node* node;
		const linked_hashmap* map;

	public:
		using difference_type = std::ptrdiff_t;
		using value_type = const typename linked_hashmap::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::bidirectional_iterator_tag;

		const_iterator() : node(nullptr), map(nullptr) {}
		const_iterator(Node* n, const linked_hashmap* m) : node(n), map(m) {}
		const_iterator(const const_iterator &other) : node(other.node), map(other.map) {}
		const_iterator(const iterator &other) : node(other.get_node()), map(other.get_map()) {}

		const_iterator operator++(int) {
			if (node == nullptr || node == map->tail) {
				throw invalid_iterator();
			}
			const_iterator tmp(*this);
			node = node->link_next;
			return tmp;
		}

		const_iterator & operator++() {
			if (node == nullptr || node == map->tail) {
				throw invalid_iterator();
			}
			node = node->link_next;
			return *this;
		}

		const_iterator operator--(int) {
			if (node == nullptr || node == map->head->link_next || (node == map->head && map->num_elements > 0)) {
				throw invalid_iterator();
			}
			if (node == map->head) {
				throw invalid_iterator();
			}
			Node* prev = node->link_prev;
			if (prev == map->head) {
				throw invalid_iterator();
			}
			const_iterator tmp(*this);
			node = prev;
			return tmp;
		}

		const_iterator & operator--() {
			if (node == nullptr || node == map->head->link_next || (node == map->head && map->num_elements > 0)) {
				throw invalid_iterator();
			}
			if (node == map->head) {
				throw invalid_iterator();
			}
			Node* prev = node->link_prev;
			if (prev == map->head) {
				throw invalid_iterator();
			}
			node = prev;
			return *this;
		}

		const value_type & operator*() const {
			if (node == nullptr || node->data == nullptr) {
				throw invalid_iterator();
			}
			return *(node->data);
		}

		bool operator==(const iterator &rhs) const {
			return node == rhs.get_node();
		}

		bool operator==(const const_iterator &rhs) const {
			return node == rhs.node;
		}

		bool operator!=(const iterator &rhs) const {
			return node != rhs.get_node();
		}

		bool operator!=(const const_iterator &rhs) const {
			return node != rhs.node;
		}

		const value_type* operator->() const noexcept {
			return node->data;
		}

		Node* get_node() const { return node; }
		const linked_hashmap* get_map() const { return map; }
		friend class iterator;
		friend class linked_hashmap;
	};

	linked_hashmap() {
		init();
	}

	linked_hashmap(const linked_hashmap &other) : hash_func(other.hash_func), equal_func(other.equal_func) {
		init();
		for (const_iterator it = other.cbegin(); it != other.cend(); ++it) {
			insert(*it);
		}
	}

	linked_hashmap & operator=(const linked_hashmap &other) {
		if (this == &other) {
			return *this;
		}
		clear_internal();
		init();
		hash_func = other.hash_func;
		equal_func = other.equal_func;
		for (const_iterator it = other.cbegin(); it != other.cend(); ++it) {
			insert(*it);
		}
		return *this;
	}

	~linked_hashmap() {
		clear_internal();
	}

	T & at(const Key &key) {
		Node* node = find_node(key);
		if (node == nullptr) {
			throw index_out_of_bound();
		}
		return node->data->second;
	}

	const T & at(const Key &key) const {
		Node* node = find_node(key);
		if (node == nullptr) {
			throw index_out_of_bound();
		}
		return node->data->second;
	}

	T & operator[](const Key &key) {
		Node* node = find_node(key);
		if (node == nullptr) {
			// Insert with default value
			T default_value = T();
			auto result = insert(value_type(key, default_value));
			return result.first->second;
		}
		return node->data->second;
	}

	const T & operator[](const Key &key) const {
		return at(key);
	}

	iterator begin() {
		return iterator(head->link_next, this);
	}

	const_iterator cbegin() const {
		return const_iterator(head->link_next, this);
	}

	iterator end() {
		return iterator(tail, this);
	}

	const_iterator cend() const {
		return const_iterator(tail, this);
	}

	bool empty() const {
		return num_elements == 0;
	}

	size_t size() const {
		return num_elements;
	}

	void clear() {
		clear_internal();
		init();
	}

	pair<iterator, bool> insert(const value_type &value) {
		// Check if key already exists
		Node* existing = find_node(value.first);
		if (existing != nullptr) {
			return pair<iterator, bool>(iterator(existing, this), false);
		}

		check_load_factor();

		// Create new node
		Node* new_node = new Node();
		new_node->data = new value_type(value);

		// Insert into hash table
		size_t bucket_idx = get_bucket_index(value.first);
		new_node->next = buckets[bucket_idx];
		new_node->prev = nullptr;
		if (buckets[bucket_idx] != nullptr) {
			buckets[bucket_idx]->prev = new_node;
		}
		buckets[bucket_idx] = new_node;

		// Insert into linked list (at end)
		link_node(new_node);

		num_elements++;
		return pair<iterator, bool>(iterator(new_node, this), true);
	}

	void erase(iterator pos) {
		if (pos == end() || pos.get_map() != this) {
			throw invalid_iterator();
		}
		Node* node = pos.get_node();
		if (node == nullptr || node == head || node == tail) {
			throw invalid_iterator();
		}

		// Remove from hash table
		size_t bucket_idx = get_bucket_index(node->data->first);
		if (node->prev != nullptr) {
			node->prev->next = node->next;
		} else {
			buckets[bucket_idx] = node->next;
		}
		if (node->next != nullptr) {
			node->next->prev = node->prev;
		}

		// Remove from linked list
		unlink_node(node);

		// Delete node
		delete node->data;
		delete node;

		num_elements--;
	}

	size_t count(const Key &key) const {
		return find_node(key) != nullptr ? 1 : 0;
	}

	iterator find(const Key &key) {
		Node* node = find_node(key);
		if (node == nullptr) {
			return end();
		}
		return iterator(node, this);
	}

	const_iterator find(const Key &key) const {
		Node* node = find_node(key);
		if (node == nullptr) {
			return cend();
		}
		return const_iterator(node, this);
	}

private:
	Node* find_node(const Key &key) const {
		size_t bucket_idx = get_bucket_index(key);
		Node* curr = buckets[bucket_idx];
		while (curr != nullptr) {
			if (equal_func(curr->data->first, key)) {
				return curr;
			}
			curr = curr->next;
		}
		return nullptr;
	}
};

}

#endif
