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

// UTF-16版（あなたの実装に合わせてパス/名前は調整してください）
#include "../prefix/prefix_tree_utf16.hpp"
#include "../prefix_with_term_id/prefix_tree_with_term_id_utf16.hpp"

#include "../louds/louds_converter_utf16.hpp"
#include "../louds/louds_utf16_writer.hpp"

#include "../louds_with_term_id/converter_with_term_id_utf16.hpp"
#include "../louds_with_term_id/louds_with_term_id_utf16_writer.hpp"

namespace fs = std::filesystem;

// -----------------------------
// UTF-8 -> UTF-16 (strict, minimal, no ICU)
// - validates UTF-8
// - rejects surrogates
// - converts scalar values > 0xFFFF into surrogate pairs
// -----------------------------
static bool utf8_to_u16(const std::string &s, std::u16string &out)
{
    out.clear();
    out.reserve(s.size()); // rough

    const unsigned char *p = reinterpret_cast<const unsigned char *>(s.data());
    size_t i = 0;
    const size_t n = s.size();

    while (i < n)
    {
        unsigned char c = p[i];

        // ASCII
        if (c <= 0x7F)
        {
            out.push_back(static_cast<char16_t>(c));
            i += 1;
            continue;
        }

        // Determine length and initial bits
        int len = 0;
        char32_t cp = 0;

        if ((c & 0xE0) == 0xC0)
        {
            len = 2;
            cp = static_cast<char32_t>(c & 0x1F);
        }
        else if ((c & 0xF0) == 0xE0)
        {
            len = 3;
            cp = static_cast<char32_t>(c & 0x0F);
        }
        else if ((c & 0xF8) == 0xF0)
        {
            len = 4;
            cp = static_cast<char32_t>(c & 0x07);
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
            cp = (cp << 6) | static_cast<char32_t>(cc & 0x3F);
        }

        // Reject overlong encodings
        if (len == 2 && cp < 0x80)
            return false;
        if (len == 3 && cp < 0x800)
            return false;
        if (len == 4 && cp < 0x10000)
            return false;

        // Surrogates are invalid as scalar values
        if (cp >= 0xD800 && cp <= 0xDFFF)
            return false;

        // Max Unicode scalar value
        if (cp > 0x10FFFF)
            return false;

        // Encode to UTF-16
        if (cp <= 0xFFFF)
        {
            out.push_back(static_cast<char16_t>(cp));
        }
        else
        {
            // surrogate pair
            cp -= 0x10000;
            char16_t hi = static_cast<char16_t>(0xD800 + ((cp >> 10) & 0x3FF));
            char16_t lo = static_cast<char16_t>(0xDC00 + (cp & 0x3FF));
            out.push_back(hi);
            out.push_back(lo);
        }

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
    std::string prefix = "jawiki_latest_utf16";
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
// 注意: char_count は UTF-16 の code unit 数（サロゲートペアは2）
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
        const fs::path out_louds = out_dir / (args.prefix + ".louds_utf16.bin");
        const fs::path out_louds_termid = out_dir / (args.prefix + ".louds_termid_utf16.bin");
        const fs::path out_metrics = out_dir / "metrics_utf16.json";

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

        // 1) Read titles and build tries (UTF-16)
        PrefixTreeUtf16 trie;
        PrefixTreeWithTermIdUtf16 trie_termid;

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
        std::u16string u16;
        while (gz_read_line(f, line))
        {
            if (args.limit != 0 && word_count >= args.limit)
                break;
            if (line.empty())
                continue;

            input_utf8_bytes_total += static_cast<uint64_t>(line.size());

            if (!utf8_to_u16(line, u16))
            {
                // If decoding fails, skip this line but keep moving
                continue;
            }

            word_count += 1;
            char_count += static_cast<uint64_t>(u16.size()); // UTF-16 code units

            trie.insert(u16);
            trie_termid.insert(u16);
        }
        gzclose(f);

        // 2) Convert -> LOUDS (UTF-16)
        ConverterUtf16 conv;
        auto t1 = std::chrono::steady_clock::now();
        LOUDSUtf16 louds = conv.convert(trie.getRoot());
        auto t2 = std::chrono::steady_clock::now();
        double seconds_convert_louds = std::chrono::duration<double>(t2 - t1).count();

        // 3) Convert -> LOUDSWithTermId (UTF-16)
        ConverterWithTermIdUtf16 conv_termid;
        auto t3 = std::chrono::steady_clock::now();
        LOUDSWithTermIdUtf16 louds_termid = conv_termid.convert(trie_termid.getRoot());
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
        std::cout << "char_count=" << char_count << " (UTF-16 code units)\n";
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
