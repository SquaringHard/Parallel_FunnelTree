#include "ft.h"
#include <string>
#include <stdexcept>    // runtime_error
#include <fstream>      // ifstream, ofstream
#include <vector>
#include <cmath>        // INFINITY, fabs
#include <algorithm>    // equal
#include <iomanip>      // fixed, setprecision
#include <limits>       // numeric_limits
#include <filesystem>
#include <iostream>     // cout
#include <chrono>
#include <numeric>      // reduce

bool compareLength(const char *filename, const vector<Funnel> &list, const indexType startIndex, const size_t n = 0) {
    const string realFilename = string(filename) + "_s=" + to_string(startIndex);
    ifstream file("expected/" + realFilename + ".txt");
    if (!file.is_open()) throw runtime_error("expected/" + realFilename + ".txt not found");

    vector<double> expectedLengths;
    for (string i; file >> i;) {
        if (i == "inf") expectedLengths.push_back(INFINITY);
        else expectedLengths.push_back(stod(i));
    }

    vector<double> lengths(expectedLengths.size(), INFINITY);
    lengths[startIndex] = 0;
    for (const Funnel &f : list) if (lengths[f.p] > f.sp2) lengths[f.p] = f.sp2;
    for (double &i : lengths) i = sqrt(i);

    const bool result = equal(expectedLengths.begin(), expectedLengths.end(), lengths.begin(),
                              [](const double a, const double b) { return fabs(a - b) < 1e-5; });
    if (!result) {
        filesystem::create_directory("output");
        ofstream file("output/" + realFilename + " (" + to_string(n) + ").txt");
        file << fixed << setprecision(numeric_limits<double>::max_digits10);
        for (const double i : lengths) file << i << '\n';
    }

    return result;
}

void run(const char *filename, const indexType startIndex = 0) {
    cout << "File \"" << filename << "\": ";
    const TriangleMesh mesh = getMesh(filename);

    const auto start = chrono::high_resolution_clock::now();
    const vector<Funnel> list = FunnelTree(mesh, startIndex);
    const auto end = chrono::high_resolution_clock::now();

    cout << "Funnel tree with root " << startIndex << " initialized with " << list.size() << " nodes in "
         << fixed << setprecision(3) << chrono::duration_cast<chrono::microseconds>(end - start).count() / 1000.0 << " ms.";
    #ifdef LENGTH_COMPARE
        cout << " (" << (compareLength(filename, list, startIndex) ? "" : "NOT ") << "passed)";
    #endif
    cout << '\n';
}

void repeat(const char *filename, const indexType startIndex = 0, const short n = 100) {
    cout << "File \"" << filename << "\" ran " << n << " times. Avg: ";
    const TriangleMesh mesh = getMesh(filename);

    vector<chrono::nanoseconds> durations;
    #ifdef LENGTH_COMPARE
        short passed = 0;
    #endif
    for (short i = 0; i < n; i++) {
        const auto start = chrono::high_resolution_clock::now();
        const vector<Funnel> list = FunnelTree(mesh, startIndex);
        const auto end = chrono::high_resolution_clock::now();

        durations.emplace_back(end - start);
        #ifdef LENGTH_COMPARE
            passed += compareLength(filename, list, startIndex, i);
        #endif
    }

    const chrono::nanoseconds avg = reduce(durations.begin(), durations.end()) / n;
    
    chrono::nanoseconds error(0);
    for (const chrono::nanoseconds i : durations) error += i < avg ? avg - i : i - avg;
    error /= n;
    error++;    // system error

    cout << fixed << setprecision(3) << chrono::duration_cast<chrono::microseconds>(avg).count() / 1000.0 << " +/- "
                                     << chrono::duration_cast<chrono::microseconds>(error).count() / 1000.0 << " ms";
    #ifdef LENGTH_COMPARE
        cout << " (" << passed << " passed)";
    #endif
    cout << '\n';
}

int main(int argc, const char *argv[]) {
    const char *allfiles[] = {"main", "cube1.geom", "cube2.geom", "cube3.geom", "cube4.geom",
                              "sphere1.geom", "sphere2.geom", "sphere3.geom", "sphere4.geom",
                              "spiral1.geom", "spiral2.geom", "J17.geom", "box1.geom",
                              "out0.geom", "out1.geom", "out9.geom", "out135.geom",
                              "pyramid1.geom", "pyramid2.geom", "pyramid3.geom"},
               **files = argv;
    if (argc <= 1) { files = allfiles; argc = sizeof(allfiles) / sizeof(*allfiles); }
    for (int i = 1; i < argc; i++) {
        run(files[i]);
        // repeat(files[i]);
    }
}