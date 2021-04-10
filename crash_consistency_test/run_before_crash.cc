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
std::map<std::string, int > opened_pos;

static std::string generate_po_name() {
    std::string ret;
    do {
        ret = std::to_string(rand());
    } while (existing_pos.find(ret) != existing_pos.end());
    return ret;
}

static std::string remove_a_po_from_existing_pos() {
    if (existing_pos.size() == 0)
        return "";

    std::map<std::string, std::vector<unsigned long>* >::iterator it;
    it = existing_pos.begin();
    std::string ret = it->first;
    if (opened_pos.find(ret) != opened_pos.end()) {
        assert(po_close(opened_pos[ret]) >= 0);
        opened_pos.erase(opened_pos.find(ret));
    }
    delete it->second;
    existing_pos.erase(it);
    return ret;
}

static std::string get_a_closed_po_from_existing_pos() {
    if (existing_pos.size() == 0)
        return "";

    std::map<std::string, std::vector<unsigned long>* >::iterator it;
    it = existing_pos.begin();
    std::string ret = it->first;
    if (opened_pos.find(ret) != opened_pos.end()) {
        assert(po_close(opened_pos[ret]) >= 0);
        opened_pos.erase(opened_pos.find(ret));
    }
    return ret;
}

static int remove_a_po_from_opened_pos() {
    if (opened_pos.size() == 0)
        return -1;

    std::map<std::string, int >::iterator it;
    it = opened_pos.begin();
    int ret = it->second;
    opened_pos.erase(it);
    return ret;
}

static std::string get_a_po_from_existing_pos() {
    if (existing_pos.size() == 0)
        return "";

    std::map<std::string, std::vector<unsigned long>* >::iterator it;
    it = existing_pos.begin();
    std::string ret = it->first;
    return ret;
}

static std::string get_a_po_from_opened_pos() {
    if (opened_pos.size() == 0)
        return "";

    std::map<std::string, int >::iterator it;
    it = opened_pos.begin();
    std::string ret = it->first;
    return ret;
}

static std::string get_a_extended_po_from_opened_pos() {
    if (opened_pos.size() == 0)
        return "";

    std::map<std::string, int >::iterator it;
    for (it = opened_pos.begin(); it != opened_pos.end(); it++) {
        if (existing_pos[it->first]->size() != 0)
            break;
    }
    if (it == opened_pos.end())
        return "";
    std::string ret = it->first;
    return ret;
}

int main() {
    existing_pos.clear();
    srand(time(NULL));

    std::ofstream op_log(OP_LOG_NAME, std::ios::out);
    while (1) {
        int syscall_num = rand()%11;
        switch (syscall_num)
        {

        /* po_creat */
        case 0: {
	        std::string po_name;
	        int pod;
	        do {
                po_name = generate_po_name();
                pod = po_creat(po_name.c_str(), 0);
            } while (pod < 0);
            existing_pos[po_name] = new std::vector<unsigned long>();
            opened_pos[po_name] = pod;
            op_log << "0 " << po_name << std::endl;
            op_log.flush();
            break;
        }
        /* po_unlink */
        case 1: {
            std::string po_name = remove_a_po_from_existing_pos();
            if (po_name.size() == 0)
                break;
            assert(po_unlink(po_name.c_str()) >= 0);
            op_log << "1 " << po_name << std::endl;
            op_log.flush();
            break;
        }
        /* po_open */
        case 2: {
            std::string po_name = get_a_closed_po_from_existing_pos();
            if (po_name.size() == 0)
                break;
            int pod = po_open(po_name.c_str(), O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
            assert(pod >= 0);
            opened_pos[po_name] = pod;
            break;
        }
        /* po_close */
        case 3: {
            int pod = remove_a_po_from_opened_pos();
            if (pod < 0)
                break;
            assert(po_close(pod) >= 0);
            break;
        }
        /* po_chunk_mmap */
        case 4: {
            std::string po_name = get_a_extended_po_from_opened_pos();
            if (po_name.size() == 0)
                break;
            unsigned long addr = (*existing_pos[po_name])[0];
            assert(po_chunk_mmap(opened_pos[po_name], addr, PROT_READ | PROT_WRITE, MAP_PRIVATE) == addr);
            break;
        }
        /* po_chunk_munmap */
        case 5: {
            std::string po_name = get_a_extended_po_from_opened_pos();
            if (po_name.size() == 0)
                break;
            unsigned long addr = (*existing_pos[po_name])[0];
            assert(po_chunk_munmap(addr) >= 0);
            break;
        }
        /* po_extend */
        case 6: {
            std::string po_name = get_a_po_from_opened_pos();
            if (po_name.size() == 0)
                break;
            unsigned long addr = po_extend(opened_pos[po_name], 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE);
            assert(addr >= 0);
            op_log << "6 " << po_name << " " << std::to_string(addr) << std::endl;
            op_log.flush();
            existing_pos[po_name]->push_back(addr);
            break;
        }
        /* po_shrink */
        case 7: {
            std::string po_name = get_a_extended_po_from_opened_pos();
            if (po_name.size() == 0)
                break;
            unsigned long addr = (*existing_pos[po_name])[0];
            assert(po_shrink(opened_pos[po_name], addr, 4096) >= 0);
            op_log << "7 " << po_name << " " << std::to_string(addr) << std::endl;
            op_log.flush();
            existing_pos[po_name]->erase(existing_pos[po_name]->begin());
            break;
        }
        /* po_stat */
        case 8: {
            std::string po_name = get_a_po_from_existing_pos();
            if (po_name.size() == 0)
                break;
            struct po_stat stat;
            int ret = po_stat(po_name.c_str(), &stat);
            assert(ret >= 0);
            break;
        }
        /* po_fstat */
        case 9: {
            std::string po_name = get_a_po_from_opened_pos();
            if (po_name.size() == 0)
                break;
            struct po_stat stat;
            int ret = po_fstat(opened_pos[po_name], &stat);
            assert(ret >= 0);
            break;
        }
        /* po_chunk_next */
        case 10: {
            std::string po_name = get_a_po_from_opened_pos();
            if (po_name.size() == 0)
                break;
            std::vector<unsigned long> *chunks = existing_pos[po_name];
            unsigned long addrbuf[chunks->size()];
            int ret = po_chunk_next(opened_pos[po_name], (unsigned long)NULL, chunks->size(), addrbuf);
            assert(ret >= 0);
            for (int i=0; i<chunks->size(); i++) {
                assert((*chunks)[i] == addrbuf[i]);
            }
            break;
        }
        default:
            break;
        }
    }
    return 0;
}
