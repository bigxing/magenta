// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <fstream>
#include <string>
#include <vector>

//#include <mdi/mdi.h>

#include "node.h"
#include "parser.h"
#include "tokens.h"

#define VERSION_MAJOR   1
#define VERSION_MINOR   0

static bool run(std::vector<std::string>& in_paths, const char* out_path,
               const char* header_path, const char* header_prefix,
               bool header_uppercase, bool debug) {
    std::ofstream out_file;
    out_file.open(out_path, std::ofstream::binary);
    if (!out_file.good()) {
        fprintf(stderr, "error: unable to open %s\n", out_path);
        return false;
    }

    // root of our tree
    Node    root(mdi_root_id, "");

    // iterate through our input files
    for (auto iter = in_paths.begin(); iter != in_paths.end(); iter++) {
        const char* in_path = iter->c_str();
        if (!process_file(nullptr, in_path, root)) {
            return false;
        }
     }

    root.compute_node_length();

    // write empty header first
    mdi_header_t header;
    memset(&header, 0, sizeof(header));
    out_file.write((const char *)&header, sizeof(header));

    // write the nodes
    root.serialize(out_file);

    // back up and fill in the header
    header.magic = MDI_MAGIC;
    header.length = out_file.tellp();
    header.version_major = VERSION_MAJOR;
    header.version_minor = VERSION_MINOR;
    out_file.seekp(0);
    out_file.write((const char *)&header, sizeof(header));

    if (header_path) {
        std::ofstream header_file;
        header_file.open(header_path, std::ofstream::binary);
        if (!header_file.good()) {
            fprintf(stderr, "error: unable to open %s\n", header_path);
            return false;
        }
        print_header_file(header_file, header_prefix, header_uppercase);
    }

    if (debug) {
        // print parse tree
        root.print();
    }

    return true;
}

static void usage() {
    fprintf(stderr, "uasge: mdigen <input files> -o <output file> -h <C header output file>\n"
                    "Additional options:\n"
                    "\t-d - debug output, prints parse tree\n"
                    "\t-p <prefix> - prefix for symbols in C header file\n"
                    "\t-u - convert symbols in C header file to upper case\n");
}
int main(int argc, char* argv[]) {
    std::vector<std::string> in_paths;
    const char* out_path = nullptr;
    const char* header_path = nullptr;
    const char* header_prefix = "";
    bool header_uppercase = false;
    bool debug = false;

    argc--;
    argv++;

    if (argc == 0) {
        usage();
        return -1;
    }
    while (argc > 0) {
        const char *arg = argv[0];
        if (arg[0] != '-') {
            in_paths.push_back(arg);
        } else if (!strcmp(arg, "-o") && argc >= 2) {
            out_path = argv[1];
            argc--;
            argv++;
        } else if (!strcmp(arg, "-h") && argc >= 2) {
            header_path = argv[1];
            argc--;
            argv++;
        } else if (!strcmp(arg, "-p") && argc >= 2) {
            header_prefix = argv[1];
            argc--;
            argv++;
        } else if (!strcmp(arg, "-u")) {
            header_uppercase = true;
        } else if (!strcmp(arg, "-d")) {
            debug = true;
        } else {
            fprintf(stderr, "unknown argument \"%s\"\n", arg);
             usage();
            return -1;
        }
        argc--;
        argv++;
    }

    if (in_paths.size() == 0) {
        fprintf(stderr, "no input files specified\n");
        usage();
        return -1;
    }
    if (!out_path) {
        fprintf(stderr, "no output file specified\n");
        usage();
        return -1;
    }

    bool result = run(in_paths, out_path, header_path, header_prefix, header_uppercase, debug);
    return (result ? 0 : -1);
}
