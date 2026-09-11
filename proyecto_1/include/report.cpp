#include "../lib/report.h"
#include "../lib/disco.h"
#include <iostream>
#include <stdlib.h>
#include "string"
#include <locale>

using namespace std;

Report::Report(){}

string Report::extOf(string p) {
    size_t pos = p.find_last_of('.');

    if (pos == string::npos)
        return "jpg";

    return p.substr(pos + 1);
}

void Report::generar(vector<string> context, Mount m)
{
    mount = m;

    vector<string> required = {
        "id",
        "path",
        "name"
    };

    string name;
    string path;
    string id;
    string pathFileLs;

    for (string current : context) {

        string id_ =
            shared.lower(
                current.substr(
                    0,
                    current.find('=')
                )
            );

        current.erase(
            0,
            id_.length() + 1
        );

        if (
            current.substr(0,1) == "\""
        ) {
            current =
                current.substr(
                    1,
                    current.length() - 2
                );
        }

        if (shared.compare(id_, "name")) {

            if (
                count(
                    required.begin(),
                    required.end(),
                    id_
                )
            ) {
                auto itr =
                    find(
                        required.begin(),
                        required.end(),
                        id_
                    );

                required.erase(itr);
                name = current;
            }

        } else if (
            shared.compare(id_, "id")
        ) {

            if (
                count(
                    required.begin(),
                    required.end(),
                    id_
                )
            ) {
                auto itr =
                    find(
                        required.begin(),
                        required.end(),
                        id_
                    );

                required.erase(itr);
                id = current;
            }

        } else if (
            shared.compare(id_, "path")
        ) {

            if (
                count(
                    required.begin(),
                    required.end(),
                    id_
                )
            ) {
                auto itr =
                    find(
                        required.begin(),
                        required.end(),
                        id_
                    );

                required.erase(itr);
                path = current;
            }

        } else if (
            shared.compare(id_, "path_file_ls")
        ) {

            pathFileLs = current;
        }
    }

    if (required.size() != 0) {

        shared.handler(
            "REPORT",
            " faltan parametros para realizar el reporte"
        );

        return;
    }

    if (shared.compare(name, "MBR")) {
        mbr(path, id);

    } else if (shared.compare(name, "INODE")) {
        inode(path, id);

    } else if (shared.compare(name, "BLOCK")) {
        block(path, id);

    } else if (shared.compare(name, "BM_INODE")) {
        bminode(path, id);

    } else if (shared.compare(name, "BM_BLOCK")) {
        bmblock(path, id);

    } else if (shared.compare(name, "SB")) {
        sb(path, id);

    } else if (shared.compare(name, "TREE")) {
        tree(path, id);

    } else if (shared.compare(name, "DISK")) {
        dks(path, id);

    } else if (shared.compare(name, "FILE")) {
        file(path, id, pathFileLs);

    } else if (shared.compare(name, "LS")) {
        ls(path, id, pathFileLs);

    } else {

        shared.handler(
            "REPORT",
            "no es un reporte válido"
        );

        return;
    }
}

void Report::mbr(string p, string id) {
    try {
        string path;
        Structs::Partition partition = mount.getmount(id, &path);

        FILE *file = fopen(path.c_str(), "rb+");
        if (file == NULL) {
            throw runtime_error("disco no existente");
        }

        Structs::MBR disco;
        rewind(file);
        fread(&disco, sizeof(Structs::MBR), 1, file);
        fclose(file);

        string pd = p.substr(0, p.find('.'));
        pd += ".dot";
        FILE *doc = fopen(pd.c_str(), "r");
        if (doc == NULL) {
            string cmm = "mkdir -p \"" + pd + "\"";
            string cmm2 = "rmdir \"" + pd + "\"";
            system(cmm.c_str());
            system(cmm2.c_str());
        } else {
            fclose(doc);
        }

        Structs::Partition partitions[4];
        partitions[0] = disco.mbr_Partition_1;
        partitions[1] = disco.mbr_Partition_2;
        partitions[2] = disco.mbr_Partition_3;
        partitions[3] = disco.mbr_Partition_4;
        struct tm *tm;
        tm = localtime(&disco.mbr_fecha_creacion);
        char mostrar_fecha [20];
        strftime(mostrar_fecha, 20, "%Y/%m/%d %H:%M:%S", tm);
        string content;
        content = "digraph G{\n"
                  "rankdir=TB;\n"
                  "graph [ dpi = \"600\" ]; \n"
                  "forcelabels= true;\n"
                  "node [shape = plaintext];\n"
                  "general [label = <<table>\n"
                  "<tr><td COLSPAN = '2' BGCOLOR=\"#102027\"><font color=\"white\">MBR</font></td></tr>\n"
                  "<tr><td BGCOLOR=\"#ff6f00\">NOMBRE</td><td BGCOLOR=\"#ff6f00\" >VALOR</td></tr>\n"
                  "<tr>\n"
                  "<td>mbr_tamaño</td>\n"
                  "<td>" +
                  to_string(disco.mbr_tamano) + "</td>\n"
                                               "</tr>\n"
                                               "<tr>\n"
                                               "<td>mbr_fecha_creación</td>\n"
                                               "<td>" +
                  string(mostrar_fecha) + "</td>\n"
                                                    "</tr>\n"
                                                    "<tr>\n"
                                                    "<td>mbr_disk_signature</td>\n"
                                                    "<td>" +
                  to_string(disco.mbr_disk_signature) + "</td>\n"
                                                       "</tr>\n"
                                                       "<tr>\n"
                                                       "<td>Disk_fit</td>\n"
                                                       "<td>" +
                  string(1, disco.disk_fit) + "</td>\n"
                                             "</tr>\n";

        Structs::Partition extended;
        bool ext = false;
        for (int i = 0; i < 4; ++i) {
            if (partitions[i].part_status == '1') {
                if (partitions[i].part_type == 'E') {
                    ext = true;
                    extended = partitions[i];
                }
                content += "<tr>\n"
                           "<td>part_status_" + to_string(i + 1) + "</td>\n"
                                                                   "<td>" +
                           partitions[i].part_status + " </td >\n"
                                                       "</tr>\n"
                                                       "<tr>\n"
                                                       "<td>part_type_" + to_string(i + 1) + "</td>\n"
                                                                                             "<td>" +
                           partitions[i].part_type + "</td>\n"
                                                     "</tr>\n"
                                                     "<tr>\n"
                                                     "<td>part_fit_" + to_string(i + 1) + "</td>\n"
                                                                                          "<td>" +
                           partitions[i].part_fit + "</td>\n"
                                                    "</tr>\n"
                                                    "<tr>\n"
                                                    "<td>part_start_" + to_string(i + 1) + "</td>\n"
                                                                                           "<td>" +
                           to_string(partitions[i].part_start) + "</td>\n"
                                                                 "</tr>\n"
                                                                 "<tr>\n"
                                                                 "<td>part_size_" + to_string(i + 1) + "</td>\n"
                                                                                                       "<td>" +
                           to_string(partitions[i].part_size) + "</td>\n"
                                                                "</tr>\n"
                                                                "<tr>\n"
                                                                "<td>part_name_" + to_string(i + 1) + "</td>\n"
                                                                                                      "<td>" +
                           partitions[i].part_name + "</td>\n"
                                                     "</tr>\n";
            }
        }
        int count = 0;
        if (ext) {
            vector<Structs::EBR> ebrs = disk.getlogics(extended, path);
            for (Structs::EBR ebr : ebrs) {
                content += "<tr><td BORDER=\"0\"></td><td BORDER=\"0\"></td></tr>"
                           "<tr><td COLSPAN = '2' BGCOLOR=\"#102027\"><font color=\"white\">EBR_" +
                           to_string(count + 1) +
                           "</font></td></tr>\n"
                           "<tr><td BGCOLOR=\"#ff6f00\">NOMBRE</td><td BGCOLOR=\"#ff6f00\" >VALOR</td></tr>\n"
                           "<tr>\n"
                           "<td>part_status_" + to_string(count + 1) + "</td>\n"
                                                                       "<td>" +
                           ebr.part_status + "</td>\n"
                                             "</tr>\n"
                                             "<tr>\n"
                                             "<td>part_fit_" + to_string(count + 1) + "</td>\n"
                                                                                      "<td>" +
                           ebr.part_fit + "</td>\n"
                                          "</tr>\n"
                                          "<tr>\n"
                                          "<td>part_start_" + to_string(count + 1) + "</td>\n"
                                                                                     "<td>" +
                           to_string(ebr.part_start) + "</td>\n"
                                                       "</tr>\n"
                                                       "<tr>\n"
                                                       "<td>part_size_" + to_string(count + 1) + "</td>\n"
                                                                                                 "<td>" +
                           to_string(ebr.part_size) + "</td>\n"
                                                      "</tr>\n"
                                                      "<tr>\n"
                                                      "<td>part_next" + to_string(count + 1) + "</td>\n"
                                                                                               "<td>" +
                           to_string(ebr.part_next) + "</td>\n"
                                                      "</tr>\n"
                                                      "<tr>\n"
                                                      "<td>part_name" + to_string(count + 1) + "</td>\n"
                                                                                               "<td>" +
                           ebr.part_name + "</td>\n"
                                           "</tr>\n";
                count++;
            }

        }
        content += "</table>>];";
        content += "\n\n}\n";
        ofstream outfile(pd);
        outfile << content.c_str() << endl;
        outfile.close();
        string function = "dot -T" + extOf(p) + " " + pd + " -o " + p;
        system(function.c_str());
        //function = "rm \"" + pd + "\"";
        //system(function.c_str());
        shared.response("REPORT", "generado correctamente");
    } catch (exception &e) {
        shared.handler("REPORT", e.what());
    }
}

void Report::dks(string p, string id) {
    try {
        string path;
        Structs::Partition partition = mount.getmount(id, &path);

        FILE *file = fopen(path.c_str(), "rb+");

        if (file == NULL) {
            throw runtime_error("disco no existente");
        }

        Structs::MBR mbrData;

        rewind(file);
        fread(&mbrData, sizeof(Structs::MBR), 1, file);

        fclose(file);

        string pd = p.substr(0, p.find_last_of('.')) + ".dot";

        FILE *doc = fopen(pd.c_str(), "r");
        if (doc == NULL) {
            string cmm = "mkdir -p \"" + pd + "\"";
            string cmm2 = "rmdir \"" + pd + "\"";
            system(cmm.c_str());
            system(cmm2.c_str());
        } else {
            fclose(doc);
        }

        Structs::Partition all[4] = {
            mbrData.mbr_Partition_1,
            mbrData.mbr_Partition_2,
            mbrData.mbr_Partition_3,
            mbrData.mbr_Partition_4
        };

        vector<Structs::Partition> used;

        for (int i = 0; i < 4; i++) {
            if (all[i].part_status == '1') {
                used.push_back(all[i]);
            }
        }

        sort(
            used.begin(),
            used.end(),
            [](const Structs::Partition &a,
               const Structs::Partition &b) {
                return a.part_start < b.part_start;
            }
        );

        string content =
            "digraph G{\n"
            "rankdir=TB;\n"
            "forcelabels=true;\n"
            "graph [ dpi = \"600\" ];\n"
            "node [shape=plaintext];\n";

        content += "nodo1 [label = <<table>\n<tr>\n";

        string logicRow = "<tr>\n";

        int logicCells = 0;

        int cursor = (int)sizeof(Structs::MBR);

        for (auto &pt : used) {

            int gap = pt.part_start - cursor;

            if (gap > 0) {
                float pct =
                    round(
                        ((float)gap / mbrData.mbr_tamano) * 10000.0f
                    ) / 100.0f;

                content +=
                    "<td ROWSPAN='2'>Libre\n" +
                    to_string(pct) +
                    "% del disco</td>\n";
            }

            if (pt.part_type == 'E' || pt.part_type == 'e') {

                vector<Structs::EBR> ebrs =
                    disk.getlogics(pt, path);

                int innerCursor = pt.part_start;

                for (auto &ebr : ebrs) {

                    int innerGap =
                        ebr.part_start - innerCursor;

                    if (innerGap > 0) {

                        float pctg =
                            round(
                                ((float)innerGap /
                                 mbrData.mbr_tamano) * 10000.0f
                            ) / 100.0f;

                        logicRow +=
                            "<td>Libre\n" +
                            to_string(pctg) +
                            "% del disco</td>\n";

                        logicCells++;
                    }

                    logicRow += "<td>EBR</td>\n";

                    float pctL =
                        round(
                            ((float)ebr.part_size /
                             mbrData.mbr_tamano) * 10000.0f
                        ) / 100.0f;

                    logicRow +=
                        "<td>Logica\n" +
                        to_string(pctL) +
                        "% del disco</td>\n";

                    logicCells += 2;

                    innerCursor =
                        ebr.part_start +
                        (int)sizeof(Structs::EBR) +
                        ebr.part_size;
                }

                int trailingGap =
                    (pt.part_start + pt.part_size) -
                    innerCursor;

                if (trailingGap > 0) {

                    float pctg =
                        round(
                            ((float)trailingGap /
                             mbrData.mbr_tamano) * 10000.0f
                        ) / 100.0f;

                    logicRow +=
                        "<td>Libre\n" +
                        to_string(pctg) +
                        "% del disco</td>\n";

                    logicCells++;
                }

                content +=
                    "<td COLSPAN='" +
                    to_string(max(1, logicCells)) +
                    "'>Extendida</td>\n";

            } else {

                float pct =
                    round(
                        ((float)pt.part_size /
                         mbrData.mbr_tamano) * 10000.0f
                    ) / 100.0f;

                content +=
                    "<td ROWSPAN='2'>Primaria\n" +
                    to_string(pct) +
                    "% del disco</td>\n";
            }

            cursor =
                pt.part_start +
                pt.part_size;
        }

        int finalGap =
            mbrData.mbr_tamano - cursor;

        if (finalGap > 0) {

            float pct =
                round(
                    ((float)finalGap /
                     mbrData.mbr_tamano) * 10000.0f
                ) / 100.0f;

            content +=
                "<td ROWSPAN='2'>Libre\n" +
                to_string(pct) +
                "% del disco</td>\n";
        }

        if (logicCells > 0) {
            logicRow += "</tr>\n";
            content += "</tr>\n" + logicRow + "</table>>];\n}\n";
        } else {
            content += "</tr>\n</table>>];\n}\n";
        }

        ofstream outfile(pd);
        outfile << content.c_str() << endl;
        outfile.close();

        string function =
            "dot -T" + extOf(p) +
            " " + pd + " -o " + p;

        system(function.c_str());

        shared.response(
            "REPORT",
            "generado correctamente"
        );

    } catch (exception &e) {
        shared.handler("REPORT", e.what());
    }
}

void Report::inode(string p, string id) {
    try {
        string path;
        Structs::Superblock spr;
        Structs::Inodes inode;

        Structs::Partition partition = mount.getmount(id, &path);

        FILE *file = fopen(path.c_str(), "rb+");
        if (file == NULL) {
            throw runtime_error("disco no existente");
        }

        fseek(file, partition.part_start, SEEK_SET);
        fread(&spr, sizeof(Structs::Superblock), 1, file);

        fseek(file, spr.s_inode_start, SEEK_SET);
        fread(&inode, sizeof(Structs::Inodes), 1, file);

        int freeI = fileManager.getfree(spr, path, "BI");

        string pd = p.substr(0, p.find('.'));
        pd += ".dot";
        FILE *doc = fopen(pd.c_str(), "r");
        if (doc == NULL) {
            string cmm = "mkdir -p \"" + pd + "\"";
            string cmm2 = "rmdir \"" + pd + "\"";
            system(cmm.c_str());
            system(cmm2.c_str());
        } else {
            fclose(doc);
        }

        string content;
        content = "digraph G{\n"
                  "rankdir=LR;\n"
                  "graph [ dpi = \"600\" ]; \n"
                  "forcelabels= true;\n"
                  "node [shape = plaintext];\n";

        for (int i = 0; i < freeI; ++i) {
            content += "inode" + to_string(i) + "  [label = <<table>\n"
                                                "<tr><td COLSPAN = '2' BGCOLOR=\"#145A32\"><font color=\"white\">INODO " +
                       to_string(i) + "</font></td></tr>\n"
                                      "<tr><td BGCOLOR=\"#90EE90\">NOMBRE</td><td BGCOLOR=\"#90EE90\" >VALOR</td></tr>\n"
                                      "<tr>\n"
                                      "<td>i_uid</td>\n"
                                      "<td>" +
                       to_string(inode.i_uid) + "</td>\n"
                                                "</tr>\n"
                                                "<tr>\n"
                                                "<td>i_gid</td>\n"
                                                "<td>" +
                       to_string(inode.i_gid) + "</td>\n"
                                                "</tr>\n"
                                                "<tr>\n"
                                                "<td>i_size</td>\n"
                                                "<td>" +
                       to_string(inode.i_size) + "</td>\n"
                                                 "</tr>\n"
                                                 "<tr>\n"
                                                 "<td>i_atime</td>\n"
                                                 "<td>" +
                       to_string(inode.i_atime) + "</td>\n"
                                       "</tr>\n"
                                       "<tr>\n"
                                       "<td>i_ctime</td>\n"
                                       "<td>" +
                       to_string(inode.i_ctime) + "</td>\n"
                                       "</tr>\n"
                                       "<tr>\n"
                                       "<td>i_mtime</td>\n"
                                       "<td>" +
                       to_string(inode.i_mtime) + "</td>\n"
                                       "</tr>\n";
            for (int j = 0; j < 15; ++j) {
                content += "<tr>\n"
                           "<td>i_block_" + to_string(j + 1) + "</td>\n"
                                                               "<td>" +
                           to_string(inode.i_block[j]) + "</td>\n"
                                                         "</tr>\n";
            }
            content += "<tr>\n"
                       "<td>i_type</td>\n"
                       "<td>" + to_string(inode.i_type) + "</td>\n"
                                                          "</tr>\n"
                                                          "<tr>\n"
                                                          "<td>i_perm</td>\n"
                                                          "<td>" + string(inode.i_perm, 3) + "</td>\n"
                                                                                             "</tr>\n</table>>];\n";
            if (i != 0) {
                content += "inode" + to_string(i - 1) + "-> inode" + to_string(i) + "\n";
            }

            fread(&inode, sizeof(Structs::Inodes), 1, file);
        }
        fclose(file);
        content += "\n\n}\n";
        ofstream outfile(pd);
        outfile << content.c_str() << endl;
        outfile.close();
        string function = "dot -T" + extOf(p) + " " + pd + " -o " + p;
        system(function.c_str());
        function = "rm \"" + pd + "\"";
        system(function.c_str());
        shared.response("REPORT", "generado correctamente");
    } catch (exception &e) {
        shared.handler("REPORT", e.what());
    }
}

void Report::block(string p, string id) {
    try {
        string path;
        Structs::Superblock spr;
        Structs::Inodes inodes;
        Structs::Folderblock fb;
        Structs::Fileblock flb;
        Structs::Pointerblock pb;


        Structs::Partition partition = mount.getmount(id, &path);

        FILE *file = fopen(path.c_str(), "rb+");
        if (file == NULL) {
            throw runtime_error("disco no existente");
        }

        fseek(file, partition.part_start, SEEK_SET);
        fread(&spr, sizeof(Structs::Superblock), 1, file);

        fseek(file, spr.s_inode_start, SEEK_SET);
        fread(&inodes, sizeof(Structs::Inodes), 1, file);

        int freeI = fileManager.getfree(spr, path, "BI");

        string pd = p.substr(0, p.find('.'));
        pd += ".dot";
        FILE *doc = fopen(pd.c_str(), "r");
        if (doc == NULL) {
            string cmm = "mkdir -p \"" + pd + "\"";
            string cmm2 = "rmdir \"" + pd + "\"";
            system(cmm.c_str());
            system(cmm2.c_str());
        } else {
            fclose(doc);
        }

        string content;
        content = "digraph G{\n"
                  "rankdir=LR;\n"
                  "graph [ dpi = \"600\" ]; \n"
                  "forcelabels= true;\n"
                  "node [shape = plaintext];\n";
        int last = -1;
        for (int i = 0; i < freeI; ++i) {
            for (int j = 0; j < 15; ++j) {
                if (inodes.i_block[j] != -1) {
                    if (j < 12) {
                        if (inodes.i_type == 1) {
                            fseek(file, spr.s_block_start + (sizeof(Structs::Fileblock) * inodes.i_block[j]),
                                  SEEK_SET);
                            fread(&flb, sizeof(Structs::Fileblock), 1, file);
                            content += "BLOCK" + to_string(inodes.i_block[j]) + " [label = <<table >\n"
                                                                                "<tr><td COLSPAN = '2' BGCOLOR=\"#145A32\"><font color=\"white\">BLOCK " +
                                       to_string(inodes.i_block[j]) +
                                       "</font></td></tr>\n <tr><td COLSPAN = '2'>" + flb.b_content +
                                       "</td></tr>\n</table>>];\n";
                            if (last != -1) {
                                content += "BLOCK" + to_string(last) + "-> BLOCK" + to_string(inodes.i_block[j]) + "\n";
                            }
                            last = inodes.i_block[j];
                        } else {
                            fseek(file, spr.s_block_start + (sizeof(Structs::Folderblock) * inodes.i_block[j]),
                                  SEEK_SET);
                            fread(&fb, sizeof(Structs::Fileblock), 1, file);
                            content += "BLOCK" + to_string(inodes.i_block[j]) + "  [label = <<table>\n"
                                                                                "<tr><td COLSPAN = '2' BGCOLOR=\"#145A32\"><font color=\"white\">BLOCK " +
                                       to_string(inodes.i_block[j]) + "</font></td></tr>\n"
                                                                      "<tr><td BGCOLOR=\"#90EE90\">B_NAME</td><td BGCOLOR=\"#90EE90\" >B_INODO</td></tr>\n";
                            for (int k = 0; k < 4; ++k) {
                                string ctmp;
                                ctmp += fb.b_content[k].b_name;
                                content += "<tr>\n"
                                           "<td>" + ctmp + "</td>\n"
                                                           "<td>" +
                                           to_string(fb.b_content[k].b_inodo) + "</td>\n"
                                                                                "</tr>\n";
                            }
                            content += "</table>>];\n";
                            if (last != -1) {
                                content += "BLOCK" + to_string(last) + "-> BLOCK" + to_string(inodes.i_block[j]) + "\n";
                            }
                            last = inodes.i_block[j];
                        }
                    }
                }
            }
            fseek(file, spr.s_inode_start + (sizeof(Structs::Inodes) * (i + 1)), SEEK_SET);
            fread(&inodes, sizeof(Structs::Inodes), 1, file);
        }
        fclose(file);
        content += "\n\n}\n";
        ofstream outfile(pd);
        outfile << content.c_str() << endl;
        outfile.close();
        string function = "dot -T" + extOf(p) + " " + pd + " -o " + p;
        system(function.c_str());
        function = "rm \"" + pd + "\"";
        system(function.c_str());
        shared.response("REPORT", "generado correctamente");
    } catch (exception &e) {
        shared.handler("REPORT", e.what());
    }
}

void Report::bminode(string p, string id) {
    try {
        string path;
        Structs::Superblock spr;
        int size;
        Structs::Partition partition = mount.getmount(id, &path);

        FILE *file = fopen(path.c_str(), "rb+");
        if (file == NULL) {
            throw runtime_error("disco no existente");
        }

        fseek(file, partition.part_start, SEEK_SET);
        fread(&spr, sizeof(Structs::Superblock), 1, file);
        size = spr.s_inodes_count;

        string pd = p.substr(0, p.find('.'));
        pd += ".txt";
        FILE *doc = fopen(pd.c_str(), "r");
        if (doc == NULL) {
            string cmm = "mkdir -p \"" + pd + "\"";
            string cmm2 = "rmdir \"" + pd + "\"";
            system(cmm.c_str());
            system(cmm2.c_str());
        } else {
            fclose(doc);
        }

        string content; 
        char ch = 'x';
        int count = 0;
        fseek(file, spr.s_bm_inode_start, SEEK_SET);
        for (int i = 0; i < spr.s_inodes_count; i++) {
            fread(&ch, sizeof(ch), 1, file);
            if (count < 19) {
                content += ch;
                count++;
            } else {
                content += ch;
                content += "\n";
                count = 0;
            }
        }
        fclose(file);
        ofstream outfile(pd);
        outfile << content.c_str() << endl;
        outfile.close();
        shared.response("REPORT", "generado correctamente");
    } catch (exception &e) {
        shared.handler("REPORT", e.what());
    }
}

void Report::bmblock(string p, string id) {
    try {
        string path;
        Structs::Superblock spr;
        int size;                                  
        Structs::Partition partition = mount.getmount(id, &path);

        FILE *file = fopen(path.c_str(), "rb+");
        if (file == NULL) {
            throw runtime_error("disco no existente");
        }

        fseek(file, partition.part_start, SEEK_SET);
        fread(&spr, sizeof(Structs::Superblock), 1, file);


        string pd = p.substr(0, p.find('.'));
        pd += ".txt";
        FILE *doc = fopen(pd.c_str(), "r");
        if (doc == NULL) {
            string cmm = "mkdir -p \"" + pd + "\"";
            string cmm2 = "rmdir \"" + pd + "\"";
            system(cmm.c_str());
            system(cmm2.c_str());
        } else {
            fclose(doc);
        }

        string content;
        char ch = 'x';
        int count = 0;
        fseek(file, spr.s_bm_block_start, SEEK_SET);
        for (int i = 0; i < spr.s_blocks_count; i++) {
            fread(&ch, sizeof(ch), 1, file);
            if (count < 19) {
                content += ch;
                count++;
            } else {
                content += ch;
                content += "\n";
                count = 0;
            }
        }
        fclose(file);
        ofstream outfile(pd);
        outfile << content.c_str() << endl;
        outfile.close();
        shared.response("REPORT", "generado correctamente");
    } catch (exception &e) {
        shared.handler("REPORT", e.what());
    }
}

void Report::sb(string p, string id) {
    try {
        string path;
        Structs::Superblock spr;
        int size;
        Structs::Partition partition = mount.getmount(id, &path);

        FILE *file = fopen(path.c_str(), "rb+");
        if (file == NULL) {
            throw runtime_error("disco no existente");
        }

        fseek(file, partition.part_start, SEEK_SET);
        fread(&spr, sizeof(Structs::Superblock), 1, file);
        fclose(file);

        string pd = p.substr(0, p.find('.'));
        pd += ".dot";
        FILE *doc = fopen(pd.c_str(), "r");
        if (doc == NULL) {
            string cmm = "mkdir -p \"" + pd + "\"";
            string cmm2 = "rmdir \"" + pd + "\"";
            system(cmm.c_str());
            system(cmm2.c_str());
        } else {
            fclose(doc);
        }

        string content;
        content = "digraph G{\n"
                  "rankdir=TB;\n"
                  "graph [ dpi = \"600\" ]; \n"
                  "forcelabels= true;\n"
                  "node [shape = plaintext];\n"
                  "general [label = <<table>\n"
                  "<tr><td COLSPAN = '2' BGCOLOR=\"#145A32\"><font color=\"white\">SUPERBLOCK</font></td></tr>\n"
                  "<tr><td BGCOLOR=\"#90EE90\">NOMBRE</td><td BGCOLOR=\"#90EE90\" >VALOR</td></tr>\n"
                  "<tr>\n"
                  "<td>s_inodes_count</td>\n"
                  "<td>" +
                  to_string(spr.s_inodes_count) + "</td>\n"
                                                  "</tr>\n"
                                                  "<tr>\n"
                                                  "<td>s_blocks_count</td>\n"
                                                  "<td>" +
                  to_string(spr.s_blocks_count) + "</td>\n"
                                                  "</tr>\n"
                                                  "<tr>\n"
                                                  "<td>s_free_blocks_count</td>\n"
                                                  "<td>" +
                  to_string(spr.s_free_blocks_count) + "</td>\n"
                                                       "</tr>\n"
                                                       "<tr>\n"
                                                       "<td>s_free_inodes_count</td>\n"
                                                       "<td>" +
                  to_string(spr.s_free_inodes_count) + "</td>\n"
                                                       "</tr>\n"
                                                       "<tr>\n"
                                                       "<td>s_mtime</td>\n"
                                                       "<td>" +
                  to_string(spr.s_mtime) + "</td>\n"
                                "</tr>\n"
                                "<tr>\n"
                                "<td>s_umtime</td>\n"
                                "<td>" +
                  to_string(spr.s_umtime) + "</td>\n"
                                 "</tr>\n"
                                 "<tr>\n"
                                 "<td>s_mnt_count</td>\n"
                                 "<td>" +
                  to_string(spr.s_mnt_count) + "</td>\n"
                                               "</tr>\n"
                                               "<tr>\n"
                                               "<td>s_magic</td>\n"
                                               "<td>" +
                  to_string(spr.s_magic) + "</td>\n"
                                           "</tr>\n"
                                           "<tr>\n"
                                           "<td>s_inode_size</td>\n"
                                           "<td>" +
                  to_string(spr.s_inode_size) + "</td>\n"
                                                "</tr>\n"
                                                "<tr>\n"
                                                "<td>s_block_size</td>\n"
                                                "<td>" +
                  to_string(spr.s_block_size) + "</td>\n"
                                                "</tr>\n"
                                                "<tr>\n"
                                                "<td>s_first_ino</td>\n"
                                                "<td>" +
                  to_string(spr.s_fist_ino) + "</td>\n"
                                              "</tr>\n"
                                              "<tr>\n"
                                              "<td>s_first_blo</td>\n"
                                              "<td>" +
                  to_string(spr.s_first_blo) + "</td>\n"
                                               "</tr>\n"
                                               "<tr>\n"
                                               "<td>s_bm_inode_start</td>\n"
                                               "<td>" +
                  to_string(spr.s_bm_inode_start) + "</td>\n"
                                                    "</tr>\n"
                                                    "<tr>\n"
                                                    "<td>s_bm_block_start</td>\n"
                                                    "<td>" +
                  to_string(spr.s_bm_block_start) + "</td>\n"
                                                    "</tr>\n"
                                                    "<tr>\n"
                                                    "<td>s_inode_start</td>\n"
                                                    "<td>" +
                  to_string(spr.s_inode_start) + "</td>\n"
                                                 "</tr>\n"
                                                 "<tr>\n"
                                                 "<td>s_block_start</td>\n"
                                                 "<td>" +
                  to_string(spr.s_block_start) + "</td>\n"
                                                 "</tr>\n";

        content += "</table>>];";
        content += "\n\n}\n";
        ofstream outfile(pd);
        outfile << content.c_str() << endl;
        outfile.close();
        string function = "dot -T" + extOf(p) + " " + pd + " -o " + p;
        system(function.c_str());
        function = "rm \"" + pd + "\"";
        system(function.c_str());
        shared.response("REPORT", "generado correctamente");
    } catch (exception &e) {
        shared.handler("REPORT", e.what());
    }
}

void Report::tree(string p, string id) {
    try {
        string path;
        Structs::Superblock spr;
        Structs::Inodes inode;

        Structs::Partition partition = mount.getmount(id, &path);

        FILE *file = fopen(path.c_str(), "rb+");
        if (file == NULL) {
            throw runtime_error("disco no existente");
        }

        fseek(file, partition.part_start, SEEK_SET);
        fread(&spr, sizeof(Structs::Superblock), 1, file);

        fseek(file, spr.s_inode_start, SEEK_SET);
        fread(&inode, sizeof(Structs::Inodes), 1, file);

        int freeI = fileManager.getfree(spr, path, "BI");

        string pd = p.substr(0, p.find('.'));
        pd += ".dot";
        FILE *doc = fopen(pd.c_str(), "r");
        if (doc == NULL) {
            string cmm = "mkdir -p \"" + pd + "\"";
            string cmm2 = "rmdir \"" + pd + "\"";
            system(cmm.c_str());
            system(cmm2.c_str());
        } else {
            fclose(doc);
        }

        string content;
        content = "digraph G{\n"
                  "rankdir=LR;\n"
                  "graph [ dpi = \"600\" ]; \n"
                  "forcelabels= true;\n"
                  "node [shape = plaintext];\n";

        for (int i = 0; i < freeI; ++i) {
            content += "inode" + to_string(i) + "  [label = <<table>\n"
                                                "<tr><td COLSPAN = '2' BGCOLOR=\"#000080\"><font color=\"white\">INODO " +
                       to_string(i) + "</font></td></tr>\n"
                                      "<tr><td BGCOLOR=\"#87CEFA\">NOMBRE</td><td BGCOLOR=\"#87CEFA\" >VALOR</td></tr>\n"
                                      "<tr>\n"
                                      "<td>i_uid</td>\n"
                                      "<td>" +
                       to_string(inode.i_uid) + "</td>\n"
                                                "</tr>\n"
                                                "<tr>\n"
                                                "<td>i_gid</td>\n"
                                                "<td>" +
                       to_string(inode.i_gid) + "</td>\n"
                                                "</tr>\n"
                                                "<tr>\n"
                                                "<td>i_size</td>\n"
                                                "<td>" +
                       to_string(inode.i_size) + "</td>\n"
                                                 "</tr>\n"
                                                 "<tr>\n"
                                                 "<td>i_atime</td>\n"
                                                 "<td>" +
                       to_string(inode.i_atime) + "</td>\n"
                                       "</tr>\n"
                                       "<tr>\n"
                                       "<td>i_ctime</td>\n"
                                       "<td>" +
                       to_string(inode.i_ctime) + "</td>\n"
                                       "</tr>\n"
                                       "<tr>\n"
                                       "<td>i_mtime</td>\n"
                                       "<td>" +
                       to_string(inode.i_mtime) + "</td>\n"
                                       "</tr>\n";
            for (int j = 0; j < 15; ++j) {
                content += "<tr>\n"
                           "<td>i_block_" + to_string(j + 1) + "</td>\n"
                                                               "<td port=\"" + to_string(j) + "\">" +
                           to_string(inode.i_block[j]) + "</td>\n"
                                                         "</tr>\n";
            }
            content += "<tr>\n"
                       "<td>i_type</td>\n"
                       "<td>" + to_string(inode.i_type) + "</td>\n"
                                                          "</tr>\n"
                                                          "<tr>\n"
                                                          "<td>i_perm</td>\n"
                                                          "<td>" + string(inode.i_perm, 3) + "</td>\n"
                                                                                             "</tr>\n</table>>];\n";
            if (inode.i_type == 0) {
                for (int j = 0; j < 15; j++) {
                    if (inode.i_block[j] != -1) {
                        content +=
                                "inode" + to_string(i) + ":" + to_string(j) + "-> BLOCK" + to_string(inode.i_block[j]) +
                                "\n";

                        Structs::Folderblock foldertmp;
                        fseek(file, spr.s_block_start + (sizeof(Structs::Folderblock) * inode.i_block[j]),
                              SEEK_SET);
                        fread(&foldertmp, sizeof(Structs::Folderblock), 1, file);

                        content += "BLOCK" + to_string(inode.i_block[j]) + "  [label = <<table>\n"
                                                                           "<tr><td COLSPAN = '2' BGCOLOR=\"#145A32\"><font color=\"white\">BLOCK " +
                                   to_string(inode.i_block[j]) + "</font></td></tr>\n"
                                                                 "<tr><td BGCOLOR=\"#90EE90\">B_NAME</td><td BGCOLOR=\"#90EE90\" >B_INODO</td></tr>\n";
                        for (int k = 0; k < 4; ++k) {
                            string ctmp;
                            ctmp += foldertmp.b_content[k].b_name;
                            content += "<tr>\n"
                                       "<td>" + ctmp + "</td>\n"
                                                       "<td port=\"" + to_string(k) + "\">" +
                                       to_string(foldertmp.b_content[k].b_inodo) + "</td>\n"
                                                                                   "</tr>\n";
                        }
                        content += "</table>>];\n";

                        for (int b = 0; b < 4; b++) { //VER SI ELIMINO
                            if (foldertmp.b_content[b].b_inodo != -1) {
                                string nm(foldertmp.b_content[b].b_name);
                                if (!((nm == ".") || (nm == ".."))) {
                                    content +=
                                            "BLOCK" + to_string(inode.i_block[j]) + ":" + to_string(b) + " -> inode" +
                                            to_string(foldertmp.b_content[b].b_inodo) + ";\n";
                                }
                            }
                        }

                        if (j > 11) {
                            //Metodo para graficar bloques indirectos
                        }
                    }
                }
            } else {
                for (int j = 0; j < 15; j++) {
                    if (inode.i_block[j] != -1) {
                        if (j < 12) {
                            content +=
                                    "inode" + to_string(i) + ":" + to_string(j) + "-> BLOCK" +
                                    to_string(inode.i_block[j]) +
                                    "\n";
                            Structs::Fileblock filetmp;
                            fseek(file, spr.s_block_start + (sizeof(Structs::Fileblock) * inode.i_block[j]),
                                  SEEK_SET);
                            fread(&filetmp, sizeof(Structs::Fileblock), 1, file);

                            content += "BLOCK" + to_string(inode.i_block[j]) + " [label = <<table >\n"
                                                                               "<tr><td COLSPAN = '2' BGCOLOR=\"#CCCC00\">BLOCK " +
                                       to_string(inode.i_block[j]) +
                                       "</td></tr>\n <tr><td COLSPAN = '2'>" + filetmp.b_content +
                                       "</td></tr>\n</table>>];\n";
                        }
                    }
                }
            }
            fseek(file, spr.s_inode_start + (sizeof(Structs::Inodes) * (i + 1)), SEEK_SET);
            fread(&inode, sizeof(Structs::Inodes), 1, file);
        }
        fclose(file);
        content += "\n\n}\n";

        ofstream outfile(pd);
        outfile << content.c_str() << endl;
        outfile.close();
        string function = "dot -T" + extOf(p) + " " + pd + " -o " + p;
        system(function.c_str());
        function = "rm \"" + pd + "\"";
        system(function.c_str());
        shared.response("REPORT", "generado correctamente");
    } catch (exception &e) {
        shared.handler("REPORT", e.what());
    }
}

void Report::journaling(string p, string id) {
    try {
        string path;
        Structs::Superblock spr;
        Structs::Journaling journaling;

        Structs::Partition partition = mount.getmount(id, &path);

        FILE *file = fopen(path.c_str(), "rb+");
        if (file == NULL) {
            throw runtime_error("disco no existente");
        }

        fseek(file, spr.s_inode_start, SEEK_SET);
        fread(&spr, sizeof(Structs::Superblock), 1, file);
        if (spr.s_filesystem_type == 2) {
            throw runtime_error("sistema no válido");
        }

        fseek(file, partition.part_start + sizeof(Structs::Superblock), SEEK_SET);
        fread(&journaling, sizeof(Structs::Inodes), 1, file);

        string pd = p.substr(0, p.find('.'));
        pd += ".dot";
        FILE *doc = fopen(pd.c_str(), "r");
        if (doc == NULL) {
            string cmm = "mkdir -p \"" + pd + "\"";
            string cmm2 = "rmdir \"" + pd + "\"";
            system(cmm.c_str());
            system(cmm2.c_str());
        } else {
            fclose(doc);
        }

        string content;
        content = "digraph G{\n"
                  "rankdir=LR;\n"
                  "graph [ dpi = \"600\" ]; \n"
                  "forcelabels= true;\n"
                  "node [shape = plaintext];\n";
        int i = 0;
        do {
            if (journaling.type == -1){
                break;
            }
            content += "jor" + to_string(i) + "  [label = <<table>\n"
                                                "<tr><td COLSPAN = '2' BGCOLOR=\"#145A32\"><font color=\"white\">JOURNALING " +
                       to_string(i) + "</font></td></tr>\n"
                                      "<tr><td BGCOLOR=\"#90EE90\">NOMBRE</td><td BGCOLOR=\"#90EE90\" >VALOR</td></tr>\n"
                                      "<tr>\n"
                                      "<td>operation</td>\n"
                                      "<td>" +
                       journaling.operation + "</td>\n"
                                                "</tr>\n"
                                                "<tr>\n"
                                                "<td>type</td>\n"
                                                "<td>" +
                       to_string(journaling.type) + "</td>\n"
                                                "</tr>\n"
                                                "<tr>\n"
                                                "<td>path</td>\n"
                                                "<td>" +
                       journaling.path + "</td>\n"
                                                 "</tr>\n"
                                                 "<tr>\n"
                                                 "<td>content</td>\n"
                                                 "<td>" +
                    journaling.content + "</td>\n"
                                       "</tr>\n"
                                       "<tr>\n"
                                       "<td>date</td>\n"
                                       "<td>" +
                    to_string(journaling.date) + "</td>\n"
                                       "</tr>\n"
                                       "<tr>\n"
                                       "<td>size</td>\n"
                                       "<td>" +
                    to_string(journaling.size)  + "</td>\n"
                                       "</tr>\n</table>>];\n";
       
            
            if (i != 0) {
                content += "jor" + to_string(i - 1) + "-> jor" + to_string(i) + "\n";
            }
            i++;
            fread(&journaling, sizeof(Structs::Journaling), 1, file);
        } while (true);
        fclose(file);
        content += "\n\n}\n";
        ofstream outfile(pd);
        outfile << content.c_str() << endl;
        outfile.close();
        string function = "dot -T" + extOf(p) + " " + pd + " -o " + p;
        system(function.c_str());
        function = "rm \"" + pd + "\"";
        system(function.c_str());
        shared.response("REPORT", "generado correctamente");
    } catch (exception &e) {
        shared.handler("REPORT", e.what());
    }
}

string Report::permString(char type, char perm[3]) {
    string result;

    result += (type == 0) ? 'd' : '-';

    for (int i = 0; i < 3; i++) {
        int val = perm[i] - '0';

        result += (val & 4) ? 'r' : '-';
        result += (val & 2) ? 'w' : '-';
        result += (val & 1) ? 'x' : '-';
    }

    return result;
}

void Report::resolveOwnerGroup(
    FILE *file,
    Structs::Superblock spr,
    string diskPath,
    int uid,
    int gid,
    string &ownerName,
    string &groupName
) {
    ownerName = "?";
    groupName = "?";

    try {

        int usersInode =
            fileManager.findInode(
                file,
                spr,
                "/users.txt"
            );

        if (usersInode == -1)
            return;

        Structs::Inodes inode =
            fileManager.getInode(
                file,
                spr,
                usersInode
            );

        string txt =
            fileManager.getFileContent(
                file,
                spr,
                inode
            );

        stringstream ss(txt);
        string line;

        while (getline(ss, line)) {

            if (line.length() < 3)
                continue;

            vector<string> parts;

            stringstream ls(line);
            string field;

            while (getline(ls, field, ','))
                parts.push_back(field);

            if (parts.size() < 3)
                continue;

            if (
                (line[2] == 'U' || line[2] == 'u') &&
                parts.size() >= 4
            ) {

                if (stoi(parts[0]) == uid)
                    ownerName = parts[3];

            } else if (
                line[2] == 'G' ||
                line[2] == 'g'
            ) {

                if (stoi(parts[0]) == gid)
                    groupName = parts[2];
            }
        }

    } catch (exception &e) {
        // Mantener "?" si ocurre algún problema.
    }
}

void Report::file(string p, string id, string filePath) {
    try {

        if (filePath.empty()) {
            throw runtime_error(
                "se requiere -path_file_ls para este reporte"
            );
        }

        string path;

        Structs::Partition partition =
            mount.getmount(id, &path);

        FILE *f = fopen(path.c_str(), "rb+");

        if (f == NULL)
            throw runtime_error("disco no existente");

        Structs::Superblock spr;

        fseek(
            f,
            partition.part_start,
            SEEK_SET
        );

        fread(
            &spr,
            sizeof(Structs::Superblock),
            1,
            f
        );

        int inodeNum =
            fileManager.findInode(
                f,
                spr,
                filePath
            );

        if (inodeNum == -1) {
            fclose(f);
            throw runtime_error(
                "no existe el archivo indicado"
            );
        }

        Structs::Inodes inode =
            fileManager.getInode(
                f,
                spr,
                inodeNum
            );

        if (inode.i_type != 1) {
            fclose(f);
            throw runtime_error(
                "la ruta indicada no corresponde a un archivo"
            );
        }

        string content =
            fileManager.getFileContent(
                f,
                spr,
                inode
            );

        fclose(f);

        string pd = p;
        FILE *doc = fopen(pd.c_str(), "r");
        if (doc == NULL) {
            string cmm = "mkdir -p \"" + pd + "\"";
            string cmm2 = "rmdir \"" + pd + "\"";
            system(cmm.c_str());
            system(cmm2.c_str());
        } else {
            fclose(doc);
        }

        ofstream outfile(p);
        outfile << content;
        outfile.close();

        shared.response(
            "REPORT",
            "generado correctamente"
        );

    } catch (exception &e) {
        shared.handler("REPORT", e.what());
    }
}

void Report::ls(string p, string id, string lsPath) {
    try {

        if (lsPath.empty()) {
            throw runtime_error(
                "se requiere -path_file_ls para este reporte"
            );
        }

        string path;

        Structs::Partition partition =
            mount.getmount(id, &path);

        FILE *f = fopen(path.c_str(), "rb+");

        if (f == NULL)
            throw runtime_error("disco no existente");

        Structs::Superblock spr;

        fseek(
            f,
            partition.part_start,
            SEEK_SET
        );

        fread(
            &spr,
            sizeof(Structs::Superblock),
            1,
            f
        );

        int inodeNum =
            fileManager.findInode(
                f,
                spr,
                lsPath
            );

        if (inodeNum == -1) {
            fclose(f);
            throw runtime_error(
                "no existe la ruta indicada"
            );
        }

        Structs::Inodes targetInode =
            fileManager.getInode(
                f,
                spr,
                inodeNum
            );

        string pd =
            p.substr(
                0,
                p.find_last_of('.')
            ) + ".dot";

        FILE *doc = fopen(pd.c_str(), "r");
        if (doc == NULL) {
            string cmm = "mkdir -p \"" + pd + "\"";
            string cmm2 = "rmdir \"" + pd + "\"";
            system(cmm.c_str());
            system(cmm2.c_str());
        } else {
            fclose(doc);
        }

        string content =
            "digraph G{\n"
            "rankdir=TB;\n"
            "forcelabels=true;\n"
            "graph [ dpi = \"600\" ];\n"
            "node [shape=plaintext];\n";

        content +=
            "tabla [label = <<table>\n"
            "<tr>"
            "<td BGCOLOR=\"#ff6f00\">Permisos</td>"
            "<td BGCOLOR=\"#ff6f00\">Owner</td>"
            "<td BGCOLOR=\"#ff6f00\">Grupo</td>"
            "<td BGCOLOR=\"#ff6f00\">Size</td>"
            "<td BGCOLOR=\"#ff6f00\">Fecha</td>"
            "<td BGCOLOR=\"#ff6f00\">Hora</td>"
            "<td BGCOLOR=\"#ff6f00\">Tipo</td>"
            "<td BGCOLOR=\"#ff6f00\">Name</td>"
            "</tr>\n";

        auto addRow =
            [&](string name, Structs::Inodes inode) {

            string owner;
            string group;

            resolveOwnerGroup(
                f,
                spr,
                path,
                inode.i_uid,
                inode.i_gid,
                owner,
                group
            );

            time_t mt = inode.i_mtime;

            struct tm *tmv =
                localtime(&mt);

            char fecha[16];
            char hora[16];

            strftime(
                fecha,
                16,
                "%Y/%m/%d",
                tmv
            );

            strftime(
                hora,
                16,
                "%H:%M:%S",
                tmv
            );

            string tipo =
                (inode.i_type == 0)
                ? "Carpeta"
                : "Archivo";

            string perms =
                permString(
                    inode.i_type,
                    inode.i_perm
                );

            content +=
                "<tr>"
                "<td>" + perms + "</td>"
                "<td>" + owner + "</td>"
                "<td>" + group + "</td>"
                "<td>" + to_string(inode.i_size) + "</td>"
                "<td>" + string(fecha) + "</td>"
                "<td>" + string(hora) + "</td>"
                "<td>" + tipo + "</td>"
                "<td>" + name + "</td>"
                "</tr>\n";
        };

        if (targetInode.i_type == 0) {

            vector<Structs::Content> entries =
                fileManager.listFolder(
                    f,
                    spr,
                    targetInode
                );

            for (auto &e : entries) {

                Structs::Inodes childInode =
                    fileManager.getInode(
                        f,
                        spr,
                        e.b_inodo
                    );

                addRow(
                    string(e.b_name),
                    childInode
                );
            }

        } else {

            vector<string> comps =
                fileManager.getpath(lsPath);

            string name =
                comps.empty()
                ? lsPath
                : comps.back();

            addRow(
                name,
                targetInode
            );
        }

        fclose(f);

        content +=
            "</table>>];\n"
            "}\n";

        ofstream outfile(pd);
        outfile << content.c_str() << endl;
        outfile.close();

        string function =
            "dot -T" + extOf(p) +
            " " + pd + " -o " + p;

        system(function.c_str());

        shared.response(
            "REPORT",
            "generado correctamente"
        );

    } catch (exception &e) {
        shared.handler("REPORT", e.what());
    }
}
