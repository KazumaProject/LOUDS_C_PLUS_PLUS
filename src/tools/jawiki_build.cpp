#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <filesystem>

#include <zlib.h>

#include "../prefix/prefix_tree.hpp"
#include "../prefix_with_term_id/prefix_tree_with_term_id.hpp"

#include "../louds/converter.hpp"
#include "../louds/louds.hpp"

#include "../louds_with_term_id/converter_with_term_id.hpp"
#include "../louds_with_term_id/louds_with_term_id.hpp"

namespace fs = std::filesystem;

// -----------------------------
// UTF-8 -> UTF-32 (char32_t) decoder (strict, minimal, no ICU)
// -----------------------------
static bool utf8_to_u32(const std::string &s, std::u32string &out)
{
    out.clear();
    out.reserve(s.size());

    const unsigned char *p = reinterpret_cast<const unsigned char *>(s.data());
    size_t i = 0;
    const size_t n = s.size();

    while (i < n)
    {
        unsigned char c = p[i];

        if (c <= 0x7F)
        {
            out.push_back(static_cast<char32_t>(c));
            i += 1;
            continue;
        }

        // Determine length
        int len = 0;
        char32_t cp = 0;

        if ((c & 0xE0) == 0xC0)
        {
            len = 2;
            cp = c & 0x1F;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            len = 3;
            cp = c & 0x0F;
        }
        else if ((c & 0xF8) == 0xF0)
        {
            len = 4;
            cp = c & 0x07;
        }
        else
        {
            return false;
        }

        if (i + static_cast<size_t>(len) > n)
            return false;

        for (int k = 1; k < len; ++k)
        {
            unsigned char cc = p[i + static_cast<size_t>(k)];
            if ((cc & 0xC0) != 0x80)
                return false;
            cp = (cp << 6) | (cc & 0x3F);
        }

        // Reject overlong encodings and invalid ranges
        if (len == 2 && cp < 0x80)
            return false;
        if (len == 3 && cp < 0x800)
            return false;
        if (len == 4 && cp < 0x10000)
            return false;

        // Surrogates are invalid in UTF-8
        if (cp >= 0xD800 && cp <= 0xDFFF)
            return false;

        // Max Unicode scalar value
        if (cp > 0x10FFFF)
            return false;

        out.push_back(cp);
        i += static_cast<size_t>(len);
    }
    return true;
}

// -----------------------------
// gz line reader (handles long lines by accumulating chunks)
// -----------------------------
static bool gz_read_line(gzFile f, std::string &line)
{
    line.clear();
    const int BUF = 1 << 15; // 32768
    char buf[BUF];

    while (true)
    {
        char *r = gzgets(f, buf, BUF);
        if (!r)
        {
            // EOF or error
            if (line.empty())
                return false;
            return true;
        }

        line.append(r);

        // If newline found, we got a full line
        if (!line.empty() && line.back() == '\n')
        {
            // trim \n and optional \r
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
            {
                line.pop_back();
            }
            return true;
        }

        // else: buffer filled without newline, continue
    }
}

// -----------------------------
// Simple CLI args
// -----------------------------
struct Args
{
    std::string input_gz;
    std::string out_dir = "out";
    std::string prefix = "jawiki_latest";
    uint64_t limit = 0; // 0 = no limit
};

static void usage_and_exit(const char *prog)
{
    std::cerr
        << "Usage:\n"
        << "  " << prog << " --input <jawiki-*-all-titles-in-ns0.gz> --out-dir <dir> --prefix <name> [--limit N]\n";
    std::exit(2);
}

static Args parse_args(int argc, char **argv)
{
    Args a;
    for (int i = 1; i < argc; ++i)
    {
        std::string k = argv[i];
        auto need = [&](const char *opt) -> std::string
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Missing value for " << opt << "\n";
                usage_and_exit(argv[0]);
            }
            return std::string(argv[++i]);
        };

        if (k == "--input")
            a.input_gz = need("--input");
        else if (k == "--out-dir")
            a.out_dir = need("--out-dir");
        else if (k == "--prefix")
            a.prefix = need("--prefix");
        else if (k == "--limit")
        {
            std::string v = need("--limit");
            a.limit = static_cast<uint64_t>(std::stoull(v));
        }
        else
        {
            std::cerr << "Unknown option: " << k << "\n";
            usage_and_exit(argv[0]);
        }
    }
    if (a.input_gz.empty())
        usage_and_exit(argv[0]);
    return a;
}

// -----------------------------
// Bytes formatter (human readable)
// -----------------------------
static std::string format_bytes(uint64_t bytes)
{
    const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    double v = static_cast<double>(bytes);
    int u = 0;
    while (v >= 1024.0 && u < 4)
    {
        v /= 1024.0;
        ++u;
    }
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(2);
    oss << v << " " << units[u];
    return oss.str();
}

// -----------------------------
// Metrics JSON writer (no external deps)
// -----------------------------
static void write_metrics_json(
    const fs::path &path,
    uint64_t word_count,
    uint64_t char_count,
    uint64_t input_gz_bytes,
    uint64_t input_utf8_bytes_total,
    double seconds_total,
    double seconds_convert_louds,
    double seconds_convert_louds_termid)
{
    std::ofstream ofs(path);
    if (!ofs)
    {
        throw std::runtime_error("failed to open metrics.json for write: " + path.string());
    }

    ofs << "{\n";
    ofs << "  \"word_count\": " << word_count << ",\n";
    ofs << "  \"char_count\": " << char_count << ",\n";
    ofs << "  \"input_gz_bytes\": " << input_gz_bytes << ",\n";
    ofs << "  \"input_utf8_bytes_total\": " << input_utf8_bytes_total << ",\n";
    ofs << "  \"seconds_total\": " << seconds_total << ",\n";
    ofs << "  \"seconds_convert_louds\": " << seconds_convert_louds << ",\n";
    ofs << "  \"seconds_convert_louds_with_term_id\": " << seconds_convert_louds_termid << "\n";
    ofs << "}\n";
}

int main(int argc, char **argv)
{
    try
    {
        Args args = parse_args(argc, argv);

        fs::create_directories(args.out_dir);

        const fs::path out_dir(args.out_dir);
        const fs::path out_louds = out_dir / (args.prefix + ".louds.bin");
        const fs::path out_louds_termid = out_dir / (args.prefix + ".louds_termid.bin");
        const fs::path out_metrics = out_dir / "metrics.json";

        auto t_begin = std::chrono::steady_clock::now();

        // Input gzip file size (compressed size)
        uint64_t input_gz_bytes = 0;
        try
        {
            input_gz_bytes = static_cast<uint64_t>(fs::file_size(args.input_gz));
        }
        catch (...)
        {
            input_gz_bytes = 0;
        }

        // 1) Read titles and build tries
        PrefixTree trie;
        PrefixTreeWithTermId trie_termid;

        uint64_t word_count = 0;
        uint64_t char_count = 0;

        // Total UTF-8 bytes read (approx. uncompressed bytes without newline)
        uint64_t input_utf8_bytes_total = 0;

        gzFile f = gzopen(args.input_gz.c_str(), "rb");
        if (!f)
        {
            std::cerr << "Failed to open gz: " << args.input_gz << "\n";
            return 1;
        }

        std::string line;
        std::u32string u32;
        while (gz_read_line(f, line))
        {
            if (args.limit != 0 && word_count >= args.limit)
                break;
            if (line.empty())
                continue;

            // Accumulate bytes (line already trimmed of \n/\r in gz_read_line)
            input_utf8_bytes_total += static_cast<uint64_t>(line.size());
            // If you want to estimate including newline, you can add:
            // input_utf8_bytes_total += 1;

            if (!utf8_to_u32(line, u32))
            {
                // If decoding fails, skip this line but keep moving
                continue;
            }

            // Count
            word_count += 1;
            char_count += static_cast<uint64_t>(u32.size());

            // Insert to both tries
            trie.insert(u32);
            trie_termid.insert(u32);
        }
        gzclose(f);

        // 2) Convert -> LOUDS
        Converter conv;
        auto t1 = std::chrono::steady_clock::now();
        LOUDS louds = conv.convert(trie.getRoot());
        auto t2 = std::chrono::steady_clock::now();
        double seconds_convert_louds = std::chrono::duration<double>(t2 - t1).count();

        // 3) Convert -> LOUDSWithTermId
        ConverterWithTermId conv_termid;
        auto t3 = std::chrono::steady_clock::now();
        LOUDSWithTermId louds_termid = conv_termid.convert(trie_termid.getRoot());
        auto t4 = std::chrono::steady_clock::now();
        double seconds_convert_termid = std::chrono::duration<double>(t4 - t3).count();

        // 4) Save
        louds.saveToFile(out_louds.string());
        louds_termid.saveToFile(out_louds_termid.string());

        auto t_end = std::chrono::steady_clock::now();
        double seconds_total = std::chrono::duration<double>(t_end - t_begin).count();

        // 5) Write metrics
        write_metrics_json(
            out_metrics,
            word_count,
            char_count,
            input_gz_bytes,
            input_utf8_bytes_total,
            seconds_total,
            seconds_convert_louds,
            seconds_convert_termid);

        // Console summary (Actions log)
        std::cout << "input_gz_bytes=" << input_gz_bytes;
        if (input_gz_bytes != 0)
        {
            std::cout << " (" << format_bytes(input_gz_bytes) << ")";
        }
        std::cout << "\n";

        std::cout << "input_utf8_bytes_total=" << input_utf8_bytes_total
                  << " (" << format_bytes(input_utf8_bytes_total) << ")\n";

        std::cout << "word_count=" << word_count << "\n";
        std::cout << "char_count=" << char_count << "\n";
        std::cout << "seconds_total=" << seconds_total << "\n";
        std::cout << "seconds_convert_louds=" << seconds_convert_louds << "\n";
        std::cout << "seconds_convert_louds_with_term_id=" << seconds_convert_termid << "\n";
        std::cout << "out_louds=" << out_louds.string() << "\n";
        std::cout << "out_louds_termid=" << out_louds_termid.string() << "\n";
        std::cout << "out_metrics=" << out_metrics.string() << "\n";

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return 1;
    }
}
