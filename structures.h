#ifndef STRUCTURES_H
#define STRUCTURES_H

/*
 * File: structures.h
 * Purpose: Custom hand-written Queue, Min-Heap, Stack, and BST structures.
 * DSA Concepts:
 *   - Circular Queue: FIFO processing for normal severity patients.
 *   - Binary Min-Heap: Priority queue for critical patients and Dijkstra.
 *   - Array-based Stack: LIFO history tracking with undo capability.
 *   - Binary Search Tree (BST): Bed capacity indexing.
 * Time Complexity:
 *   - Queue enqueue/dequeue: O(1)
 *   - Heap insert/extract: O(log n)
 *   - Stack push/pop: O(1)
 *   - BST insert/search: O(log n) average, O(n) worst case
 */

#include "models.h"
#include <vector>

// FIFO queue for Medium and Low severity emergency requests.
struct PatientQueue {
    Patient items[200];
    int head = 0;
    int tail = 0;
    int count = 0;

    // Adds a patient to the back of the queue.
    void enqueue(Patient p) {
        if (count < 200) {
            items[tail] = p;
            tail = (tail + 1) % 200;
            count++;
        }
    }

    // Removes and returns the patient at the front of the queue.
    Patient dequeue() {
        Patient emptyPatient{};
        if (count == 0) return emptyPatient;
        Patient p = items[head];
        head = (head + 1) % 200;
        count--;
        return p;
    }

    // Returns the front patient without removing them.
    Patient front() {
        Patient emptyPatient{};
        if (count == 0) return emptyPatient;
        return items[head];
    }

    // Checks if the queue has zero items.
    bool isEmpty() {
        return count == 0;
    }

    // Returns the number of patients waiting in the queue.
    int size() {
        return count;
    }
};

// Compares two patients to see if p1 has higher priority than p2.
inline bool isPatientHigherPriority(const Patient& p1, const Patient& p2) {
    if (p1.severity_rank != p2.severity_rank) {
        return p1.severity_rank < p2.severity_rank;
    }
    return p1.arrival_order < p2.arrival_order;
}

// Min-Heap priority queue for Critical and High severity patients.
struct PatientHeap {
    std::vector<Patient> data;

    // Swaps elements up the tree to restore min-heap property.
    void heapifyUp(int idx) {
        while (idx > 0) {
            int p = (idx - 1) / 2;
            if (isPatientHigherPriority(data[idx], data[p])) {
                Patient temp = data[idx];
                data[idx] = data[p];
                data[p] = temp;
                idx = p;
            } else {
                break;
            }
        }
    }

    // Swaps elements down the tree to restore min-heap property.
    void heapifyDown(int idx) {
        int n = data.size();
        while (true) {
            int smallest = idx;
            int left = 2 * idx + 1;
            int right = 2 * idx + 2;

            if (left < n && isPatientHigherPriority(data[left], data[smallest])) {
                smallest = left;
            }
            if (right < n && isPatientHigherPriority(data[right], data[smallest])) {
                smallest = right;
            }
            if (smallest != idx) {
                Patient temp = data[idx];
                data[idx] = data[smallest];
                data[smallest] = temp;
                idx = smallest;
            } else {
                break;
            }
        }
    }

    // Inserts a patient into the priority queue.
    void insert(Patient p) {
        data.push_back(p);
        heapifyUp(data.size() - 1);
    }

    // Extracts and returns the highest priority patient.
    Patient extractTop() {
        Patient emptyPatient{};
        if (data.empty()) return emptyPatient;
        Patient top = data[0];
        data[0] = data.back();
        data.pop_back();
        if (!data.empty()) {
            heapifyDown(0);
        }
        return top;
    }

    // Returns the top patient without removing them.
    Patient peek() {
        Patient emptyPatient{};
        if (data.empty()) return emptyPatient;
        return data[0];
    }

    // Checks if the priority queue is empty.
    bool isEmpty() {
        return data.empty();
    }

    // Returns the count of patients in the heap.
    int size() {
        return data.size();
    }
};

// Key-value item stored inside the Dijkstra priority queue.
struct DijkstraNode {
    int node;
    double dist;
};

// Min-Heap used inside Dijkstra algorithm for selecting next closest node.
struct DijkstraHeap {
    std::vector<DijkstraNode> data;

    // Moves the element up to keep smallest distance at root.
    void heapifyUp(int idx) {
        while (idx > 0) {
            int p = (idx - 1) / 2;
            if (data[idx].dist < data[p].dist) {
                DijkstraNode temp = data[idx];
                data[idx] = data[p];
                data[p] = temp;
                idx = p;
            } else {
                break;
            }
        }
    }

    // Moves the element down to maintain smallest distance at root.
    void heapifyDown(int idx) {
        int n = data.size();
        while (true) {
            int smallest = idx;
            int left = 2 * idx + 1;
            int right = 2 * idx + 2;

            if (left < n && data[left].dist < data[smallest].dist) {
                smallest = left;
            }
            if (right < n && data[right].dist < data[smallest].dist) {
                smallest = right;
            }
            if (smallest != idx) {
                DijkstraNode temp = data[idx];
                data[idx] = data[smallest];
                data[smallest] = temp;
                idx = smallest;
            } else {
                break;
            }
        }
    }

    // Adds a node with its distance to the heap.
    void insert(int node, double dist) {
        DijkstraNode dn;
        dn.node = node;
        dn.dist = dist;
        data.push_back(dn);
        heapifyUp(data.size() - 1);
    }

    // Removes and returns the node with minimum distance.
    DijkstraNode extractTop() {
        DijkstraNode emptyNode{-1, -1.0};
        if (data.empty()) return emptyNode;
        DijkstraNode top = data[0];
        data[0] = data.back();
        data.pop_back();
        if (!data.empty()) {
            heapifyDown(0);
        }
        return top;
    }

    // Checks if the heap has no items.
    bool isEmpty() {
        return data.empty();
    }
};

// LIFO stack tracking dispatch operations for rollback / undo.
struct DispatchStack {
    std::vector<DispatchRecord> data;

    // Pushes a completed dispatch record onto the stack.
    void push(DispatchRecord r) {
        data.push_back(r);
    }

    // Pops and returns the most recent dispatch record.
    DispatchRecord pop() {
        DispatchRecord emptyRecord{};
        if (data.empty()) return emptyRecord;
        DispatchRecord top = data.back();
        data.pop_back();
        return top;
    }

    // Peeks at the most recent dispatch record without popping.
    DispatchRecord peek() {
        DispatchRecord emptyRecord{};
        if (data.empty()) return emptyRecord;
        return data.back();
    }

    // Checks if the dispatch stack is empty.
    bool isEmpty() {
        return data.empty();
    }

    // Returns the total number of recorded dispatches.
    int size() {
        return data.size();
    }
};

// Node in the Binary Search Tree ordered by free beds.
struct BSTNode {
    int hospital_id;
    int free_beds;
    BSTNode* left = nullptr;
    BSTNode* right = nullptr;
};

// Binary Search Tree to organize hospitals by available bed count.
struct HospitalBST {
    BSTNode* root = nullptr;

    // Recursive helper to insert a node by bed count.
    BSTNode* insertHelper(BSTNode* node, int hid, int beds) {
        if (!node) {
            BSTNode* n = new BSTNode();
            n->hospital_id = hid;
            n->free_beds = beds;
            return n;
        }
        if (beds < node->free_beds) {
            node->left = insertHelper(node->left, hid, beds);
        } else {
            node->right = insertHelper(node->right, hid, beds);
        }
        return node;
    }

    // Inserts a hospital into the BST.
    void insert(int hid, int beds) {
        root = insertHelper(root, hid, beds);
    }

    // Recursive helper to free all allocated tree nodes.
    void freeTree(BSTNode* node) {
        if (!node) return;
        freeTree(node->left);
        freeTree(node->right);
        delete node;
    }

    // Clears the entire BST.
    void clear() {
        freeTree(root);
        root = nullptr;
    }
};

#endif
