#ifndef AF_INI_PARSE_H__
#define AF_INI_PARSE_H__

#include <assert.h>
#include <stdint.h>

// linux
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// simd
#include <immintrin.h>

namespace af
{

using u32 = uint32_t;
using u64 = uint64_t;
using u8x32 = __m256i;

class string_view
{
public:
  string_view() = default;
  string_view(const char* data, u64 size)
    : m_data(data)
    , m_size(size)
  {
  }

  const char* data() const { return m_data; }
  u64 size() const { return m_size; }

private:
  const char* m_data = nullptr;
  u64 m_size = 0;
};

/**
 * @brief Defer something till the end of a scope
 */
template<typename Func>
struct scope_guard
{
public:
  template<typename AtExit>
  scope_guard(AtExit atexit) noexcept
    : mAtExit(atexit)
  {
  }

  ~scope_guard() noexcept { mAtExit(); }

private:
  Func mAtExit;
};

template<typename AtExit>
inline scope_guard<AtExit>
make_scope_guard(AtExit atexit)
{
  return scope_guard<AtExit>(atexit);
}

enum class ini_parse_status
{
  ok = 0,
  // The file doesn't exist
  invalid_file_cant_open_read = 1,
  // The file contains an un-closed section `[ ...`
  invalid_section_unmatched_token,
  // The file contains an empty section `[]`
  invalid_section_empty,
  // Empty key
  invalid_key_empty,
  // Empty value
  invalid_value_empty,
  // Unable to mmap the file
  invalid_mapping
};

constexpr const char*
ini_status_string(ini_parse_status s)
{
  switch (s)
  {
    case ini_parse_status::ok:
      return "ok";
    case ini_parse_status::invalid_file_cant_open_read:
      return "can't open file for read";
    case ini_parse_status::invalid_section_unmatched_token:
      return "invalid section: unmatched token";
    case ini_parse_status::invalid_section_empty:
      return "invalid section: empty";
    case ini_parse_status::invalid_key_empty:
      return "invalid key: empty";
    case ini_parse_status::invalid_value_empty:
      return "invalid value: empty";
    case ini_parse_status::invalid_mapping:
      return "invalid mapping: can't mmap";
    default:
      break;
  }
  return "unknown error";
}

template<typename ValueCallback>
ini_parse_status
ini_parse(const char* b,
          const char* const e,
          ValueCallback&& cb) // TODO: we shoulf forward
{
  // clang-format off
  const u8x32 separators = _mm256_setr_epi8(
    ' ', 0, 0, 0, 0, 0, 0, 0, 0, '\t', '\n', 0, 0, '=', 0, 0,
    ' ', 0, 0, 0, 0, 0, 0, 0, 0, '\t', '\n', 0, 0, '=', 0, 0);
  // clang-format on

  while (b < e)
  {
    // parse the file sequentially, reading at least 64 chars at a time
    // we can safely read 32 bytes extra, we made sure of that
    const u8x32 text0 = _mm256_loadu_si256((u8x32*)b);
    const u8x32 text1 = _mm256_loadu_si256((u8x32*)(b + 32));
    // use this trick from perf-ninja (challenge 6 solutions), clever!
    // thanks Robert Burke
    const u8x32 seps0 = _mm256_shuffle_epi8(separators, text0);
    const u8x32 seps1 = _mm256_shuffle_epi8(separators, text1);
    const u8x32 eq0 = _mm256_cmpeq_epi8(text0, seps0);
    const u8x32 eq1 = _mm256_cmpeq_epi8(text1, seps1);

    [[maybe_unused]] u64 separators_mask =
      ((u64)(u32)_mm256_movemask_epi8(eq0)) |
      (((u64)_mm256_movemask_epi8(eq1)) << 32);

    while (separators_mask)
    {
      const int first_word_size = __builtin_ctzll(separators_mask);

      string_view key(b, first_word_size);
      separators_mask >>= first_word_size + 1;

      const int second_word_size = __builtin_ctzll(separators_mask);

      string_view value(b + first_word_size, second_word_size);
      separators_mask >>= second_word_size + 1;

      cb(string_view{}, key, value);
    }
    b += 64;
  }

  return ini_parse_status::ok;
}

/**
 * @brief Calls the user provided callback whenever a key/value is available
 * @pre .ini file with value len < 1791, key len < 256, section len < 512
 * @param path .ini file path
 * @param cb user provided (with optional state) callback
 * @return error code describind the success/failure
 */
template<typename ValueCallback>
ini_parse_status
ini_read_and_parse(const char* path, ValueCallback&& cb)
{
  void* memory = nullptr;
  u64 memory_size = 0;
  u64 file_size = 0;

  const auto defer_unmap = make_scope_guard(
    [memory, &memory_size]()
    {
      if (memory_size != 0)
      {
        ::munmap(memory, memory_size);
      }
    });

  // get the file into memory
  {
    // try to open the file
    const int fd = ::open(path, O_RDONLY);
    if (fd == -1)
    {
      return ini_parse_status::invalid_file_cant_open_read;
    }

    const auto defer_closefile = make_scope_guard([fd]() { ::close(fd); });

    struct stat sb;
    if (::fstat(fd, &sb) == -1)
    {
      return ini_parse_status::invalid_file_cant_open_read;
    }

    // constexpr u64 small_page_size = 4 * 1'024;
    constexpr u64 huge_page_size = 2 * 1'024 * 1'024;

    file_size = sb.st_size;

    const u64 file_size_page_aligned =
      ((file_size + 1ul * huge_page_size) / huge_page_size) * huge_page_size;

    const auto get_memory = [](u64 size, int huge_tlb) -> void*
    {
      return ::mmap(nullptr,
                    size,
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | huge_tlb,
                    -1,
                    0);
    };

    // get some memory, which we can align at 2MB, instead of the usual 4KB
    // first, let's try HUGETLB
    memory = get_memory(file_size_page_aligned, MAP_HUGETLB);
    if (memory == MAP_FAILED)
    {
      // try small pages 4KB
      memory = get_memory(file_size_page_aligned, 0);
      if (memory == MAP_FAILED)
      {
        return ini_parse_status::invalid_mapping;
      }
    }

    memory_size = file_size_page_aligned;

    // now let's mmap the file at the beginning
    if (void* mapped_file = ::mmap(memory,
                                   file_size_page_aligned,
                                   PROT_READ | PROT_WRITE,
                                   MAP_FIXED | MAP_PRIVATE,
                                   fd,
                                   0);
        mapped_file == MAP_FAILED)
    {
      return ini_parse_status::invalid_mapping;
    }
    else
    {
      // tell the kernel what we're doing
      ::madvise(mapped_file, file_size_page_aligned, MADV_SEQUENTIAL);
    }
    // let's pad the file with zeros
    char* zero_out = static_cast<char*>(memory) + file_size;
    for (u64 i = 0; i < 128; ++i)
    {
      // we could vectorize this ourselves, but let's see if the compiler can do
      // it for us, of course we could use memset, which is probably some
      // intrinsic
      zero_out[i] = 0;
    }
  }

  const char* b = static_cast<const char*>(memory);
  const char* e = b + file_size;

  return ini_parse(b, e, cb);
}

} // namespace af

#endif // AF_INI_PARSE_H__
