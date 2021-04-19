#ifndef EASTLAKE_H_
#include "eastlake.h"
#endif

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <time.h>
#include <string>
#include <map>
#include <vector>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#define OP_LOG_NAME "op_log"

std::map<std::string, std::vector<unsigned long>* > existing_pos;

bool check_is_ok() {
    std::map<std::string, std::vector<unsigned long>* >::iterator it;
    for (it=existing_pos.begin(); it!=existing_pos.end(); it++) {
        // std::cout << it->first << " " << it->second->size() << std::endl;
        if (it->second->size() != 0) {
            int pod = po_open(it->first.c_str(), O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
            assert(pod >= 0);
            unsigned long addrbuf[it->second->size()];
            int ret = po_chunk_next(pod, (unsigned long)NULL, it->second->size(), addrbuf);
            assert(ret >= 0);
            for (int i=0; i<it->second->size(); i++)
                assert((*it->second)[i] == addrbuf[i]);
        }
    }
    return true;
}

int main() {
    existing_pos.clear();

    std::ifstream op_log(OP_LOG_NAME, std::ios::in);
    while (!op_log.eof()) {
        int syscall_num;
        op_log >> syscall_num;
        switch (syscall_num)
        {
        /* po_creat */
        case 0: {
	        unsigned long po_name;
            op_log >> po_name;
            existing_pos[std::to_string(po_name)] = new std::vector<unsigned long>();
            break;
        }
        /* po_unlink */
        case 1: {
            unsigned long po_name;
            op_log >> po_name;
            delete existing_pos[std::to_string(po_name)];
            existing_pos.erase(existing_pos.find(std::to_string(po_name)));
            break;
        }
        /* po_open */
        case 2: {
            break;
        }
        /* po_close */
        case 3: {
            break;
        }
        /* po_chunk_mmap */
        case 4: {
            break;
        }
        /* po_chunk_munmap */
        case 5: {
            break;
        }
        /* po_extend */
        case 6: {
            unsigned long po_name;
            op_log >> po_name;
            unsigned long addr;
            op_log >> addr;
            break;
        }
        /* po_shrink */
        case 7: {
            unsigned long po_name;
            op_log >> po_name;
            unsigned long addr;
            op_log >> addr;
            for (int i=0; i<existing_pos[std::to_string(po_name)]->size(); i++) {
                if ((*existing_pos[std::to_string(po_name)])[i] == addr) {
                    existing_pos[std::to_string(po_name)]->erase(existing_pos[std::to_string(po_name)]->begin() + i);
                    break;
                }
            }
            break;
        }
        /* po_stat */
        case 8: {
            break;
        }
        /* po_fstat */
        case 9: {
            break;
        }
        /* po_chunk_next */
        case 10: {
            break;
        }
        default:
            break;
        }
    }
    op_log.close();
    if (!check_is_ok()) {
        std::cout << "check failed" << std::endl;
    }
    return 0;
}
