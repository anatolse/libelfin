
#include "dwarf++.hh"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <io.h>

#include <fstream>
#include <cstdint>
#include <vector>
#include <array>
#include <map>

using namespace std;

void
dump_line_table(const dwarf::line_table &lt)
{
        for (auto &line : lt) {
                if (line.end_sequence)
                        printf("\n");
                else
                        printf("%-40s%8d%#20" PRIx64 "\n", line.file->path.c_str(),
                               line.line, line.address);
        }
}

class wasm_loader : public dwarf::loader
{
public:
    using ByteBuffer = std::vector<uint8_t>;
    using ByteBufferIt = ByteBuffer::const_iterator;
    struct section
    {
        std::string name;
        ByteBufferIt start;
        ByteBufferIt end;
        const uint8_t* data() const
        {
            return &(*start);
        }
        size_t size() const 
        {
            return std::distance(start, end);
        }
    };

    template <typename T, bool bSigned>
    T ReadInternal(ByteBufferIt& it)
    {
        static_assert(!std::numeric_limits<T>::is_signed); // the sign flag must be specified separately

        T ret = 0;
        for (unsigned int nShift = 0; ; )
        {
            uint8_t n = Read1(it);
            bool bEnd = !(0x80 & n);
            if (!bEnd)
                n &= ~0x80;

            ret |= T(n) << nShift;
            nShift += 7;

            if (bEnd)
            {
                if constexpr (bSigned)
                {
                    if (0x40 & n)
                        ret |= (~static_cast<T>(0) << nShift);
                }
                break;
            }

            //Test(nShift < sizeof(ret) * 8);
        }

        return ret;
    }

    uint8_t Read1(ByteBufferIt& it)
    {
        return Consume<uint8_t>(it);
    }

    template<typename T>
    T Consume(ByteBufferIt& it) 
    {
         T res = *reinterpret_cast<const T*>(&(*it));
         std::advance(it, sizeof(T));
         return res;
    };
    wasm_loader(const std::string& path)
    {
        {
            std::fstream f(path, std::ios_base::binary | std::ios_base::in);
            f.seekg(0, std::ios_base::end);
            size_t size = f.tellg();
            f.seekg(0);
            m_buffer.resize(size);
            f.read(reinterpret_cast<char*>(&m_buffer[0]), size);
        }
        constexpr std::array<uint8_t, 4> sig = {0, 'a', 's', 'm'};
        constexpr std::array<uint8_t, 4> version = { 1, 0, 0, 0 };
        auto it = m_buffer.begin();
        if (!std::equal(sig.begin(), sig.end(), it))
            throw std::runtime_error("");
        std::advance(it, sig.size());
        if (!std::equal(version.begin(), version.end(), it))
            throw std::runtime_error("");
        std::advance(it, version.size());
        
        while (it != m_buffer.end())
        {
            uint8_t type = Consume<uint8_t>(it);
            uint32_t size = ReadInternal<uint32_t, false>(it);
            auto end = it + size;

            if (type == 0)
            {
                auto& s = m_sections.emplace_back();
                uint32_t nameSize = ReadInternal<uint32_t, false>(it);
                s.start = it + nameSize;
                s.end = end;
                s.name = { it, it + nameSize };
            }
            
            it = end;
        }
        

    }

    const void* load(dwarf::section_type section, size_t* size_out)
    {
        const auto& name = dwarf::elf::section_type_to_name(section);
        auto it = std::find_if(m_sections.begin(), m_sections.end(),
            [&](const auto& s)
        {
            return s.name == name;
        });

        if (it == m_sections.end())
            return nullptr;

        *size_out = it->size();
        return it->data();
    }
private:
    ByteBuffer m_buffer;
    std::vector<section> m_sections;
};

int
main(int argc, char **argv)
{
        if (argc != 2) {
                fprintf(stderr, "usage: %s elf-file\n", argv[0]);
                return 2;
        }

        //int fd = open(argv[1], O_RDONLY);
        //if (fd < 0) {
        //        fprintf(stderr, "%s: %s\n", argv[1], strerror(errno));
        //        return 1;
        //}

        //elf::elf ef(elf::create_mmap_loader(fd));
        //dwarf::dwarf dw(dwarf::elf::create_loader(ef));
        dwarf::dwarf dw(std::make_shared<wasm_loader>(argv[1]));
        for (auto cu : dw.compilation_units()) {
                printf("--- <%x>\n", (unsigned int)cu.get_section_offset());
                dump_line_table(cu.get_line_table());
                printf("\n");
        }

        return 0;
}
