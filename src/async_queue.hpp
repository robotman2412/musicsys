
#pragma once

#include <queue>
#include <mutex>
#include <thread>

template<typename T> class ASQueue {
	protected:
		// Internal storage type.
		template<typename X>
		struct Link {
			Link<X> *prev;
			Link<X> *next;
			X data;
		};
		// First link in list.
		Link<T> *first;
		// Last in list.
		Link<T> *last;
		// Amount of links.
		volatile size_t linkCount;
		// Mutex for sending/receiving.
		std::mutex mtx;
		
	public:
		// Create an empty queue.
		ASQueue() {
			linkCount = 0;
			first = (Link<T> *) NULL;
			last  = (Link<T> *) NULL;
		}
		// You are not allowed to copy a queue.
		ASQueue(ASQueue &) = delete;
		// You are not allowed to move a queue.
		ASQueue(ASQueue &&) = delete;
		// You are not allowed to assign a queue.
		void operator=(ASQueue &) = delete;
		void operator=(ASQueue &&) = delete;
		// Clear out the queue.
		~ASQueue() {
			clear();
		}
		
		// Send an element to the queue.
		void send(T elem) {
			// Acquire mutex.
			std::lock_guard lock(mtx);
			
			if (!linkCount) {
				// Initialise empty queue.
				first = new Link<T>();
				first->prev = NULL;
				first->next = NULL;
				first->data = elem;
				last = first;
				
			} else {
				// Add to non-empty queue.
				Link<T> *next = new Link<T>();
				next->prev = last;
				next->next = NULL;
				next->data = elem;
				last->next = next;
				last = next;
			}
			
			linkCount ++;
		}
		
		// Remove all elements from the queue.
		void clear() {
			// Acquire mutex.
			std::lock_guard lock(mtx);
			
			// Iterate over all links.
			Link<T> *cur = first;
			while (cur) {
				// Delete each link.
				Link<T> *next = cur->next;
				delete cur;
				cur = next;
			}
			
			// Set size to 0.
			linkCount = 0;
			first = NULL;
			last  = NULL;
		}
		
		// Get size of the queue.
		size_t size() {
			// Acquire mutex.
			std::lock_guard lock(mtx);
			
			// Return internal size.
			return linkCount;
		}
		
		// Wait to receive an element.
		T waitRecv() {
			// Use tryRecv constantly.
			T item;
			while (!tryRecv(item));
			return item;
		}
		
		// Try to receive an element.
		bool tryRecv(T &out) {
			// Acquire mutex.
			std::lock_guard lock(mtx);
			
			// Check size.
			if (!linkCount) {
				return false;
			}
			
			// Consume first item.
			out = first->data;
			
			// Unlink first.
			Link<T> *mem = first;
			if (first->next) {
				first->next->prev = first;
				first = first->next;
			} else {
				last  = NULL;
				first = NULL;
			}
			delete mem;
			
			// Decrement size.
			linkCount --;
			
			return true;
		}
};
