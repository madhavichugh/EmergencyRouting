#ifndef GRAPH_H
#define GRAPH_H

/*
 * File: graph.h
 * Purpose: Road network graph representation and pathfinding algorithms.
 * DSA Concepts:
 *   - Adjacency List: Space-efficient representation of road vertices and edges.
 *   - Dijkstra's Algorithm: Shortest travel time path using custom Min-Heap.
 *   - Breadth-First Search (BFS): Fewest-hop exploration for nearest base fallback.
 *   - Depth-First Search (DFS): Network connectivity and component analysis.
 * Time Complexity:
 *   - Dijkstra: O((V + E) log V)
 *   - BFS Traversal: O(V + E)
 *   - DFS Traversal: O(V + E)
 */

#include "models.h"
#include "structures.h"
#include <string>
#include <vector>
#include <unordered_map>

// Road network graph covering all hospital and locality nodes.
struct RoadGraph {
    int node_count = 0;
    std::vector<std::string> node_names;
    std::unordered_map<std::string, int> name_to_id;
    std::vector<std::vector<RoadEdge>> adj;

    // Registers a node name and assigns it a unique numeric index.
    int addOrGetNode(const std::string& name) {
        auto it = name_to_id.find(name);
        if (it != name_to_id.end()) {
            return it->second;
        }
        int id = node_count++;
        name_to_id[name] = id;
        node_names.push_back(name);
        adj.push_back(std::vector<RoadEdge>());
        return id;
    }

    // Adds a bidirectional road edge between two named locations.
    void addEdge(const std::string& from, const std::string& to, double dist_km, double travel_min, double traffic_factor) {
        int u = addOrGetNode(from);
        int v = addOrGetNode(to);
        RoadEdge e1{v, dist_km, travel_min, traffic_factor, false};
        RoadEdge e2{u, dist_km, travel_min, traffic_factor, false};
        adj[u].push_back(e1);
        adj[v].push_back(e2);
    }

    // Sets whether a specific road between u and v is blocked.
    bool setRoadBlocked(const std::string& from, const std::string& to, bool blocked) {
        if (name_to_id.find(from) == name_to_id.end() || name_to_id.find(to) == name_to_id.end()) {
            return false;
        }
        int u = name_to_id[from];
        int v = name_to_id[to];
        bool changed = false;
        for (auto& edge : adj[u]) {
            if (edge.to_node == v) {
                edge.blocked = blocked;
                changed = true;
            }
        }
        for (auto& edge : adj[v]) {
            if (edge.to_node == u) {
                edge.blocked = blocked;
                changed = true;
            }
        }
        return changed;
    }

    // Runs Dijkstra from a single source to all nodes using our Min-Heap.
    void runDijkstra(int src, std::vector<double>& dist, std::vector<int>& parent, std::vector<double>& dist_km) {
        const double INF = 1e9;
        dist.assign(node_count, INF);
        parent.assign(node_count, -1);
        dist_km.assign(node_count, 0.0);

        DijkstraHeap heap;
        dist[src] = 0.0;
        heap.insert(src, 0.0);

        while (!heap.isEmpty()) {
            DijkstraNode top = heap.extractTop();
            int u = top.node;
            double d = top.dist;

            if (d > dist[u]) continue;

            for (const auto& edge : adj[u]) {
                if (edge.blocked) continue;
                double weight = edge.travel_min * edge.traffic_factor;
                if (dist[u] + weight < dist[edge.to_node]) {
                    dist[edge.to_node] = dist[u] + weight;
                    parent[edge.to_node] = u;
                    dist_km[edge.to_node] = dist_km[u] + edge.dist_km;
                    heap.insert(edge.to_node, dist[edge.to_node]);
                }
            }
        }
    }

    // Finds the shortest path and travel time between two nodes.
    RouteResult getRoute(int src, int dest) {
        RouteResult res{false, 0.0, 0.0, {}, {}};
        if (src < 0 || src >= node_count || dest < 0 || dest >= node_count) {
            return res;
        }
        std::vector<double> dist;
        std::vector<int> parent;
        std::vector<double> dist_km;
        runDijkstra(src, dist, parent, dist_km);

        if (dist[dest] >= 1e8) {
            return res;
        }

        res.found = true;
        res.total_time = dist[dest];
        res.total_dist = dist_km[dest];

        int curr = dest;
        while (curr != -1) {
            res.path_nodes.push_back(curr);
            res.path_names.push_back(node_names[curr]);
            curr = parent[curr];
        }

        // Reverse to get path from source to destination
        int n = res.path_nodes.size();
        for (int i = 0; i < n / 2; i++) {
            std::swap(res.path_nodes[i], res.path_nodes[n - 1 - i]);
            std::swap(res.path_names[i], res.path_names[n - 1 - i]);
        }
        return res;
    }

    // Uses BFS to find reachable target node with fewest hops (fallback).
    int findNearestByHops(int src, const std::vector<int>& targets) {
        if (src < 0 || src >= node_count) return -1;
        std::unordered_map<int, bool> is_target;
        for (int t : targets) is_target[t] = true;

        std::vector<bool> vis(node_count, false);
        std::vector<int> q;
        int q_head = 0;

        vis[src] = true;
        q.push_back(src);

        while (q_head < (int)q.size()) {
            int u = q[q_head++];
            if (is_target.find(u) != is_target.end()) {
                return u;
            }
            for (const auto& edge : adj[u]) {
                if (edge.blocked) continue;
                if (!vis[edge.to_node]) {
                    vis[edge.to_node] = true;
                    q.push_back(edge.to_node);
                }
            }
        }
        return -1;
    }

    // Recursive DFS helper to visit reachable nodes.
    void dfsHelper(int u, std::vector<bool>& vis) {
        vis[u] = true;
        for (const auto& edge : adj[u]) {
            if (edge.blocked) continue;
            if (!vis[edge.to_node]) {
                dfsHelper(edge.to_node, vis);
            }
        }
    }

    // Uses DFS to count connected components and check if network is fragmented.
    int checkConnectedComponents() {
        std::vector<bool> vis(node_count, false);
        int components = 0;
        for (int i = 0; i < node_count; i++) {
            if (!vis[i]) {
                components++;
                dfsHelper(i, vis);
            }
        }
        return components;
    }
};

#endif
