/*
 * Copyright (C) 2024-present, Guanyou.Chen. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "logger/log.h"
#include "command/core/cmd_read.h"
#include "command/command_manager.h"
#include "base/utils.h"
#include "base/memory_map.h"
#include "api/core.h"
#include "common/bit.h"
#include "common/disassemble/capstone.h"
#include <unistd.h>
#include <getopt.h>
#include <iomanip>

int ReadCommand::main(int argc, char* const argv[]) {
    if (!CoreApi::IsReady() || !(argc > 1))
        return 0;

    int opt;
    uint64_t begin = Utils::atol(argv[1]) & CoreApi::GetVabitsMask();
    uint64_t end = 0;
    char* filepath = nullptr;
    int read_opt = OPT_READ_ALL;
    bool dump_inst = false;
    bool dump_string = false;

    int option_index = 0;
    optind = 0; // reset
    static struct option long_options[] = {
        {"origin",  no_argument,       0,  1 },
        {"mmap",    no_argument,       0,  2 },
        {"overlay", no_argument,       0,  3 },
        {"end",     required_argument, 0, 'e'},
        {"file",    required_argument, 0, 'f'},
        {"inst",    no_argument,       0, 'i'},
        {"string",  no_argument,       0, 's'},
        {0,         0,                 0,  0 },
    };
    
    while ((opt = getopt_long(argc, argv, "e:f:0123is",
                long_options, &option_index)) != -1) {
        switch (opt) {
            case 'e':
                end = Utils::atol(optarg) & CoreApi::GetVabitsMask();
                break;
            case 'f':
                filepath = optarg;
                break;
            case 1:
                read_opt = OPT_READ_OR;
                break;
            case 2:
                read_opt = OPT_READ_MMAP;
                break;
            case 3:
                read_opt = OPT_READ_OVERLAY;
                break;
            case 'i':
                dump_inst = true;
                break;
            case 's':
                dump_string = true;
                break;
        }
    }

    LoadBlock* block = CoreApi::FindLoadBlock(begin);
    if (block && end > (block->vaddr() + block->size()))
        end = block->vaddr() + block->size();

    // always alloc align 0x10
    int count = RoundUp(RoundUp(end - begin, 8) / 8, 2);
    if (begin >= end || !count) {
        uint64_t* value = reinterpret_cast<uint64_t *>(CoreApi::GetReal(begin, read_opt));
        if (value) {
            if (!filepath) {
                if (!dump_inst && !dump_string) {
                    std::string ascii = Utils::ConvertAscii(*value, 8);
                    LOGI(ANSI_COLOR_CYAN "%" PRIx64 "" ANSI_COLOR_RESET ": %016" PRIx64 "  %s\n", begin, (*value), ascii.c_str());
                } else if (dump_inst) {
                    capstone::Disassember::Option opt(begin, 1);
                    capstone::Disassember::Dump("", (uint8_t *)value, 8, begin, opt);
                } else if (dump_string) {
                    LOGI(ANSI_COLOR_CYAN "%s" ANSI_COLOR_RESET "\n", reinterpret_cast<const char *>(value));
                }
            } else {
                saveBinary(filepath, value, end - begin);
            }
        }
    } else {
        std::unique_ptr<MemoryMap> map(MemoryMap::MmapZeroMem(count * 8));
        if (!map) {
            LOGE("no vma!!\n");
            return 0;
        }
        if (CoreApi::Read(begin, count * 8, (uint8_t*)map->data(), read_opt)) {
            uint64_t* value = reinterpret_cast<uint64_t *>(map->data());
            if (!filepath) {
                if (!dump_inst && !dump_string) {
                    for (int i = 0; i < count; i += 2) {
                        LOGI(ANSI_COLOR_CYAN "%" PRIx64 "" ANSI_COLOR_RESET ": %016" PRIx64 "  %016" PRIx64 "  %s%s\n", (begin + i * 8), value[i], value[i + 1],
                                Utils::ConvertAscii(value[i], 8).c_str(), Utils::ConvertAscii(value[i + 1], 8).c_str());
                    }
                } else if (dump_inst) {
                    capstone::Disassember::Option opt(begin, -1);
                    capstone::Disassember::Dump("", (uint8_t *)value, count * 8, begin, opt);
                } else if (dump_string) {
                    LOGI(ANSI_COLOR_CYAN "%s" ANSI_COLOR_RESET "\n", reinterpret_cast<const char *>(value));
                }
            } else {
                saveBinary(filepath, value, end - begin);
            }
        }
    }
    return 0;
}

void ReadCommand::saveBinary(char* path, uint64_t* real, uint64_t size) {
    if (path) {
        FILE* fp = fopen(path, "wb");
        if (fp) {
            fwrite(real, size, 1, fp);
            fclose(fp);
            LOGI("Saved [%s].\n", path);
        }
    }
}

void ReadCommand::ShowBuffer(uint64_t ptr, int buffer_size) {
    if (buffer_size) {
        int argc = 4;
        std::string bs = Utils::ToHex(ptr);
        std::string es = Utils::ToHex(ptr + buffer_size);
        char* argv[4] = {
            const_cast<char*>("rd"),
            const_cast<char*>(bs.c_str()),
            const_cast<char*>("-e"),
            const_cast<char*>(es.c_str())};
        CommandManager::Execute(argv[0], argc, argv);
    }
}

void ReadCommand::usage() {
    LOGI("Usage: read|rd <BEGIN_ADDR> [OPTION..]\n");
    LOGI("Priority: overlay > mmap > origin\n");
    LOGI("Option:\n");
    LOGI("    -e, --end <END_ADDR>   read [BEGIN, END) memory content\n");
    LOGI("        --origin           read memory content from corefile\n");
    LOGI("        --mmap             read memory content from file mmap\n");
    LOGI("        --overlay          read memory content form overwirte\n");
    LOGI("    -i, --inst             read memory content convert asm code\n");
    LOGI("    -s, --string           read memory content convert string\n");
    LOGI("    -f, --file <PATH>      read memory binary save to output file\n");
    ENTER();
    LOGI("core-parser> rd 75d9a3fa8000 -e 75d9a3fa8020\n");
    LOGI("75d9a3fa8000: 0000000000000000  0202020202020202  ................\n");
    LOGI("75d9a3fa8010: 0202020202020202  0230020202020202  ..............0.\n");
    ENTER();
    LOGI("core-parser> rd 75d9a3fa8000 -e 75d9a3fa8020 --origin\n");
    LOGI("75d9a3fa8000: 0202020202020202  0202020202020202  ................\n");
    LOGI("75d9a3fa8010: 0202020202020202  0230020202020202  ..............0.\n");
    ENTER();
    LOGI("core-parser> rd 0x71907db5 -e 0x71907ddd -i\n");
    LOGI("0x71907db5:                   2057ff | call qword ptr [rdi + 0x20]\n");
    LOGI("0x71907db8:                     db85 | test ebx, ebx\n");
    LOGI("0x71907dba:             00000018840f | je 0x71907dd8\n");
    LOGI("0x71907dc0:                   de8948 | mov rsi, rbx\n");
    LOGI("0x71907dc3:                     3e8b | mov edi, dword ptr [rsi]\n");
    LOGI("0x71907dc5:               0000ebbeb8 | mov eax, 0xebbe\n");
    LOGI("0x71907dca:           00000080bf8b48 | mov rdi, qword ptr [rdi + 0x80]\n");
    LOGI("0x71907dd1:                 207f8b48 | mov rdi, qword ptr [rdi + 0x20]\n");
    LOGI("0x71907dd5:                   2057ff | call qword ptr [rdi + 0x20]\n");
    LOGI("0x71907dd8:                 78c48348 | add rsp, 0x78\n");
    LOGI("0x71907ddc:                       5b | pop rbx\n");
    LOGI("0x71907ddd:                       5d | pop rbp\n");
    ENTER();
    LOGI("core-parser> rd 791804999000 -e 7918049ac000 -f libGLESv2_emulation.so\n");
    LOGI("Saved [libGLESv2_emulation.so].\n");
}

