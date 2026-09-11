#include "../lib/filemanager.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <stdexcept>

using namespace std;

FileManager::FileManager() {
}

Structs::Inodes FileManager::readInode(FILE *file, Structs::Superblock spr, int inodeNum) {
    Structs::Inodes inode;
    fseek(file, spr.s_inode_start + sizeof(Structs::Inodes) * inodeNum, SEEK_SET);
    fread(&inode, sizeof(Structs::Inodes), 1, file);
    return inode;
}

void FileManager::writeInode(FILE *file, Structs::Superblock spr, int inodeNum, Structs::Inodes inode) {
    fseek(file, spr.s_inode_start + sizeof(Structs::Inodes) * inodeNum, SEEK_SET);
    fwrite(&inode, sizeof(Structs::Inodes), 1, file);
}

void FileManager::writeSuperblock(FILE *file, Structs::Partition partition, Structs::Superblock spr) {
    fseek(file, partition.part_start, SEEK_SET);
    fwrite(&spr, sizeof(Structs::Superblock), 1, file);
}

int FileManager::allocateInode(FILE *file, Structs::Superblock &spr) {
    char ch = '0';
    for (int i = 0; i < spr.s_inodes_count; i++) {
        fseek(file, spr.s_bm_inode_start + i, SEEK_SET);
        fread(&ch, sizeof(char), 1, file);
        if (ch == '0') {
            ch = '1';
            fseek(file, spr.s_bm_inode_start + i, SEEK_SET);
            fwrite(&ch, sizeof(char), 1, file);
            spr.s_free_inodes_count--;
            return i;
        }
    }
    return -1;
}

int FileManager::allocateBlock(FILE *file, Structs::Superblock &spr) {
    char ch = '0';
    for (int i = 0; i < spr.s_blocks_count; i++) {
        fseek(file, spr.s_bm_block_start + i, SEEK_SET);
        fread(&ch, sizeof(char), 1, file);
        if (ch == '0') {
            ch = '1';
            fseek(file, spr.s_bm_block_start + i, SEEK_SET);
            fwrite(&ch, sizeof(char), 1, file);
            spr.s_free_blocks_count--;
            return i;
        }
    }
    return -1;
}

int FileManager::getfree(Structs::Superblock spr, string pth, string t) {
    char ch = 'x';
    FILE *file = fopen(pth.c_str(), "rb");
    if (!file) return -1;
    if (t == "BI") {
        fseek(file, spr.s_bm_inode_start, SEEK_SET);
        for (int i = 0; i < spr.s_inodes_count; i++) {
            fread(&ch, sizeof(ch), 1, file);
            if (ch == '0') {
                fclose(file);
                return i;
            }
        }
    } else {
        fseek(file, spr.s_bm_block_start, SEEK_SET);
        for (int i = 0; i < spr.s_blocks_count; i++) {
            fread(&ch, sizeof(ch), 1, file);
            if (ch == '0') {
                fclose(file);
                return i;
            }
        }
    }
    fclose(file);
    return -1;
}

bool FileManager::hasPermission(Structs::Inodes inode, int uid, int gid, string username, char action) {
    if (username == "root" || uid == 1) {
        return true;
    }
    int index = 2; // default: otros
    if (uid == inode.i_uid) {
        index = 0; // propietario
    } else if (gid == inode.i_gid) {
        index = 1; // grupo
    }

    int val = inode.i_perm[index] - '0';
    if (action == 'r') {
        return (val & 4) != 0;
    } else if (action == 'w') {
        return (val & 2) != 0;
    } else if (action == 'x') {
        return (val & 1) != 0;
    }
    return false;
}

vector<int> FileManager::getFolderBlocks(FILE *file, Structs::Superblock spr, Structs::Inodes inode) {
    vector<int> blocks;
    for (int i = 0; i < 12; i++) {
        if (inode.i_block[i] != -1) {
            blocks.push_back(inode.i_block[i]);
        }
    }
    if (inode.i_block[12] != -1) {
        Structs::Pointerblock pb;
        fseek(file, spr.s_block_start + sizeof(Structs::Pointerblock) * inode.i_block[12], SEEK_SET);
        fread(&pb, sizeof(Structs::Pointerblock), 1, file);
        for (int i = 0; i < 16; i++) {
            if (pb.b_pointers[i] != -1) {
                blocks.push_back(pb.b_pointers[i]);
            }
        }
    }
    return blocks;
}

int FileManager::locateChild(FILE *file, Structs::Superblock spr, int parentInodeNum, string name) {
    Structs::Inodes parent = readInode(file, spr, parentInodeNum);
    vector<int> blocks = getFolderBlocks(file, spr, parent);

    for (int bNum : blocks) {
        Structs::Folderblock fb;
        fseek(file, spr.s_block_start + sizeof(Structs::Folderblock) * bNum, SEEK_SET);
        fread(&fb, sizeof(Structs::Folderblock), 1, file);

        for (int j = 0; j < 4; j++) {
            if (fb.b_content[j].b_inodo != -1) {
                if (shared.compare(fb.b_content[j].b_name, name)) {
                    return fb.b_content[j].b_inodo;
                }
            }
        }
    }
    return -1;
}

void FileManager::addEntryToFolder(FILE *file, Structs::Superblock &spr, int parentInodeNum, string name, int childInode) {
    Structs::Inodes parent = readInode(file, spr, parentInodeNum);

    // 1. Search existing direct blocks
    for (int i = 0; i < 12; i++) {
        if (parent.i_block[i] != -1) {
            Structs::Folderblock fb;
            fseek(file, spr.s_block_start + sizeof(Structs::Folderblock) * parent.i_block[i], SEEK_SET);
            fread(&fb, sizeof(Structs::Folderblock), 1, file);

            for (int j = 0; j < 4; j++) {
                if (fb.b_content[j].b_inodo == -1) {
                    memset(fb.b_content[j].b_name, 0, 12);
                    strncpy(fb.b_content[j].b_name, name.c_str(), 11);
                    fb.b_content[j].b_inodo = childInode;

                    fseek(file, spr.s_block_start + sizeof(Structs::Folderblock) * parent.i_block[i], SEEK_SET);
                    fwrite(&fb, sizeof(Structs::Folderblock), 1, file);
                    parent.i_mtime = time(nullptr);
                    writeInode(file, spr, parentInodeNum, parent);
                    return;
                }
            }
        }
    }

    // 2. Search existing indirect blocks
    if (parent.i_block[12] != -1) {
        Structs::Pointerblock pb;
        fseek(file, spr.s_block_start + sizeof(Structs::Pointerblock) * parent.i_block[12], SEEK_SET);
        fread(&pb, sizeof(Structs::Pointerblock), 1, file);

        for (int i = 0; i < 16; i++) {
            if (pb.b_pointers[i] != -1) {
                Structs::Folderblock fb;
                fseek(file, spr.s_block_start + sizeof(Structs::Folderblock) * pb.b_pointers[i], SEEK_SET);
                fread(&fb, sizeof(Structs::Folderblock), 1, file);

                for (int j = 0; j < 4; j++) {
                    if (fb.b_content[j].b_inodo == -1) {
                        memset(fb.b_content[j].b_name, 0, 12);
                        strncpy(fb.b_content[j].b_name, name.c_str(), 11);
                        fb.b_content[j].b_inodo = childInode;

                        fseek(file, spr.s_block_start + sizeof(Structs::Folderblock) * pb.b_pointers[i], SEEK_SET);
                        fwrite(&fb, sizeof(Structs::Folderblock), 1, file);
                        parent.i_mtime = time(nullptr);
                        writeInode(file, spr, parentInodeNum, parent);
                        return;
                    }
                }
            }
        }
    }

    // 3. Allocate a new Folderblock
    int newFolderBlockNum = allocateBlock(file, spr);
    if (newFolderBlockNum == -1) {
        throw runtime_error("No hay bloques libres en el sistema de archivos");
    }

    Structs::Folderblock newFb;
    for (int j = 0; j < 4; j++) {
        memset(newFb.b_content[j].b_name, 0, 12);
        newFb.b_content[j].b_inodo = -1;
    }
    strncpy(newFb.b_content[0].b_name, name.c_str(), 11);
    newFb.b_content[0].b_inodo = childInode;

    // Link new block to direct pointer
    for (int i = 0; i < 12; i++) {
        if (parent.i_block[i] == -1) {
            parent.i_block[i] = newFolderBlockNum;
            parent.i_size += sizeof(Structs::Folderblock);
            parent.i_mtime = time(nullptr);

            fseek(file, spr.s_block_start + sizeof(Structs::Folderblock) * newFolderBlockNum, SEEK_SET);
            fwrite(&newFb, sizeof(Structs::Folderblock), 1, file);

            writeInode(file, spr, parentInodeNum, parent);
            return;
        }
    }

    // Direct pointers full; use indirect pointer i_block[12]
    if (parent.i_block[12] == -1) {
        int ptrBlockNum = allocateBlock(file, spr);
        if (ptrBlockNum == -1) {
            throw runtime_error("No hay bloques libres en el sistema de archivos");
        }

        Structs::Pointerblock pb;
        for (int k = 0; k < 16; k++) pb.b_pointers[k] = -1;
        pb.b_pointers[0] = newFolderBlockNum;

        parent.i_block[12] = ptrBlockNum;
        parent.i_size += sizeof(Structs::Folderblock);
        parent.i_mtime = time(nullptr);

        fseek(file, spr.s_block_start + sizeof(Structs::Pointerblock) * ptrBlockNum, SEEK_SET);
        fwrite(&pb, sizeof(Structs::Pointerblock), 1, file);

        fseek(file, spr.s_block_start + sizeof(Structs::Folderblock) * newFolderBlockNum, SEEK_SET);
        fwrite(&newFb, sizeof(Structs::Folderblock), 1, file);

        writeInode(file, spr, parentInodeNum, parent);
        return;
    } else {
        Structs::Pointerblock pb;
        fseek(file, spr.s_block_start + sizeof(Structs::Pointerblock) * parent.i_block[12], SEEK_SET);
        fread(&pb, sizeof(Structs::Pointerblock), 1, file);

        for (int k = 0; k < 16; k++) {
            if (pb.b_pointers[k] == -1) {
                pb.b_pointers[k] = newFolderBlockNum;
                parent.i_size += sizeof(Structs::Folderblock);
                parent.i_mtime = time(nullptr);

                fseek(file, spr.s_block_start + sizeof(Structs::Pointerblock) * parent.i_block[12], SEEK_SET);
                fwrite(&pb, sizeof(Structs::Pointerblock), 1, file);

                fseek(file, spr.s_block_start + sizeof(Structs::Folderblock) * newFolderBlockNum, SEEK_SET);
                fwrite(&newFb, sizeof(Structs::Folderblock), 1, file);

                writeInode(file, spr, parentInodeNum, parent);
                return;
            }
        }
        throw runtime_error("la carpeta ha superado la capacidad máxima de bloques indirectos (16)");
    }
}

int FileManager::resolveParentPath(FILE *file, Structs::Superblock &spr, vector<string> comps, bool createMissing, int uid, int gid, string username) {
    int currentInodeNum = 0;

    for (size_t i = 0; i < comps.size(); i++) {
        string comp = comps[i];
        Structs::Inodes currInode = readInode(file, spr, currentInodeNum);

        int childInodeNum = locateChild(file, spr, currentInodeNum, comp);
        if (childInodeNum != -1) {
            currentInodeNum = childInodeNum;
        } else {
            if (!createMissing) {
                throw runtime_error("el directorio padre '/" + comp + "' no existe");
            }

            if (!hasPermission(currInode, uid, gid, username, 'w')) {
                throw runtime_error("no tiene permiso de escritura sobre el directorio padre");
            }

            int newInodeNum = allocateInode(file, spr);
            if (newInodeNum == -1) {
                throw runtime_error("No hay inodos libres en el sistema de archivos");
            }

            int newBlockNum = allocateBlock(file, spr);
            if (newBlockNum == -1) {
                throw runtime_error("No hay bloques libres en el sistema de archivos");
            }

            Structs::Inodes newInode;
            newInode.i_uid = uid;
            newInode.i_gid = gid;
            newInode.i_size = sizeof(Structs::Folderblock);
            newInode.i_atime = time(nullptr);
            newInode.i_ctime = time(nullptr);
            newInode.i_mtime = time(nullptr);
            newInode.i_type = 0;
            newInode.i_perm[0] = '6';
            newInode.i_perm[1] = '6';
            newInode.i_perm[2] = '4';
            newInode.i_block[0] = newBlockNum;

            Structs::Folderblock fb;
            memset(fb.b_content[0].b_name, 0, 12);
            strcpy(fb.b_content[0].b_name, ".");
            fb.b_content[0].b_inodo = newInodeNum;

            memset(fb.b_content[1].b_name, 0, 12);
            strcpy(fb.b_content[1].b_name, "..");
            fb.b_content[1].b_inodo = currentInodeNum;

            memset(fb.b_content[2].b_name, 0, 12);
            fb.b_content[2].b_inodo = -1;

            memset(fb.b_content[3].b_name, 0, 12);
            fb.b_content[3].b_inodo = -1;

            writeInode(file, spr, newInodeNum, newInode);
            fseek(file, spr.s_block_start + sizeof(Structs::Folderblock) * newBlockNum, SEEK_SET);
            fwrite(&fb, sizeof(Structs::Folderblock), 1, file);

            addEntryToFolder(file, spr, currentInodeNum, comp, newInodeNum);

            currentInodeNum = newInodeNum;
        }
    }
    return currentInodeNum;
}

void FileManager::writeFileContent(FILE *file, Structs::Superblock &spr, int inodeNum, string content) {
    int len = (int)content.length();
    int blocksNeeded = (len + 63) / 64;
    if (len == 0) blocksNeeded = 0;

    if (blocksNeeded > 28) {
        throw runtime_error("el archivo supera la capacidad soportada (doble/triple indirecto no implementado)");
    }

    Structs::Inodes fileInode = readInode(file, spr, inodeNum);

    for (int b = 0; b < blocksNeeded; b++) {
        int offset = b * 64;
        int chunkSize = min(64, len - offset);

        Structs::Fileblock fileb;
        memset(fileb.b_content, 0, 64);
        if (chunkSize > 0) {
            memcpy(fileb.b_content, content.c_str() + offset, chunkSize);
        }

        if (b < 12) {
            if (fileInode.i_block[b] == -1) {
                int nBlock = allocateBlock(file, spr);
                if (nBlock == -1) throw runtime_error("No hay bloques libres en el sistema de archivos");
                fileInode.i_block[b] = nBlock;
            }
            fseek(file, spr.s_block_start + sizeof(Structs::Fileblock) * fileInode.i_block[b], SEEK_SET);
            fwrite(&fileb, sizeof(Structs::Fileblock), 1, file);
        } else {
            if (fileInode.i_block[12] == -1) {
                int ptrBlock = allocateBlock(file, spr);
                if (ptrBlock == -1) throw runtime_error("No hay bloques libres en el sistema de archivos");
                fileInode.i_block[12] = ptrBlock;

                Structs::Pointerblock pb;
                for (int k = 0; k < 16; k++) pb.b_pointers[k] = -1;
                fseek(file, spr.s_block_start + sizeof(Structs::Pointerblock) * ptrBlock, SEEK_SET);
                fwrite(&pb, sizeof(Structs::Pointerblock), 1, file);
            }

            Structs::Pointerblock pb;
            fseek(file, spr.s_block_start + sizeof(Structs::Pointerblock) * fileInode.i_block[12], SEEK_SET);
            fread(&pb, sizeof(Structs::Pointerblock), 1, file);

            int ptrIndex = b - 12;
            if (pb.b_pointers[ptrIndex] == -1) {
                int nBlock = allocateBlock(file, spr);
                if (nBlock == -1) throw runtime_error("No hay bloques libres en el sistema de archivos");
                pb.b_pointers[ptrIndex] = nBlock;

                fseek(file, spr.s_block_start + sizeof(Structs::Pointerblock) * fileInode.i_block[12], SEEK_SET);
                fwrite(&pb, sizeof(Structs::Pointerblock), 1, file);
            }

            fseek(file, spr.s_block_start + sizeof(Structs::Fileblock) * pb.b_pointers[ptrIndex], SEEK_SET);
            fwrite(&fileb, sizeof(Structs::Fileblock), 1, file);
        }
    }

    fileInode.i_size = len;
    fileInode.i_mtime = time(nullptr);
    writeInode(file, spr, inodeNum, fileInode);
}

string FileManager::readFileContent(FILE *file, Structs::Superblock spr, Structs::Inodes inode) {
    int remaining = inode.i_size;
    if (remaining <= 0) return "";

    string content = "";

    // Direct blocks
    for (int i = 0; i < 12 && remaining > 0; i++) {
        if (inode.i_block[i] != -1) {
            Structs::Fileblock fb;
            fseek(file, spr.s_block_start + sizeof(Structs::Fileblock) * inode.i_block[i], SEEK_SET);
            fread(&fb, sizeof(Structs::Fileblock), 1, file);

            int bytesToRead = min(remaining, 64);
            content.append(fb.b_content, bytesToRead);
            remaining -= bytesToRead;
        }
    }

    // Indirect block
    if (remaining > 0 && inode.i_block[12] != -1) {
        Structs::Pointerblock pb;
        fseek(file, spr.s_block_start + sizeof(Structs::Pointerblock) * inode.i_block[12], SEEK_SET);
        fread(&pb, sizeof(Structs::Pointerblock), 1, file);

        for (int k = 0; k < 16 && remaining > 0; k++) {
            if (pb.b_pointers[k] != -1) {
                Structs::Fileblock fb;
                fseek(file, spr.s_block_start + sizeof(Structs::Fileblock) * pb.b_pointers[k], SEEK_SET);
                fread(&fb, sizeof(Structs::Fileblock), 1, file);

                int bytesToRead = min(remaining, 64);
                content.append(fb.b_content, bytesToRead);
                remaining -= bytesToRead;
            }
        }
    }

    return content;
}

vector<string> FileManager::getpath(string s) {
    vector<string> result;
    if (s.empty()) {
        return result;
    }

    s.push_back('/');
    string tmp;
    int status = -1;
    for (char &c : s) {
        if (status != -1) {
            if (status == 2 && c == '\"') {
                status = 3;
                continue;
            } else if (status == 1) {
                if (c == '\"') {
                    status = 2;
                    continue;
                } else if (c == '/') {
                    continue;
                }
                status = 3;
            }

            if ((status == 3) && c == '/') {
                status = 1;
                if (!tmp.empty()) {
                    result.push_back(tmp);
                }
                tmp = "";
                continue;
            }
            tmp += c;
        } else if (c == '/') {
            status = 1;
        }
    }
    return result;
}

void FileManager::mkdir(vector<string> context, Structs::Partition partition, string pth, int uid, int gid, string username) {
    try {
        string path = "";
        bool p = false;

        for (auto current : context) {
            string id = shared.lower(current.substr(0, current.find('=')));
            current.erase(0, id.length() + 1);
            if (current.substr(0, 1) == "\"") {
                current = current.substr(1, current.length() - 2);
            }
            while (!id.empty() && id.front() == '-') {
                id.erase(0, 1);
            }

            if (shared.compare(id, "path")) {
                path = current;
            } else if (shared.compare(id, "p")) {
                p = true;
            }
        }

        if (path.empty()) {
            throw runtime_error("requiere el parámetro obligatorio -path");
        }

        vector<string> comps = getpath(path);
        if (comps.empty()) {
            throw runtime_error("la ruta especificada no es válida");
        }

        string newDirName = comps.back();
        comps.pop_back();

        FILE *file = fopen(pth.c_str(), "rb+");
        if (!file) {
            throw runtime_error("no se pudo abrir el disco");
        }

        Structs::Superblock spr;
        fseek(file, partition.part_start, SEEK_SET);
        fread(&spr, sizeof(Structs::Superblock), 1, file);

        int parentInodeNum = resolveParentPath(file, spr, comps, p, uid, gid, username);
        Structs::Inodes parentInode = readInode(file, spr, parentInodeNum);

        if (!hasPermission(parentInode, uid, gid, username, 'w')) {
            fclose(file);
            throw runtime_error("no tiene permiso de escritura en el directorio padre");
        }

        int existing = locateChild(file, spr, parentInodeNum, newDirName);
        if (existing != -1) {
            fclose(file);
            throw runtime_error("el directorio ya existe");
        }

        int newInodeNum = allocateInode(file, spr);
        if (newInodeNum == -1) {
            fclose(file);
            throw runtime_error("No hay inodos libres en el sistema de archivos");
        }

        int newBlockNum = allocateBlock(file, spr);
        if (newBlockNum == -1) {
            fclose(file);
            throw runtime_error("No hay bloques libres en el sistema de archivos");
        }

        Structs::Inodes newInode;
        newInode.i_uid = uid;
        newInode.i_gid = gid;
        newInode.i_size = sizeof(Structs::Folderblock);
        newInode.i_atime = time(nullptr);
        newInode.i_ctime = time(nullptr);
        newInode.i_mtime = time(nullptr);
        newInode.i_type = 0;
        newInode.i_perm[0] = '6';
        newInode.i_perm[1] = '6';
        newInode.i_perm[2] = '4';
        newInode.i_block[0] = newBlockNum;

        Structs::Folderblock fb;
        memset(fb.b_content[0].b_name, 0, 12);
        strcpy(fb.b_content[0].b_name, ".");
        fb.b_content[0].b_inodo = newInodeNum;

        memset(fb.b_content[1].b_name, 0, 12);
        strcpy(fb.b_content[1].b_name, "..");
        fb.b_content[1].b_inodo = parentInodeNum;

        memset(fb.b_content[2].b_name, 0, 12);
        fb.b_content[2].b_inodo = -1;

        memset(fb.b_content[3].b_name, 0, 12);
        fb.b_content[3].b_inodo = -1;

        writeInode(file, spr, newInodeNum, newInode);
        fseek(file, spr.s_block_start + sizeof(Structs::Folderblock) * newBlockNum, SEEK_SET);
        fwrite(&fb, sizeof(Structs::Folderblock), 1, file);

        addEntryToFolder(file, spr, parentInodeNum, newDirName, newInodeNum);
        writeSuperblock(file, partition, spr);

        fclose(file);
        shared.response("MKDIR", "se ha creado el directorio con éxito");
    }
    catch (exception &e) {
        shared.handler("MKDIR", e.what());
    }
}

void FileManager::mkfile(vector<string> context, Structs::Partition partition, string pth, int uid, int gid, string username) {
    try {
        string path = "";
        int size = -1;
        string cont = "";
        bool r = false;

        for (auto current : context) {
            string id = shared.lower(current.substr(0, current.find('=')));
            current.erase(0, id.length() + 1);
            if (current.substr(0, 1) == "\"") {
                current = current.substr(1, current.length() - 2);
            }
            while (!id.empty() && id.front() == '-') {
                id.erase(0, 1);
            }

            if (shared.compare(id, "path")) {
                path = current;
            } else if (shared.compare(id, "size")) {
                size = stoi(current);
                if (size < 0) {
                    throw runtime_error("el tamaño (-size) no puede ser negativo");
                }
            } else if (shared.compare(id, "cont")) {
                cont = current;
            } else if (shared.compare(id, "r")) {
                r = true;
            }
        }

        if (path.empty()) {
            throw runtime_error("requiere el parámetro obligatorio -path");
        }

        string fileContent = "";
        if (!cont.empty()) {
            ifstream cFile(cont);
            if (!cFile.is_open()) {
                throw runtime_error("no se pudo abrir el archivo de -cont: " + cont);
            }
            string str((istreambuf_iterator<char>(cFile)), istreambuf_iterator<char>());
            fileContent = str;
            cFile.close();
        } else if (size >= 0) {
            for (int i = 0; i < size; i++) {
                fileContent += to_string(i % 10);
            }
        }

        vector<string> comps = getpath(path);
        if (comps.empty()) {
            throw runtime_error("la ruta especificada no es válida");
        }

        string fileName = comps.back();
        comps.pop_back();

        FILE *file = fopen(pth.c_str(), "rb+");
        if (!file) {
            throw runtime_error("no se pudo abrir el disco");
        }

        Structs::Superblock spr;
        fseek(file, partition.part_start, SEEK_SET);
        fread(&spr, sizeof(Structs::Superblock), 1, file);

        int parentInodeNum = resolveParentPath(file, spr, comps, r, uid, gid, username);
        Structs::Inodes parentInode = readInode(file, spr, parentInodeNum);

        if (!hasPermission(parentInode, uid, gid, username, 'w')) {
            fclose(file);
            throw runtime_error("no tiene permiso de escritura en el directorio padre");
        }

        int targetInodeNum = locateChild(file, spr, parentInodeNum, fileName);
        if (targetInodeNum != -1) {
            cout << "El archivo ya existe. ¿Desea sobrescribirlo? (s/n): ";
            char resp;
            cin >> resp;
            if (resp != 's' && resp != 'S') {
                cout << "Operación cancelada." << endl;
                fclose(file);
                return;
            }
            Structs::Inodes fileInode = readInode(file, spr, targetInodeNum);
            if (!hasPermission(fileInode, uid, gid, username, 'w')) {
                fclose(file);
                throw runtime_error("no tiene permiso de escritura sobre el archivo");
            }
        } else {
            targetInodeNum = allocateInode(file, spr);
            if (targetInodeNum == -1) {
                fclose(file);
                throw runtime_error("No hay inodos libres en el sistema de archivos");
            }

            Structs::Inodes fileInode;
            fileInode.i_uid = uid;
            fileInode.i_gid = gid;
            fileInode.i_size = 0;
            fileInode.i_atime = time(nullptr);
            fileInode.i_ctime = time(nullptr);
            fileInode.i_mtime = time(nullptr);
            fileInode.i_type = 1;
            fileInode.i_perm[0] = '6';
            fileInode.i_perm[1] = '6';
            fileInode.i_perm[2] = '4';

            writeInode(file, spr, targetInodeNum, fileInode);
            addEntryToFolder(file, spr, parentInodeNum, fileName, targetInodeNum);
        }

        writeFileContent(file, spr, targetInodeNum, fileContent);
        writeSuperblock(file, partition, spr);

        fclose(file);
        shared.response("MKFILE", "archivo creado con éxito");
    }
    catch (exception &e) {
        shared.handler("MKFILE", e.what());
    }
}

void FileManager::cat(vector<string> context, Structs::Partition partition, string pth, int uid, int gid, string username) {
    try {
        vector<string> filePaths;

        for (auto current : context) {
            string id = shared.lower(current.substr(0, current.find('=')));
            current.erase(0, id.length() + 1);
            if (current.substr(0, 1) == "\"") {
                current = current.substr(1, current.length() - 2);
            }
            while (!id.empty() && id.front() == '-') {
                id.erase(0, 1);
            }

            if (id.length() >= 4 && id.substr(0, 4) == "file") {
                filePaths.push_back(current);
            }
        }

        if (filePaths.empty()) {
            throw runtime_error("requiere al menos un parámetro de archivo (-file1=...)");
        }

        FILE *file = fopen(pth.c_str(), "rb");
        if (!file) {
            throw runtime_error("no se pudo abrir el disco");
        }

        Structs::Superblock spr;
        fseek(file, partition.part_start, SEEK_SET);
        fread(&spr, sizeof(Structs::Superblock), 1, file);

        string output = "";

        for (size_t i = 0; i < filePaths.size(); i++) {
            string filePath = filePaths[i];
            vector<string> comps = getpath(filePath);
            if (comps.empty()) {
                fclose(file);
                throw runtime_error("la ruta '" + filePath + "' no es válida");
            }

            string fileName = comps.back();
            comps.pop_back();

            int parentInodeNum;
            try {
                parentInodeNum = resolveParentPath(file, spr, comps, false, uid, gid, username);
            } catch (exception &e) {
                fclose(file);
                throw runtime_error("error al resolver la ruta: " + string(e.what()));
            }

            int targetInodeNum = locateChild(file, spr, parentInodeNum, fileName);
            if (targetInodeNum == -1) {
                fclose(file);
                throw runtime_error("el archivo '" + filePath + "' no existe");
            }

            Structs::Inodes fileInode = readInode(file, spr, targetInodeNum);
            if (fileInode.i_type != 1) {
                fclose(file);
                throw runtime_error("el elemento '" + filePath + "' no es un archivo");
            }

            if (!hasPermission(fileInode, uid, gid, username, 'r')) {
                fclose(file);
                throw runtime_error("no tiene permiso de lectura sobre " + filePath);
            }

            string content = readFileContent(file, spr, fileInode);

            if (!output.empty() && output.back() != '\n') {
                output += "\n";
            }
            output += content;
        }

        fclose(file);
        cout << output << endl;
    }
    catch (exception &e) {
        shared.handler("CAT", e.what());
    }
}

int FileManager::findInode(FILE *file, Structs::Superblock spr, string path) {
    vector<string> comps = getpath(path);

    if (comps.empty())
        return 0;

    string last = comps.back();
    comps.pop_back();

    int parentInodeNum;

    try {
        parentInodeNum = resolveParentPath(
            file,
            spr,
            comps,
            false,
            1,
            1,
            "root"
        );
    }
    catch (exception &e) {
        return -1;
    }

    if (parentInodeNum == -1) return -1;

    return locateChild(file, spr, parentInodeNum, last);
}

Structs::Inodes FileManager::getInode(
    FILE *file,
    Structs::Superblock spr,
    int inodeNum
) {
    return readInode(file, spr, inodeNum);
}

string FileManager::getFileContent(
    FILE *file,
    Structs::Superblock spr,
    Structs::Inodes inode
) {
    return readFileContent(file, spr, inode);
}

vector<Structs::Content> FileManager::listFolder(
    FILE *file,
    Structs::Superblock spr,
    Structs::Inodes inode
) {
    vector<Structs::Content> entries;

    vector<int> blocks = getFolderBlocks(file, spr, inode);

    for (int b : blocks) {
        Structs::Folderblock fb;

        fseek(
            file,
            spr.s_block_start + (BLOCK_SIZE * b),
            SEEK_SET
        );

        fread(&fb, sizeof(fb), 1, file);

        for (int k = 0; k < 4; k++) {
            if (fb.b_content[k].b_inodo != -1) {
                string nm(fb.b_content[k].b_name);

                if (nm != "." && nm != "..") {
                    entries.push_back(fb.b_content[k]);
                }
            }
        }
    }

    return entries;
}