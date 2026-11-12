#ifndef SORTING_H
#define SORTING_H

/*
 * File: sorting.h
 * Purpose: Custom hand-written Merge Sort to rank candidate hospitals.
 * DSA Concept: Divide and Conquer sorting (Merge Sort).
 * Time Complexity:
 *   - Best, Average, Worst Case: O(n log n)
 *   - Space Complexity: O(n) auxiliary space for merging
 */

#include "models.h"
#include <vector>

// Compares two candidate hospitals: primary is travel time (shorter is better),
// secondary is available beds (more is better).
inline bool candidateIsBetter(const CandidateHospital& a, const CandidateHospital& b) {
    if (a.travel_time != b.travel_time) {
        return a.travel_time < b.travel_time;
    }
    return a.free_beds > b.free_beds;
}

// Merges two sorted subarrays arr[left..mid] and arr[mid+1..right].
inline void mergeSubarrays(std::vector<CandidateHospital>& arr, int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    std::vector<CandidateHospital> L(n1);
    std::vector<CandidateHospital> R(n2);

    for (int i = 0; i < n1; i++) {
        L[i] = arr[left + i];
    }
    for (int j = 0; j < n2; j++) {
        R[j] = arr[mid + 1 + j];
    }

    int i = 0;
    int j = 0;
    int k = left;

    while (i < n1 && j < n2) {
        if (candidateIsBetter(L[i], R[j])) {
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
    }

    while (i < n1) {
        arr[k++] = L[i++];
    }
    while (j < n2) {
        arr[k++] = R[j++];
    }
}

// Recursive function that divides array into halves and merges them.
inline void mergeSortRange(std::vector<CandidateHospital>& arr, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSortRange(arr, left, mid);
        mergeSortRange(arr, mid + 1, right);
        mergeSubarrays(arr, left, mid, right);
    }
}

// Public interface to sort candidate hospitals using our Merge Sort.
inline void sortCandidateHospitals(std::vector<CandidateHospital>& arr) {
    if (arr.size() > 1) {
        mergeSortRange(arr, 0, (int)arr.size() - 1);
    }
}

#endif
