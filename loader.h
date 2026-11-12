#ifndef LOADER_H
#define LOADER_H

/*
 * File: loader.h
 * Purpose: Loads CSV datasets into memory using vectors and hash maps.
 * DSA Concepts:
 *   - std::vector: Dynamic arrays for contiguous sequential data storage.
 *   - std::unordered_map: Hash table for O(1) average lookup by entity ID.
 * Time Complexity:
 *   - Loading CSV: O(N) where N is number of rows in the CSV files.
 *   - Lookups in unordered_map: O(1) average time.
 */

#include "models.h"
#include "graph.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>

// Splits a single CSV row by commas into individual column tokens.
inline std::vector<std::string> parseCsvRow(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token = "";
    for (char c : line) {
        if (c == ',') {
            tokens.push_back(token);
            token = "";
        } else if (c != '\r') {
            token += c;
        }
    }
    tokens.push_back(token);
    return tokens;
}

// Reads data/hospitals.csv into a list and a fast-lookup hash map.
inline bool loadHospitals(const std::string& path, std::vector<Hospital>& list, std::unordered_map<int, Hospital>& map) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line.rfind("id,", 0) == 0) continue;
        std::vector<std::string> t = parseCsvRow(line);
        if (t.size() >= 11) {
            Hospital h;
            h.id = std::stoi(t[0]);
            h.name = t[1];
            h.area = t[2];
            h.lat = std::stod(t[3]);
            h.lng = std::stod(t[4]);
            h.total_beds = std::stoi(t[5]);
            h.free_beds = std::stoi(t[6]);
            h.icu_total = std::stoi(t[7]);
            h.icu_free = std::stoi(t[8]);
            h.specialties = t[9];
            h.has_emergency = std::stoi(t[10]);
            list.push_back(h);
            map[h.id] = h;
        }
    }
    return true;
}

// Reads data/drivers.csv into a list and a fast-lookup hash map.
inline bool loadDrivers(const std::string& path, std::vector<Driver>& list, std::unordered_map<int, Driver>& map) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line.rfind("id,", 0) == 0) continue;
        std::vector<std::string> t = parseCsvRow(line);
        if (t.size() >= 5) {
            Driver d;
            d.id = std::stoi(t[0]);
            d.name = t[1];
            d.phone = t[2];
            d.exp_years = std::stoi(t[3]);
            d.shift = t[4];
            list.push_back(d);
            map[d.id] = d;
        }
    }
    return true;
}

// Reads data/ambulances.csv into a list and a fast-lookup hash map.
inline bool loadAmbulances(const std::string& path, std::vector<Ambulance>& list, std::unordered_map<int, Ambulance>& map) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line.rfind("id,", 0) == 0) continue;
        std::vector<std::string> t = parseCsvRow(line);
        if (t.size() >= 8) {
            Ambulance a;
            a.id = std::stoi(t[0]);
            a.vehicle_no = t[1];
            a.type = t[2];
            a.base_hospital_id = std::stoi(t[3]);
            a.lat = std::stod(t[4]);
            a.lng = std::stod(t[5]);
            a.status = t[6];
            a.driver_id = std::stoi(t[7]);
            list.push_back(a);
            map[a.id] = a;
        }
    }
    return true;
}

// Reads data/roads.csv to populate the graph adjacency list.
inline bool loadRoads(const std::string& path, RoadGraph& graph) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line.rfind("from,", 0) == 0) continue;
        std::vector<std::string> t = parseCsvRow(line);
        if (t.size() >= 5) {
            std::string from = t[0];
            std::string to = t[1];
            double dist_km = std::stod(t[2]);
            double travel_min = std::stod(t[3]);
            double traffic = std::stod(t[4]);
            graph.addEdge(from, to, dist_km, travel_min, traffic);
        }
    }
    return true;
}

#endif
