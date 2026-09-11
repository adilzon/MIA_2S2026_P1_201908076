#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <bits/stdc++.h>

#include "../lib/shared.h"
#include "../lib/structs.h"
#include "../lib/mount.h"

using namespace std;

class FileManager {
public:
    FileManager();

    void mkdir(vector<string> context,
               Structs::Partition partition,
               string pth,
               int uid,
               int gid,
               string username);

    void mkfile(vector<string> context,
                Structs::Partition partition,
                string pth,
                int uid,
                int gid,
                string username);

    void cat(vector<string> context,
             Structs::Partition partition,
             string pth,
             int uid,
             int gid,
             string username);

    // --- Utilidades públicas para reportes ---
    int findInode(FILE *file, Structs::Superblock spr, string path);

    Structs::Inodes getInode(FILE *file, Structs::Superblock spr, int inodeNum);

    string getFileContent(FILE *file, Structs::Superblock spr, Structs::Inodes inode);

    vector<Structs::Content> listFolder(FILE *file, Structs::Superblock spr, Structs::Inodes inode);

    vector<string> getpath(string s);

    int getfree(Structs::Superblock spr, string pth, string t);

private:
    static const int BLOCK_SIZE = 64;

    Shared shared;

    Structs::Inodes readInode(FILE *file,
                              Structs::Superblock spr,
                              int inodeNum);

    void writeInode(FILE *file,
                    Structs::Superblock spr,
                    int inodeNum,
                    Structs::Inodes inode);

    void writeSuperblock(FILE *file,
                         Structs::Partition partition,
                         Structs::Superblock spr);

    int allocateInode(FILE *file,
                      Structs::Superblock &spr);

    int allocateBlock(FILE *file,
                      Structs::Superblock &spr);

    vector<int> getFolderBlocks(FILE *file,
                                Structs::Superblock spr,
                                Structs::Inodes inode);

    int locateChild(FILE *file,
                    Structs::Superblock spr,
                    int parentInodeNum,
                    string name);

    void addEntryToFolder(FILE *file,
                          Structs::Superblock &spr,
                          int parentInodeNum,
                          string name,
                          int childInode);

    int resolveParentPath(FILE *file,
                          Structs::Superblock &spr,
                          vector<string> comps,
                          bool createMissing,
                          int uid,
                          int gid,
                          string username);

    bool hasPermission(Structs::Inodes inode,
                       int uid,
                       int gid,
                       string username,
                       char action);

    void writeFileContent(FILE *file,
                          Structs::Superblock &spr,
                          int inodeNum,
                          string content);

    string readFileContent(FILE *file,
                           Structs::Superblock spr,
                           Structs::Inodes inode);
};

#endif