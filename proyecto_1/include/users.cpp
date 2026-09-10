#include "../lib/users.h"

#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include "string"

using namespace std;

Users::Users() {
}

int Users::locateUsersTxtInode(FILE *file, Structs::Superblock spr) {
    Structs::Inodes rootInode;
    fseek(file, spr.s_inode_start, SEEK_SET);
    fread(&rootInode, sizeof(Structs::Inodes), 1, file);

    for (int i = 0; i < 12; i++) {
        if (rootInode.i_block[i] != -1) {
            Structs::Folderblock fb;
            fseek(file, spr.s_block_start + rootInode.i_block[i] * sizeof(Structs::Folderblock), SEEK_SET);
            fread(&fb, sizeof(Structs::Folderblock), 1, file);

            for (int j = 0; j < 4; j++) {
                if (fb.b_content[j].b_inodo != -1) {
                    if (shared.compare(fb.b_content[j].b_name, "users.txt")) {
                        return fb.b_content[j].b_inodo;
                    }
                }
            }
        }
    }
    throw runtime_error("el archivo users.txt no fue encontrado");
}

string Users::readUsersContent(FILE *file, Structs::Superblock spr, Structs::Inodes inode) {
    if (inode.i_size <= 0) {
        return "";
    }
    if (inode.i_size > 12 * 64) {
        throw runtime_error("Capacidad maxima de users.txt superada");
    }

    string content = "";
    int remaining = inode.i_size;

    for (int i = 0; i < 12; i++) {
        if (remaining <= 0) break;
        if (inode.i_block[i] == -1) break;

        Structs::Fileblock fb;
        fseek(file, spr.s_block_start + inode.i_block[i] * sizeof(Structs::Fileblock), SEEK_SET);
        fread(&fb, sizeof(Structs::Fileblock), 1, file);

        int bytesToRead = min(remaining, 64);
        content.append(fb.b_content, bytesToRead);
        remaining -= bytesToRead;
    }

    return content;
}

int Users::allocFreeBlock(FILE *file, Structs::Superblock &spr) {
    char status;
    for (int i = 0; i < spr.s_blocks_count; i++) {
        fseek(file, spr.s_bm_block_start + i, SEEK_SET);
        fread(&status, sizeof(char), 1, file);
        if (status == '0') {
            status = '1';
            fseek(file, spr.s_bm_block_start + i, SEEK_SET);
            fwrite(&status, sizeof(char), 1, file);
            spr.s_free_blocks_count--;
            return i;
        }
    }
    return -1;
}

void Users::writeUsersContent(FILE *file, Structs::Superblock &spr, Structs::Partition partition, int inodeNum, string content) {
    Structs::Inodes inode;
    fseek(file, spr.s_inode_start + inodeNum * sizeof(Structs::Inodes), SEEK_SET);
    fread(&inode, sizeof(Structs::Inodes), 1, file);

    int len = content.length();
    int blocksNeeded = (len + 63) / 64;
    if (blocksNeeded == 0 && len == 0) {
        blocksNeeded = 1;
    }

    if (blocksNeeded > 12) {
        throw runtime_error("El contenido excede el espacio maximo disponible para users.txt (12 bloques directos)");
    }

    for (int i = 0; i < blocksNeeded; i++) {
        if (inode.i_block[i] == -1) {
            int newBlock = allocFreeBlock(file, spr);
            if (newBlock == -1) {
                throw runtime_error("No hay bloques libres en el sistema de archivos");
            }
            inode.i_block[i] = newBlock;
        }

        Structs::Fileblock fileb;
        memset(fileb.b_content, 0, sizeof(fileb.b_content));

        int offset = i * 64;
        int chunkSize = min(64, len - offset);
        if (chunkSize > 0) {
            memcpy(fileb.b_content, content.c_str() + offset, chunkSize);
        }

        fseek(file, spr.s_block_start + inode.i_block[i] * sizeof(Structs::Fileblock), SEEK_SET);
        fwrite(&fileb, sizeof(Structs::Fileblock), 1, file);
    }

    inode.i_size = len;
    inode.i_mtime = time(nullptr);

    // Write updated inode
    fseek(file, spr.s_inode_start + inodeNum * sizeof(Structs::Inodes), SEEK_SET);
    fwrite(&inode, sizeof(Structs::Inodes), 1, file);

    // Write updated superblock
    fseek(file, partition.part_start, SEEK_SET);
    fwrite(&spr, sizeof(Structs::Superblock), 1, file);
}

bool Users::login(vector<string> context, Mount m) {
    mount = m;
    string id = "";
    string usuario = "";
    string password = "";

    for (auto current : context) {
        string id_ = shared.lower(current.substr(0, current.find('=')));
        current.erase(0, id_.length() + 1);
        if (current.substr(0, 1) == "\"") {
            current = current.substr(1, current.length() - 2);
        }
        if (shared.compare(id_, "id")) {
            id = current;
        } else if (shared.compare(id_, "usuario") || shared.compare(id_, "usr") || shared.compare(id_, "user")) {
            usuario = current;
        } else if (shared.compare(id_, "password") || shared.compare(id_, "pwd") || shared.compare(id_, "pass")) {
            password = current;
        }
    }

    if (id == "" || usuario == "" || password == "") {
        shared.handler("LOGIN", "requiere ciertos parámetros obligatorios");
        return false;
    }
    return login(usuario, password, id);
}

bool Users::login(string u, string p, string id) {
    try {
        string path;
        Structs::Partition partition = mount.getmount(id, &path);

        FILE *rfile = fopen(path.c_str(), "rb");
        if (!rfile) {
            throw runtime_error("no se pudo abrir el disco");
        }

        Structs::Superblock super;
        fseek(rfile, partition.part_start, SEEK_SET);
        fread(&super, sizeof(Structs::Superblock), 1, rfile);

        int inodeNum = locateUsersTxtInode(rfile, super);
        Structs::Inodes inode;
        fseek(rfile, super.s_inode_start + inodeNum * sizeof(Structs::Inodes), SEEK_SET);
        fread(&inode, sizeof(Structs::Inodes), 1, rfile);

        string txt = readUsersContent(rfile, super, inode);
        fclose(rfile);

        vector<string> vctr = getElements(txt, '\n');
        int foundUid = -1;
        int foundGid = -1;
        string foundGroupName = "";
        bool userFound = false;

        for (string line : vctr) {
            if (line.empty()) continue;
            if (line[0] != '0' && (line[2] == 'U' || line[2] == 'u')) {
                vector<string> in = getElements(line, ',');
                if (in.size() >= 5 && shared.compare(in[3], u) && shared.compare(in[4], p)) {
                    foundUid = stoi(in[0]);
                    foundGroupName = in[2];
                    userFound = true;
                    break;
                }
            }
        }

        if (!userFound) {
            throw runtime_error("no hay credenciales similares");
        }

        for (string line : vctr) {
            if (line.empty()) continue;
            if (line[0] != '0' && (line[2] == 'G' || line[2] == 'g')) {
                vector<string> in = getElements(line, ',');
                if (in.size() >= 3 && shared.compare(in[2], foundGroupName)) {
                    foundGid = stoi(in[0]);
                    break;
                }
            }
        }

        if (foundGid == -1) {
            throw runtime_error("el grupo del usuario no existe o esta desactivado");
        }

        shared.response("LOGIN", "logueado correctamente");
        logged.id = id;
        logged.user = u;
        logged.password = p;
        logged.uid = foundUid;
        logged.gid = foundGid;
        logged.grp = foundGroupName;
        return true;
    }
    catch (exception &e) {
        shared.handler("LOGIN", e.what());
        return false;
    }
}

vector<string> Users::getElements(string txt, char c) {
    vector<string> v;
    string line;
    if (c == ',') {
        txt.push_back(',');
    }
    for (char &x: txt) {
        if (x == c) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            v.push_back(line);
            line = "";
            continue;
        }
        line += x;
    }
    if (!line.empty() && c != ',') {
        if (line.back() == '\r') {
            line.pop_back();
        }
        v.push_back(line);
    }

    if (v.empty()) {
        throw runtime_error("no hay archivo txt");
    }
    return v;
}

bool Users::logout() {
    shared.response("LOGOUT", "hasta luego " + logged.user);
    logged = User();
    return false;
}

void Users::grp(vector<string> context, string action) {
    vector<string> required = {"name"};
    string name;

    for (int i = 0; i < context.size(); i++) {
        string current = context.at(i);
        string id_ = current.substr(0, current.find("="));
        current.erase(0, id_.length() + 1);
        if (current.substr(0, 1) == "\"") {
            current = current.substr(1, current.length() - 2);
        }

        if (shared.compare(id_, "name")) {
            if (count(required.begin(), required.end(), id_)) {
                auto itr = find(required.begin(), required.end(), id_);
                required.erase(itr);
                name = current;
            }
        }
    }

    if (required.size() != 0) {
        shared.handler(action + "GRP", "requiere ciertos parámetros obligatorios");
        return;
    }
    if (shared.compare(action, "MK")) {
        mkgrp(name);
    } else {
        rmgrp(name);
    }
}

void Users::mkgrp(string n) {
    try {
        if (!(shared.compare(logged.user, "root"))) {
            throw runtime_error("no se puede realizar la acción sin el usuario root");
        }
        string path;
        Structs::Partition partition = mount.getmount(logged.id, &path);

        FILE *rfile = fopen(path.c_str(), "rb+");
        if (!rfile) {
            throw runtime_error("no se pudo abrir el disco");
        }

        Structs::Superblock super;
        fseek(rfile, partition.part_start, SEEK_SET);
        fread(&super, sizeof(Structs::Superblock), 1, rfile);

        int inodeNum = locateUsersTxtInode(rfile, super);
        Structs::Inodes inode;
        fseek(rfile, super.s_inode_start + inodeNum * sizeof(Structs::Inodes), SEEK_SET);
        fread(&inode, sizeof(Structs::Inodes), 1, rfile);

        string txt = readUsersContent(rfile, super, inode);

        vector<string> vctr = getElements(txt, '\n');
        int c = 0;
        for (string line : vctr) {
            if (line.empty()) continue;
            if (line[2] == 'G' || line[2] == 'g') {
                c++;
                vector<string> in = getElements(line, ',');
                if (in.size() >= 3 && shared.compare(in[2], n)) {
                    if (line[0] != '0') {
                        fclose(rfile);
                        throw runtime_error("el grupo ya existe");
                    }
                }
            }
        }
        txt += to_string(c + 1) + ",G," + n + "\n";

        writeUsersContent(rfile, super, partition, inodeNum, txt);
        fclose(rfile);
        shared.response("MKGRP", "grupo creado correctamente");
    }
    catch (exception &e) {
        shared.handler("MKGRP", e.what());
    }
}

void Users::rmgrp(string n) {
    try {
        if (!(shared.compare(logged.user, "root"))) {
            throw runtime_error("no se puede realizar la acción sin el usuario root");
        }
        string path;
        Structs::Partition partition = mount.getmount(logged.id, &path);

        FILE *rfile = fopen(path.c_str(), "rb+");
        if (!rfile) {
            throw runtime_error("no se pudo abrir el disco");
        }

        Structs::Superblock super;
        fseek(rfile, partition.part_start, SEEK_SET);
        fread(&super, sizeof(Structs::Superblock), 1, rfile);

        int inodeNum = locateUsersTxtInode(rfile, super);
        Structs::Inodes inode;
        fseek(rfile, super.s_inode_start + inodeNum * sizeof(Structs::Inodes), SEEK_SET);
        fread(&inode, sizeof(Structs::Inodes), 1, rfile);

        string txt = readUsersContent(rfile, super, inode);

        bool exist = false;
        vector<string> vctr = getElements(txt, '\n');
        string newTxt = "";
        for (string line : vctr) {
            if (line.empty()) continue;
            if ((line[2] == 'G' || line[2] == 'g') && line[0] != '0') {
                vector<string> in = getElements(line, ',');
                if (in.size() >= 3 && shared.compare(in[2], n)) {
                    exist = true;
                    newTxt += "0,G," + in[2] + "\n";
                    continue;
                }
            }
            newTxt += line + "\n";
        }
        if (!exist) {
            fclose(rfile);
            throw runtime_error("el grupo no existe");
        }

        writeUsersContent(rfile, super, partition, inodeNum, newTxt);
        fclose(rfile);
        shared.response("RMGRP", "grupo eliminado correctamente");
    }
    catch (exception &e) {
        shared.handler("RMGRP", e.what());
    }
}

void Users::usr(vector<string> context, string action) {
    vector<string> required;
    if (shared.compare(action, "MK")) {
        required = {"usr", "pwd", "grp"};
    } else {
        required = {"usr"};
    }
    string usr;
    string pwd;
    string grp;

    for (int i = 0; i < context.size(); i++) {
        string current = context.at(i);
        string id_ = current.substr(0, current.find("="));
        current.erase(0, id_.length() + 1);
        if (current.substr(0, 1) == "\"") {
            current = current.substr(1, current.length() - 2);
        }

        if (shared.compare(id_, "usr") || shared.compare(id_, "user") || shared.compare(id_, "usuario")) {
            auto itr = find(required.begin(), required.end(), "usr");
            if (itr != required.end()) {
                required.erase(itr);
            }
            usr = current;
        } else if (shared.compare(id_, "pwd") || shared.compare(id_, "pass") || shared.compare(id_, "password")) {
            auto itr = find(required.begin(), required.end(), "pwd");
            if (itr != required.end()) {
                required.erase(itr);
            }
            pwd = current;
        } else if (shared.compare(id_, "grp")) {
            auto itr = find(required.begin(), required.end(), "grp");
            if (itr != required.end()) {
                required.erase(itr);
            }
            grp = current;
        }
    }

    if (required.size() != 0) {
        shared.handler(action + "USR", "requiere ciertos parámetros obligatorios");
        return;
    }
    if (shared.compare(action, "MK")) {
        mkusr(usr, pwd, grp);
    } else {
        rmusr(usr);
    }
}

void Users::mkusr(string usr, string pwd, string grp) {
    try {
        if (!(shared.compare(logged.user, "root"))) {
            throw runtime_error("no se puede realizar la acción sin el usuario root");
        }
        if (usr.length() > 10 || pwd.length() > 10 || grp.length() > 10) {
            throw runtime_error("los parámetros no deben exceder 10 caracteres");
        }

        string path;
        Structs::Partition partition = mount.getmount(logged.id, &path);

        FILE *rfile = fopen(path.c_str(), "rb+");
        if (!rfile) {
            throw runtime_error("no se pudo abrir el disco");
        }

        Structs::Superblock super;
        fseek(rfile, partition.part_start, SEEK_SET);
        fread(&super, sizeof(Structs::Superblock), 1, rfile);

        int inodeNum = locateUsersTxtInode(rfile, super);
        Structs::Inodes inode;
        fseek(rfile, super.s_inode_start + inodeNum * sizeof(Structs::Inodes), SEEK_SET);
        fread(&inode, sizeof(Structs::Inodes), 1, rfile);

        string txt = readUsersContent(rfile, super, inode);

        vector<string> vctr = getElements(txt, '\n');
        bool groupExists = false;
        int uCount = 0;

        for (string line : vctr) {
            if (line.empty()) continue;
            vector<string> in = getElements(line, ',');
            if ((line[2] == 'G' || line[2] == 'g') && line[0] != '0') {
                if (in.size() >= 3 && shared.compare(in[2], grp)) {
                    groupExists = true;
                }
            } else if (line[2] == 'U' || line[2] == 'u') {
                uCount++;
                if (line[0] != '0') {
                    if (in.size() >= 4 && shared.compare(in[3], usr)) {
                        fclose(rfile);
                        throw runtime_error("el usuario ya existe");
                    }
                }
            }
        }

        if (!groupExists) {
            fclose(rfile);
            throw runtime_error("el grupo indicado no existe");
        }

        txt += to_string(uCount + 1) + ",U," + grp + "," + usr + "," + pwd + "\n";

        writeUsersContent(rfile, super, partition, inodeNum, txt);
        fclose(rfile);
        shared.response("MKUSR", "usuario creado correctamente");
    }
    catch (exception &e) {
        shared.handler("MKUSR", e.what());
    }
}

void Users::rmusr(string usr) {
    try {
        if (!(shared.compare(logged.user, "root"))) {
            throw runtime_error("no se puede realizar la acción sin el usuario root");
        }
        string path;
        Structs::Partition partition = mount.getmount(logged.id, &path);

        FILE *rfile = fopen(path.c_str(), "rb+");
        if (!rfile) {
            throw runtime_error("no se pudo abrir el disco");
        }

        Structs::Superblock super;
        fseek(rfile, partition.part_start, SEEK_SET);
        fread(&super, sizeof(Structs::Superblock), 1, rfile);

        int inodeNum = locateUsersTxtInode(rfile, super);
        Structs::Inodes inode;
        fseek(rfile, super.s_inode_start + inodeNum * sizeof(Structs::Inodes), SEEK_SET);
        fread(&inode, sizeof(Structs::Inodes), 1, rfile);

        string txt = readUsersContent(rfile, super, inode);

        bool exist = false;
        vector<string> vctr = getElements(txt, '\n');
        string newTxt = "";
        for (string line : vctr) {
            if (line.empty()) continue;
            if ((line[2] == 'U' || line[2] == 'u') && line[0] != '0') {
                vector<string> in = getElements(line, ',');
                if (in.size() >= 4 && shared.compare(in[3], usr)) {
                    exist = true;
                    newTxt += "0,U," + in[2] + "," + in[3] + "," + in[4] + "\n";
                    continue;
                }
            }
            newTxt += line + "\n";
        }

        if (!exist) {
            fclose(rfile);
            throw runtime_error("el usuario no existe");
        }

        writeUsersContent(rfile, super, partition, inodeNum, newTxt);
        fclose(rfile);
        shared.response("RMUSR", "usuario eliminado correctamente");
    }
    catch (exception &e) {
        shared.handler("RMUSR", e.what());
    }
}

void Users::chgrp(vector<string> context) {
    string user_name = "";
    string grp_name = "";

    for (int i = 0; i < context.size(); i++) {
        string current = context.at(i);
        string id_ = current.substr(0, current.find("="));
        current.erase(0, id_.length() + 1);
        if (current.substr(0, 1) == "\"") {
            current = current.substr(1, current.length() - 2);
        }

        if (shared.compare(id_, "user") || shared.compare(id_, "usr")) {
            user_name = current;
        } else if (shared.compare(id_, "grp")) {
            grp_name = current;
        }
    }

    if (user_name == "" || grp_name == "") {
        shared.handler("CHGRP", "requiere ciertos parámetros obligatorios");
        return;
    }
    chgrp(user_name, grp_name);
}

void Users::chgrp(string usr, string grp) {
    try {
        if (!(shared.compare(logged.user, "root"))) {
            throw runtime_error("no se puede realizar la acción sin el usuario root");
        }
        string path;
        Structs::Partition partition = mount.getmount(logged.id, &path);

        FILE *rfile = fopen(path.c_str(), "rb+");
        if (!rfile) {
            throw runtime_error("no se pudo abrir el disco");
        }

        Structs::Superblock super;
        fseek(rfile, partition.part_start, SEEK_SET);
        fread(&super, sizeof(Structs::Superblock), 1, rfile);

        int inodeNum = locateUsersTxtInode(rfile, super);
        Structs::Inodes inode;
        fseek(rfile, super.s_inode_start + inodeNum * sizeof(Structs::Inodes), SEEK_SET);
        fread(&inode, sizeof(Structs::Inodes), 1, rfile);

        string txt = readUsersContent(rfile, super, inode);

        vector<string> vctr = getElements(txt, '\n');

        bool groupExists = false;
        bool userExists = false;

        for (string line : vctr) {
            if (line.empty()) continue;
            if ((line[2] == 'G' || line[2] == 'g') && line[0] != '0') {
                vector<string> in = getElements(line, ',');
                if (in.size() >= 3 && shared.compare(in[2], grp)) {
                    groupExists = true;
                    break;
                }
            }
        }

        if (!groupExists) {
            fclose(rfile);
            throw runtime_error("el grupo destino no existe o esta eliminado");
        }

        for (string line : vctr) {
            if (line.empty()) continue;
            if ((line[2] == 'U' || line[2] == 'u') && line[0] != '0') {
                vector<string> in = getElements(line, ',');
                if (in.size() >= 4 && shared.compare(in[3], usr)) {
                    userExists = true;
                    break;
                }
            }
        }

        if (!userExists) {
            fclose(rfile);
            throw runtime_error("el usuario no existe");
        }

        string newTxt = "";
        for (string line : vctr) {
            if (line.empty()) continue;
            if ((line[2] == 'U' || line[2] == 'u') && line[0] != '0') {
                vector<string> in = getElements(line, ',');
                if (in.size() >= 5 && shared.compare(in[3], usr)) {
                    newTxt += in[0] + ",U," + grp + "," + in[3] + "," + in[4] + "\n";
                    continue;
                }
            }
            newTxt += line + "\n";
        }

        writeUsersContent(rfile, super, partition, inodeNum, newTxt);
        fclose(rfile);
        shared.response("CHGRP", "grupo de usuario actualizado correctamente");
    }
    catch (exception &e) {
        shared.handler("CHGRP", e.what());
    }
}
