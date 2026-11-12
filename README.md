# Emergency Response and Ambulance Routing System

## Build and Run
```bash
g++ -std=c++17 main.cpp dispatcher.cpp -o app
./app          # runs demo and starts web server at http://localhost:8080
./app --demo   # runs console DSA demo scenarios directly
```

## DSA Concept to File and Function Mapping

| Data Structure / Algorithm | File | Function / Struct | Purpose |
| :--- | :--- | :--- | :--- |
| **Array (`std::vector`)** | `models.h`, `loader.h` | `hospitals`, `ambulances` | Contiguous fleet and entity storage |
| **HashMap (`std::unordered_map`)** | `loader.h`, `graph.h` | `name_to_id`, `hospital_map` | O(1) average lookup by ID and name |
| **Queue (Custom FIFO)** | `structures.h` | `PatientQueue::enqueue/dequeue` | FIFO handling for Medium/Low patients |
| **MinHeap (Custom Priority Queue)**| `structures.h` | `PatientHeap::insert/extractTop` | Priority triage for Critical/High cases |
| **MinHeap (Dijkstra)** | `structures.h` | `DijkstraHeap::insert/extractTop`| O(log V) vertex extraction in Dijkstra |
| **Stack (Custom LIFO)** | `structures.h` | `DispatchStack::push/pop` | Dispatch history and Undo rollback |
| **Graph (Adjacency List)** | `graph.h` | `RoadGraph::adj`, `addEdge` | Delhi NCR road vertices and edge weights |
| **Dijkstra's Algorithm** | `graph.h` | `RoadGraph::runDijkstra` | Shortest travel time routing with parent array |
| **Breadth-First Search (BFS)** | `graph.h` | `RoadGraph::findNearestByHops` | Fallback search for closest base by hops |
| **Depth-First Search (DFS)** | `graph.h` | `RoadGraph::checkConnectedComponents`| Road-blockage connectivity analysis |
| **Merge Sort (Custom)** | `sorting.h` | `mergeSortRange`, `mergeSubarrays`| Top-3 alternative hospital ranking |
| **Binary Search Tree (BST)** | `structures.h` | `HospitalBST::insert` | Hospital indexing by free bed capacity |
