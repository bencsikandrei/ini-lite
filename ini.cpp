#include <cstdio>
#include <cassert>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

typedef unsigned long long u64;

struct afb_string_span
{
    const char* data;
    u64 size;
};

enum afb_status_code
{
    afb_status_ok = 0,
    afb_status_invalid_section_name,
    afb_status_unmatched_token,
    afb_status_invalid_key_value_pair,
    afb_status_invalid_value
};

typedef bool (*afb_parse_callback)(afb_string_span, afb_string_span, afb_string_span, void* user_data);

constexpr afb_status_code afb_parse_ini(const char* padded_ini, u64 padded_ini_size, afb_parse_callback parse_cb, void* user_data)
{
    const char* begin = padded_ini;
    const char* it = begin;
    const char* end = padded_ini + padded_ini_size;
    // remove space

    constexpr const auto is_space = [](char c) { return c == ' ' || c == '\t'; };
    constexpr const auto ltrim = [is_space](const char* it, const char* end) {
        while (it != end && is_space(*(it))) ++it;
        return it;
    };
    // assert(!is_space(*it));
    afb_string_span current_section{};
    afb_string_span current_key{};
    afb_string_span current_value{};
    do {
        while(it != end && is_space(*it)) ++it;
        if (it == end)
        {
            return afb_status_ok;
        };
        switch (*it)
        {
            case '\n': 
                ++it;
                break;
                // comment
            case '#': case ';':
                while(it != end && *(it++) != '\n');

                break;
                // start section
            case '[':
                ++it;
                it = ltrim(it, end);
                if (it != end) 
                {
                    const char* section_begin = it;
                    while (it != end && *it != ']' && *it != '\n') ++it;
                    if (it != end)
                    {
                        if (*it == ']')
                        {
                            current_section = { section_begin, u64(it - section_begin) };
                            ++it;
                            it = ltrim(it, end);
                            if (it != end && *it != '\n')
                            {
                                return afb_status_invalid_section_name;
                            }
                        }
                        else {
                            // error, unfinished section
                            return afb_status_unmatched_token;
                        }
                    }
                }
                else
                {
                    // error, unfinished section
                    return afb_status_unmatched_token;
                }
                break;
                // normal line key=value
            default:
                {
                    // search for equal, has to be before \n
                    // we should have something like: key=value (with optional spaces)
                    const char* line_begin = it;
                    while (it != end && *it != '=' && *it != '\n') ++it;
                    if (it == end || *it == '\n')
                    {
                        return afb_status_invalid_key_value_pair;
                    }
                    current_key =  { line_begin, u64(it - line_begin) };
                    ++it; // skip the =
                    const char* value_begin = it;
                    // let's ignore trailing spaces for now
                    while (it != end && *it != '\n') ++it;
                    if (it == end)
                    {
                        return afb_status_invalid_value;
                    }
                    current_value = { value_begin, u64(it - value_begin) };
                    parse_cb(current_section, current_key, current_value, user_data);
                    break;
                }
        }
    } while (it!=end);

    return afb_status_ok;
}

static constexpr const char* k_ini = R"(
[multi]
a = value
b = value


)";

static_assert(afb_parse_ini(k_ini, sizeof(k_ini) / sizeof(char) - 1, [](afb_string_span, afb_string_span, afb_string_span, void*) { return true; }, nullptr) == afb_status_ok);



int main(int argc, char** argv)
{
    if (argc < 2)
    {
        puts("Provide file name");
        return 1;
    }

    const int fd = open(argv[1], O_RDONLY);
    if (fd == -1)
    {
        puts("Can't open file");
        return 1;
    }
    struct CloseFile
    {
        int fd;
        bool enable = true;;
        ~CloseFile()
        {
            if (enable)
            close(fd);
        }
    };
    CloseFile _{fd};

    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        puts("Can't open file");
        return 1;
    }

    void* addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED)
    {
        puts("Can't mmap file");
        return 1;
    }
    _.enable = false;
    close(fd);
   
    const afb_status_code rc =  afb_parse_ini((const char*)addr, sb.st_size, [](afb_string_span section, afb_string_span key, afb_string_span value, void* data) { 
            putchar('[');
            fwrite(section.data, sizeof(char), section.size, stdout);
            puts("]");
            fwrite(key.data, sizeof(char), key.size, stdout);
            putchar('=');
            fwrite(value.data, sizeof(char), value.size, stdout);
            puts("");
            return true;
            }, nullptr);
    if (rc != afb_status_ok)
    {
        puts("Error, invalid ini");
        munmap(addr, sb.st_size);
        return 1;
    }
    munmap(addr, sb.st_size);
}

/*
#define handle_error(msg) \
do { perror(msg); exit(EXIT_FAILURE); } while (0)

int
main(int argc, char *argv[])
{
char *addr;
int fd;
off_t offset, pa_offset;
size_t length;
ssize_t s;

if (argc < 3 || argc > 4) {
fprintf(stderr, "%s file offset [length]\n", argv[0]);
exit(EXIT_FAILURE);
}



handle_error("mmap");

s = write(STDOUT_FILENO, addr + offset - pa_offset, length);
if (s != length) {
if (s == -1)
handle_error("write");

fprintf(stderr, "partial write");
exit(EXIT_FAILURE);
}

close(fd);
*/
