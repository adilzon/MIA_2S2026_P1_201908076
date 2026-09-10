#include "../lib/mount.h"

#include <iostream>
#include <stdlib.h>
#include <string>
#include <locale>
#include <algorithm>
#include <cctype>

using namespace std;

Mount::Mount() {
    for (int i = 0; i < 26; i++) {
        mounted[i].status = '0';
        memset(mounted[i].path, 0, sizeof(mounted[i].path));
        mounted[i].letter = 0;
        mounted[i].count = 0;
        for (int j = 0; j < 10; j++) {
            mounted[i].mpartitions[j].part_status = '0';
            mounted[i].mpartitions[j].part_correlative = -1;
            memset(mounted[i].mpartitions[j].part_id, 0, sizeof(mounted[i].mpartitions[j].part_id));
        }
    }
}

void Mount::mount(vector<string> context) {
    if (context.empty()) {
        listmount();
        return;
    }
    string path = "";
    string name = "";

    for (auto current : context) {
        string id = shared.lower(current.substr(0, current.find('=')));
        current.erase(0, id.length() + 1);
        if (current.size() >= 2 && current.front() == '"' && current.back() == '"') {
            current = current.substr(1, current.length() - 2);
        } else if (current.size() >= 2 && current.front() == '\'' && current.back() == '\'') {
            current = current.substr(1, current.length() - 2);
        }

        if (shared.compare(id, "name")) {
            name = current;
        } else if (shared.compare(id, "path")) {
            path = current;
        }
    }

    if (path.empty() || name.empty()) {
        shared.handler("MOUNT", "parametros obligatorios faltantes");
        return;
    }
    mount(path, name);
}

void Mount::mount(string p, string n) {
    try {
        FILE *validate = fopen(p.c_str(), "rb");
        if (validate == NULL) {
            shared.handler("MOUNT", "disco no existente");
            return;
        }

        Structs::MBR disk;
        rewind(validate);
        fread(&disk, sizeof(Structs::MBR), 1, validate);
        fclose(validate);

        Structs::Partition partition;
        try {
            partition = dsk.findby(disk, n, p);
        } catch (exception &e) {
            shared.handler("MOUNT", e.what());
            return;
        }

        if (partition.part_type == 'E' || partition.part_type == 'L') {
            shared.handler("MOUNT", "solo se permiten particiones primarias");
            return;
        }

        int diskIndex = -1;
        for (int i = 0; i < 26; i++) {
            if (mounted[i].status == '1' && string(mounted[i].path) == p) {
                diskIndex = i;
                for (int j = 0; j < 10; j++) {
                    if (mounted[i].mpartitions[j].part_status == '1' && shared.compare(mounted[i].mpartitions[j].part_name, n)) {
                        shared.handler("MOUNT", "la particion ya esta montada");
                        return;
                    }
                }
                break;
            }
        }

        if (diskIndex == -1) {
            for (int i = 0; i < 26; i++) {
                if (mounted[i].status == '0') {
                    diskIndex = i;
                    mounted[i].status = '1';
                    strncpy(mounted[i].path, p.c_str(), sizeof(mounted[i].path) - 1);
                    mounted[i].letter = 'A' + i;
                    mounted[i].count = 0;
                    break;
                }
            }
        }

        if (diskIndex == -1) {
            shared.handler("MOUNT", "no hay slots de discos disponibles");
            return;
        }

        int partIndex = -1;
        for (int j = 0; j < 10; j++) {
            if (mounted[diskIndex].mpartitions[j].part_status == '0') {
                partIndex = j;
                break;
            }
        }

        if (partIndex == -1) {
            shared.handler("MOUNT", "no hay slots de particion disponibles en este disco");
            return;
        }

        mounted[diskIndex].count++;
        int partNumber = mounted[diskIndex].count;

        partition.part_status = '1';
        partition.part_correlative = partNumber;

        string id_str = carnet + to_string(partNumber) + mounted[diskIndex].letter;
        memset(partition.part_id, 0, sizeof(partition.part_id));
        strncpy(partition.part_id, id_str.c_str(), sizeof(partition.part_id));

        mounted[diskIndex].mpartitions[partIndex] = partition;

        shared.response("MOUNT", "se ha realizado correctamente el mount -id=" + id_str);
    }
    catch (exception &e) {
        shared.handler("MOUNT", e.what());
        return;
    }
}

void Mount::unmount(vector<string> context) {
    vector<string> required = {"id"};
    string id_;

    for (int i = 0; i < context.size(); i++) {
        string current = context.at(i);
        string id = current.substr(0, current.find("="));
        current.erase(0, id.length() + 1);
        if (current.size() >= 2 && current.front() == '"' && current.back() == '"') {
            current = current.substr(1, current.length() - 2);
        }

        if (shared.compare(id, "id")) {
            auto itr = find(required.begin(), required.end(), id);
            if (itr != required.end()) {
                required.erase(itr);
            }
            id_ = current;
        }
    }
    if (required.size() != 0) {
        shared.handler("UNMOUNT", "faltan parametros obligatorios");
        return;
    }
    unmount(id_);
}

void Mount::unmount(string id) {
    try {
        if (id.length() < 4 || id.substr(0, 2) != carnet) {
            throw runtime_error("el primer identificador no es valido");
        }
        char letter = toupper(id.back());
        int correlative = stoi(id.substr(2, id.length() - 3));

        for (int i = 0; i < 26; i++) {
            if (mounted[i].status == '1' && toupper(mounted[i].letter) == letter) {
                for (int j = 0; j < 10; j++) {
                    if (mounted[i].mpartitions[j].part_status == '1' && mounted[i].mpartitions[j].part_correlative == correlative) {
                        mounted[i].mpartitions[j].part_status = '0';
                        memset(mounted[i].mpartitions[j].part_id, 0, sizeof(mounted[i].mpartitions[j].part_id));
                        shared.response("UNMOUNT", "se ha realizado correctamente el unmount -id=" + id);
                        return;
                    }
                }
            }
        }
        throw runtime_error("id no existente, no se desmonto nada");
    }
    catch (invalid_argument &e) {
        shared.handler("UNMOUNT", "identificador de disco incorrecto, debe ser entero");
        return;
    }
    catch (exception &e) {
        shared.handler("UNMOUNT", e.what());
        return;
    }
}

Structs::Partition Mount::getmount(string id, string *p) {
    if (id.length() < 4 || id.substr(0, 2) != carnet) {
        throw runtime_error("el primer identificador no es valido");
    }

    char letter = toupper(id.back());
    int correlative = stoi(id.substr(2, id.length() - 3));

    for (int i = 0; i < 26; i++) {
        if (mounted[i].status == '1' && toupper(mounted[i].letter) == letter) {
            for (int j = 0; j < 10; j++) {
                if (mounted[i].mpartitions[j].part_status == '1' && mounted[i].mpartitions[j].part_correlative == correlative) {
                    if (p != nullptr) {
                        *p = string(mounted[i].path);
                    }
                    return mounted[i].mpartitions[j];
                }
            }
        }
    }
    throw runtime_error("particion no existente o no montada");
}

void Mount::listmount() {
    bool found = false;
    for (int i = 0; i < 26; i++) {
        if (mounted[i].status == '1') {
            for (int j = 0; j < 10; j++) {
                if (mounted[i].mpartitions[j].part_status == '1') {
                    string id_str = carnet + to_string(mounted[i].mpartitions[j].part_correlative) + mounted[i].letter;
                    cout << id_str << ", " << mounted[i].mpartitions[j].part_name << endl;
                    found = true;
                }
            }
        }
    }
    if (!found) {
        cout << "No hay particiones montadas." << endl;
    }
}

