/**
 * mmap based string acquisition.
 */
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <utility>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>

#define BENCHMARK_ACQUIRE_INPUT(data)  if(argc < 2) { exit(-1); } benchmark::input data(argv[1]);
#define BENCHMARK_GET_ITERATIONS(iter) long iter = (argc > 2) ? benchmark::iterations(argv[2]) : 1;
#define BENCHMARK_ITERATE(input, iter) for (int i_ = 0; i_ < iter; ++i_, input.reset_())
#define BENCHMARK_FOREACH(cstring)     for(const char *cstring; not input.eof_() and (cstring = input.next_().first); /* Empty */)

#if _POSIX_C_SOURCE >= 1 or _XOPEN_SOURCE or _POSIX_SOURCE or _BSD_SOURCE or _SVID_SOURCE
#define PUTCHAR putchar_unlocked
#else // Standard I/O
#define PUTCHAR putchar
#endif

namespace benchmark
{

long iterations(const char* const argv)
{
    const long iter = labs(strtol(argv, 0, 10));
    if (iter == 0 or errno)
    {
        exit(EINVAL);
    }
    return iter;
}

class input
{
public:
    typedef char* iterator;
    typedef const std::pair<const iterator, size_t> record;

    input(const char* const file_name) : m_fd(open(file_name, O_RDONLY))
    {
        struct stat buf;
        if (m_fd == -1 or fstat(m_fd, &buf) == -1)
        {
            exit(errno);
        }

        m_begin = static_cast<iterator>(mmap(0, buf.st_size, PROT_READ, MAP_PRIVATE, m_fd, 0));
        if (m_begin == iterator(-1))
        {
            exit(errno);
        }

        m_end = m_begin + buf.st_size;
        m_position = m_begin;

        if (m_end <= m_begin or m_end[-1] != '\0')
        {
            exit(EFBIG); // file must be non-empty and last byte must be NUL.
        }
    }

    inline bool eof_() const
    {
        return (m_position >= m_end);
    }

    inline void reset_()
    {
        m_position = m_begin;
    }

    inline record next_()
    {
        const iterator at = m_position;
        if (at < m_end)
        {
            m_position = static_cast<const iterator>(rawmemchr(m_position, '\0'));
            if (m_position == 0)
            {
                m_position = m_end;
            }
        }
        return std::make_pair(at, m_position++ - at);
    }

    ~input()
    {
        close(m_fd);
        munmap(m_begin, m_end - m_begin);
    }

private:
    iterator m_position;
    iterator m_end;
    iterator m_begin;
    int m_fd;
};
} // benchmark namespace
