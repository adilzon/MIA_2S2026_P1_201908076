#ifndef MOUNT_H
#define MOUNT_H

#include <string>
#include <vector>
#include <bits/stdc++.h>
#include "../lib/shared.h"
#include "../lib/structs.h"
#include "../lib/disco.h"

using namespace std;

class Mount {
    public:
    Mount();

    typedef struct _MD
    {
        char path[150] = {0};
        char status = '0';
        char letter = 0;
        int count = 0;
        Structs::Partition mpartitions[10];
    } MountedDisc;

    MountedDisc mounted[26];

    string carnet = "76";

    void mount(vector<string> context);

    void unmount(vector<string> context);

    void mount(string p, string n);

    void unmount(string id);

    void listmount();

    Structs::Partition getmount(string id, string *p);

    private:
    Disk dsk;
    Shared shared;
};
#endif