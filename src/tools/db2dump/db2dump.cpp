// db2dump tool: converts TrinityCore static DB2 files (WDC5/WOWSTATIC) into
// SQL DDL+DML files, using the server's own per-table loadinfo metadata so that
// column names, types and localized strings are correct.
//
// Reading replicates DB2FileLoader's proven AutoProduceData/AutoProduceStrings
// path (per-section, skipping unknown TactId sections and null records) instead
// of the positional DB2Record accessor, which is not safe for dense iteration.
//
// Each table is processed through an SEH guard so that a single malformed or
// misaligned file is logged and skipped rather than aborting the whole batch.
//
// Usage: db2dump <dataDir> <outDir> [onlyTableBasename]

#include "DB2FileLoader.h"
#include "DB2FileSystemSource.h"
#include "DB2LoadInfo.h"
#include "Common.h"
#include "db2tables.generated.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <eh.h>
#endif

namespace fs = std::filesystem;

static char const* g_currentFile = "";

static std::string SanitizeName(std::string const& in)
{
    std::string out;
    out.reserve(in.size());
    for (char c : in)
        out += (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) ? c : '_';
    if (out.empty() || (out[0] >= '0' && out[0] <= '9'))
        out = "_" + out;
    return out;
}

static std::string SqlType(DBCFormer type)
{
    switch (type)
    {
        case FT_STRING:
        case FT_STRING_NOT_LOCALIZED:
            return "TEXT";
        case FT_FLOAT:
            return "DOUBLE";
        // Widen integer types so raw DB2 values never overflow a signed column.
        // FT_BYTE may hold 0..255 (or -1), FT_SHORT up to 65535, FT_INT holds
        // full uint32 IDs, FT_LONG full uint64.
        case FT_LONG:
            return "BIGINT UNSIGNED";
        case FT_INT:
            return "BIGINT";
        case FT_SHORT:
            return "INT";
        case FT_BYTE:
            return "SMALLINT";
        default:
            return "BLOB";
    }
}

static std::string SqlEscape(std::string const& s)
{
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s)
    {
        if (c == '\'')
            out += "''";
        else if (c == '\\')
            out += "\\\\";
        else if (c == '\0')
            ;
        else
            out += c;
    }
    return out;
}

// Byte width of a field in the produced data table (see AutoProduceStrings).
static uint32 FieldSize(DBCFormer type)
{
    switch (type)
    {
        case FT_FLOAT:
        case FT_INT:
            return 4;
        case FT_BYTE:
            return 1;
        case FT_SHORT:
            return 2;
        case FT_LONG:
            return 8;
        case FT_STRING:
            return sizeof(LocalizedString);
        case FT_STRING_NOT_LOCALIZED:
            return sizeof(char const*);
        default:
            return 8;
    }
}

static bool IsString(DBCFormer t) { return t == FT_STRING || t == FT_STRING_NOT_LOCALIZED; }

static void AppendValue(std::string& out, char const* p, DBCFormer type, bool isSigned, LocaleConstant locale)
{
    switch (type)
    {
        case FT_FLOAT:
        {
            float v;
            std::memcpy(&v, p, sizeof(v));
            if (std::isnan(v) || std::isinf(v))
                out += "NULL";
            else
            {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%.9g", double(v));
                out += buf;
            }
            break;
        }
        case FT_LONG:
        {
            uint64 v;
            std::memcpy(&v, p, sizeof(v));
            out += std::to_string(v);
            break;
        }
        case FT_INT:
        {
            uint32 v;
            std::memcpy(&v, p, sizeof(v));
            if (isSigned)
                out += std::to_string(static_cast<int32>(v));
            else
                out += std::to_string(v);
            break;
        }
        case FT_SHORT:
        {
            uint16 v;
            std::memcpy(&v, p, sizeof(v));
            if (isSigned)
                out += std::to_string(static_cast<int16>(v));
            else
                out += std::to_string(v);
            break;
        }
        case FT_BYTE:
        {
            uint8 v;
            std::memcpy(&v, p, sizeof(v));
            out += std::to_string(uint32(v));
            break;
        }
        case FT_STRING:
        {
            LocalizedString const* ls = reinterpret_cast<LocalizedString const*>(p);
            char const* s = ls->Str[locale];
            out += s ? ("'" + SqlEscape(s) + "'") : "NULL";
            break;
        }
        case FT_STRING_NOT_LOCALIZED:
        {
            char const* s;
            std::memcpy(&s, p, sizeof(s));
            out += s ? ("'" + SqlEscape(s) + "'") : "NULL";
            break;
        }
        default:
            out += "NULL";
            break;
    }
}

// Returns 0=ok, 1=skipped(no file), 2=load/produce failure.
static int ProcessTable(std::string const& dataDir, std::string const& outDir)
{
    DB2FileLoadInfo const* loadInfo = nullptr;
    for (unsigned i = 0; i < g_tableCount; ++i)
        if (std::string(g_tables[i].File) == g_currentFile)
        {
            loadInfo = g_tables[i].LoadInfo;
            break;
        }
    if (!loadInfo)
        return 1;

    std::string fileName = g_currentFile;
    std::string fileFull = (fs::path(dataDir) / fileName).string();
    if (!fs::exists(fileFull))
        return 1;

    DB2FileLoader db2;
    DB2FileSystemSource source(fileFull);
    try
    {
        std::fprintf(stdout, "PHASE load %s\n", fileName.c_str());
        fflush(stdout);
        db2.Load(&source, loadInfo);
    }
    catch (std::exception const& e)
    {
        std::fprintf(stderr, "Failed to load %s: %s\n", fileName.c_str(), e.what());
        return 2;
    }

    DB2Meta const* meta = loadInfo->Meta;
    uint32 fieldCount = meta->FieldCount;              // collapsed logical fields
    uint32 fileFieldCount = db2.GetHeader().FieldCount; // physical columns in the file

    // Resolve a locale present in this file for string expansion.
    LocaleConstant locale = LOCALE_enUS;
    uint32 localeBits = db2.GetHeader().Locale;
    for (uint32 l = 0; l < TOTAL_LOCALES; ++l)
        if (localeBits & (1u << l))
        {
            locale = static_cast<LocaleConstant>(l);
            break;
        }

    uint32 indexTableSize = 0;
    char** indexTable = nullptr;
    try
    {
        std::fprintf(stdout, "PHASE produce %s\n", fileName.c_str());
        fflush(stdout);
        db2.AutoProduceData(indexTableSize, indexTable);
        std::fprintf(stdout, "PHASE strings %s rows=%u\n", fileName.c_str(), indexTableSize);
        fflush(stdout);
        db2.AutoProduceStrings(indexTable, indexTableSize, locale);
    }
    catch (std::exception const& e)
    {
        std::fprintf(stderr, "Failed to produce data for %s: %s\n", fileName.c_str(), e.what());
        return 2;
    }
    // By leewheel 2026-09-10: 空已知表(indexTableSize==0)不返回跳过，而是继续往下
    // 用 loadInfo 的准确列定义生成只含 CREATE TABLE 的空表 DDL，保证 zhCN 全部
    // 文件一个不少地落入 db2files（下面 311 的 INSERT 循环对 0 条记录自然跳过）。
    // End By leewheel
    if (indexTableSize == 0)
        std::fprintf(stdout, "EMPTY[known] %s\n", fileName.c_str());

    std::string base = fs::path(fileName).stem().string();
    std::string tableName = SanitizeName(base);

    std::string sql;
    sql.reserve(2 << 20);
    sql += "CREATE TABLE IF NOT EXISTS `" + tableName + "` (\n";

    bool idInData = meta->HasIndexFieldInData();
    if (!idInData)
        sql += "  `id` BIGINT NOT NULL,\n";

    // DDL is driven by the flattened loadInfo->Fields, which is the exact
    // field layout AutoProduceData emits into each record (ID first when
    // out-of-band, then one entry per meta field x ArraySize).  Driving by
    // meta->FieldCount alone is wrong: tables like SpellName/TactKey have a
    // single logical meta field but several flattened fields (and ID stored
    // out-of-band), which previously produced zero columns and crashed on
    // colNames[0] inside startBatch.
    std::vector<std::string> colNames;
    std::vector<DBCFormer> colTypes;
    uint32 startFlat = idInData ? 0 : 1; // when id is out-of-band, Fields[0]==ID is redundant
    for (uint32 fi = startFlat; fi < loadInfo->FieldCount; ++fi)
    {
        std::string name = SanitizeName(loadInfo->Fields[fi].Name);
        std::string full = name;
        int dup = 2;
        while (std::find(colNames.begin(), colNames.end(), "`" + full + "`") != colNames.end())
            full = name + "_" + std::to_string(dup++);
        colNames.push_back("`" + full + "`");
        colTypes.push_back(loadInfo->Fields[fi].Type);
        sql += "  `" + full + "` " + SqlType(loadInfo->Fields[fi].Type) + ",\n";
    }
    if (colNames.empty())
    {
        // Defensive: never emit a column-less table (would crash startBatch).
        colNames.push_back("`id`");
        colTypes.push_back(FT_INT);
        sql += "  `id` BIGINT NOT NULL,\n";
    }
    sql.resize(sql.size() - 2);
    sql += "\n) CHARACTER SET utf8mb4;\n\n";

    uint32 const batch = 500;
    uint32 emitted = 0;
    uint32 inBatch = 0;

    auto startBatch = [&]()
    {
        sql += "INSERT INTO `" + tableName + "` (" + colNames[0];
        for (size_t ci = 1; ci < colNames.size(); ++ci)
            sql += "," + colNames[ci];
        if (!idInData)
            sql += ",`id`";
        sql += ") VALUES\n";
        inBatch = 0;
    };

    for (uint32 id = 0; id < indexTableSize; ++id)
    {
        char* recordData = indexTable[id];
        if (!recordData)
            continue;

        if (inBatch == 0)
            startBatch();
        else
            sql += ",\n";

        char const* p = recordData;
        uint64 sqlId = 0;
        if (!idInData)
        {
            uint32 iv;
            std::memcpy(&iv, p, sizeof(iv));
            sqlId = iv;
            p += 4;
        }

        uint32 fieldIndex = idInData ? 0 : 1;
        sql += "(";
        bool first = true;
        // Data layout matches AutoProduceData's record exactly: when ID is
        // out-of-band one uint32 id lives at p[0] (consumed above), then one
        // value per flattened loadInfo->Fields entry in FieldSize() bytes —
        // all of which is physically present in the record.  No per-column
        // "present" check is needed because the flattened field list *is* the
        // record layout the loader produced.
        for (size_t col = 0; col < colTypes.size(); ++col)
        {
            if (!first)
                sql += ",";
            first = false;
            DBCFormer ftype = colTypes[col];
            AppendValue(sql, p, ftype, loadInfo->Fields[startFlat + col].IsSigned, locale);
            p += FieldSize(ftype);
        }
        if (!idInData)
            sql += "," + std::to_string(sqlId);
        sql += ")";
        ++emitted;

        if (++inBatch == batch)
        {
            sql += ";\n";
            inBatch = 0;
        }
    }
    if (inBatch != 0)
        sql += ";\n";

    std::string outPath = (fs::path(outDir) / (base + ".sql")).string();
    FILE* f = std::fopen(outPath.c_str(), "wb");
    if (!f)
    {
        std::fprintf(stderr, "Cannot write %s\n", outPath.c_str());
        return 2;
    }
    std::fwrite(sql.data(), 1, sql.size(), f);
    std::fclose(f);

    std::fprintf(stdout, "OK  %-42s rows=%u\n", fileName.c_str(), emitted);
    fflush(stdout);
    return 0;
}

// By leewheel 2026-09-10:
// 通用 WDC5 解析：对没有 TC loadInfo 的表，直接通过 DB2FileLoader 的
// 列元数据 + DB2Record 访问器逐字段读取，并以值域启发式识别字符串列，
// 从而将 zhCN 目录全部 815 个 db2 文件（含 265 张之外的表）导入 db2files。
// 列名采用 field0/field1...（文件本身不含字段名）。
static bool IsPrintableString(char const* s, std::size_t maxLen)
{
    if (!s)
        return false;
    std::size_t n = 0;
    while (n < maxLen)
    {
        unsigned char c = static_cast<unsigned char>(s[n]);
        if (c == 0)
            return n > 0 && strlen(s) <= 4096;
        if (c < 0x20 || c == 0x7F)
        {
            // 允许常见的控制字符（制表/换行），其余视为非文本
            if (c != '\t' && c != '\n' && c != '\r')
                return false;
        }
        ++n;
    }
    return false;
}

// By leewheel 2026-09-10:
// 稀疏(catalog)布局文件的原始字节导出。无 loadInfo 时稀疏文件不保存列元数据，
// 逐字段访问器会因依赖 _loadInfo->Meta 而崩溃。本函数用 ReadSparseRecord 逐条
// 读取每条记录的原始字节，以 {id, data(LONGBLOB)} 两列导入 db2files，记录数与
// 字节内容均原样保留，供后续按需反查。
// End By leewheel
static int ProcessGenericSparseTable(std::string const& dataDir, std::string const& outDir, DB2FileLoader& db2)
{
    (void)dataDir;
    std::string fileName = g_currentFile;
    uint32 recordCount = db2.GetSparseRecordCount();
    if (recordCount == 0)
    {
        // By leewheel 2026-09-10: 空稀疏文件也建空表，保证一个不少
        std::string b0 = fs::path(fileName).stem().string();
        std::string t0 = SanitizeName(b0);
        std::string e0 = "CREATE TABLE IF NOT EXISTS `" + t0 + "` (\n  `id` BIGINT NOT NULL,\n  `data` LONGBLOB\n) CHARACTER SET utf8mb4;\n\n";
        std::string op0 = (fs::path(outDir) / (b0 + ".sql")).string();
        FILE* f0 = std::fopen(op0.c_str(), "wb");
        if (f0)
        {
            std::fwrite(e0.data(), 1, e0.size(), f0);
            std::fclose(f0);
        }
        std::fprintf(stdout, "EMPTY[sparse] %-31s\n", fileName.c_str());
        fflush(stdout);
        return 0;
    }

    std::string base = fs::path(fileName).stem().string();
    std::string tableName = SanitizeName(base);

    std::string sql;
    sql.reserve(2 << 20);
    sql += "CREATE TABLE IF NOT EXISTS `" + tableName + "` (\n";
    sql += "  `id` BIGINT NOT NULL,\n";
    sql += "  `data` LONGBLOB\n";
    sql += ") CHARACTER SET utf8mb4;\n\n";

    uint32 const batch = 500;
    uint32 emitted = 0;
    uint32 inBatch = 0;
    auto startBatch = [&]()
    {
        sql += "INSERT INTO `" + tableName + "` (`id`,`data`) VALUES\n";
        inBatch = 0;
    };

    for (uint32 r = 0; r < recordCount; ++r)
    {
        uint32 id = 0;
        uint8 const* data = nullptr;
        uint32 size = 0;
        if (!db2.ReadSparseRecord(r, id, data, size))
            continue;

        if (inBatch == 0)
            startBatch();
        else
            sql += ",\n";

        std::string hex;
        hex.reserve(size * 2);
        static char const* hx = "0123456789ABCDEF";
        for (uint32 b = 0; b < size; ++b)
        {
            hex += hx[data[b] >> 4];
            hex += hx[data[b] & 0xF];
        }

        sql += "(" + std::to_string(id) + ",0x" + hex + ")";
        ++emitted;

        if (++inBatch == batch)
        {
            sql += ";\n";
            inBatch = 0;
        }
    }
    if (inBatch != 0)
        sql += ";\n";

    std::string outPath = (fs::path(outDir) / (base + ".sql")).string();
    FILE* fout = std::fopen(outPath.c_str(), "wb");
    if (!fout)
    {
        std::fprintf(stderr, "Cannot write %s\n", outPath.c_str());
        return 2;
    }
    std::fwrite(sql.data(), 1, sql.size(), fout);
    std::fclose(fout);

    std::fprintf(stdout, "GEN[sparse] %-35s rows=%u\n", fileName.c_str(), emitted);
    fflush(stdout);
    return 0;
}
// End By leewheel

static int ProcessGenericTable(std::string const& dataDir, std::string const& outDir)
{
    std::string fileName = g_currentFile;
    std::string fileFull = (fs::path(dataDir) / fileName).string();
    if (!fs::exists(fileFull))
        return 1;

    DB2FileLoader db2;
    // By leewheel 2026-09-10 修复：source 必须存活到稀疏导出完成。若把 source 声明
    // 在 try 块内，Load 返回即析构，而 DB2FileLoaderSparseImpl 长期持有其指针
    // _source(悬垂)，导致稀疏 ReadSparseRecord 调用虚函数时堆损坏崩溃。故把
    // source 提升到函数体作用域，仅包 try 于 Load 部分。
    // End By leewheel
    DB2FileSystemSource source(fileFull);
    try
    {
        db2.Load(&source, nullptr); // 无 loadInfo 仍可载入数据（columnMeta 来自文件）
    }
    catch (std::exception const& e)
    {
        std::fprintf(stderr, "Failed to generic-load %s: %s\n", fileName.c_str(), e.what());
        return 2;
    }

    // By leewheel 2026-09-10:
    // 稀疏(catalog)布局不保存 columnMeta，无 loadInfo 时无法逐字段读取，且
    // 逐字段访问器(GetString/GetFieldOffset 依赖 _loadInfo->Meta)在此场景会
    // 崩溃。对稀疏文件改用 ReadSparseRecord 导出每条记录原始字节，保证该文件
    // 仍能一个不少地导入 db2files，且不崩溃。
    // End By leewheel
    bool isReg = db2.IsRegular();
    if (!isReg)
    {
        int rv = ProcessGenericSparseTable(dataDir, outDir, db2);
        return rv;
    }

    DB2Header const& header = db2.GetHeader();
    uint32 fieldCount = header.TotalFieldCount;
    if (fieldCount == 0)
        fieldCount = header.FieldCount;
    uint32 recordCount = header.RecordCount;
    uint32 stringTableSize = header.StringTableSize;
    // By leewheel 2026-09-10:
    // 空文件(RecordCount==0)同样必须"一个不少"地建表。无数据时无法采样推断
    // 列类型，但 header 含 TotalFieldCount/FieldCount，据此建 id + field0..N 空表
    // 结构，保证该 db2 文件在 db2files 中也有对应表(空表)。列统一 BIGINT(空表无
    // 数据可判类型)，后续若有数据归属可再精确化。
    // End By leewheel
    if (recordCount == 0)
    {
        std::string baseEmpty = fs::path(fileName).stem().string();
        std::string tableNameEmpty = SanitizeName(baseEmpty);
        uint32 emptyFieldCount = (header.TotalFieldCount != 0) ? header.TotalFieldCount : header.FieldCount;
        std::string sqlEmpty = "CREATE TABLE IF NOT EXISTS `" + tableNameEmpty + "` (\n  `id` BIGINT NOT NULL";
        for (uint32 f = 0; f < emptyFieldCount; ++f)
            sqlEmpty += ",\n  `field" + std::to_string(f) + "` BIGINT";
        sqlEmpty += "\n) CHARACTER SET utf8mb4;\n\n";
        std::string outPathEmpty = (fs::path(outDir) / (baseEmpty + ".sql")).string();
        FILE* fEmpty = std::fopen(outPathEmpty.c_str(), "wb");
        if (fEmpty)
        {
            std::fwrite(sqlEmpty.data(), 1, sqlEmpty.size(), fEmpty);
            std::fclose(fEmpty);
        }
        std::fprintf(stdout, "EMPTY %-38s\n", fileName.c_str());
        fflush(stdout);
        return 0;
    }

    // 采样判定每列类型（字符串 / 浮点 / 整数）。
    // WDC5 字符串字段实际存储为字符串表偏移（uint32），因此若某列所有非零
    // 采样值都 < stringTableSize 且其偏移处是合法可打印文本，则判为字符串列。
    // By leewheel 2026-09-10 修正：
    // 通用解析不能再对所有数值列统一读 8 字节——无 loadInfo 时各列位宽不同，
    // 用 GetUInt64 会把窄列(如 2/4 字节)和相邻列一起读出，产生跨列垃圾值。
    // 这里按列元数据的位宽(BitSize)选择正确的访问器(1/2/4/8 字节)。
    // 字符串列专列处理（GetString 内部已按 uint32 偏移解析）。
    // End By leewheel
    uint32 const maxSample = std::min<uint32>(recordCount, 128);

    // 每列元数据（缓存，避免循环内反复查询）
    std::vector<uint8> colBytes(fieldCount, 8);      // None 压缩字节宽（1/2/4/8）
    std::vector<uint8> colComp(fieldCount, 0);       // 压缩类型
    for (uint32 f = 0; f < fieldCount; ++f)
    {
        uint16 bitOff = 0, bitSize = 0;
        uint32 addData = 0, comp = 0;
        if (fieldCount <= header.TotalFieldCount)
            db2.GetColumnMeta(f, bitOff, bitSize, addData, comp);
        colComp[f] = uint8(comp);
        if (comp == 0) // None：BitSize 即字段位宽
        {
            uint32 bw = uint32(bitSize + 7) / 8;
            if (bw <= 1)      colBytes[f] = 1;
            else if (bw <= 2) colBytes[f] = 2;
            else if (bw <= 4) colBytes[f] = 4;
            else              colBytes[f] = 8;
        }
    }

    // 按位宽读取列原始值（None 用窄读，压缩列值由 GetUInt64 正确解包）
    auto readFieldU64 = [&](DB2Record const& rec, uint32 f) -> uint64
    {
        try
        {
            if (colComp[f] == 0)
            {
                switch (colBytes[f])
                {
                    case 1:  return rec.GetUInt8(f, 0);
                    case 2:  return rec.GetUInt16(f, 0);
                    case 4:  return rec.GetUInt32(f, 0);
                    default: return rec.GetUInt64(f, 0);
                }
            }
            return rec.GetUInt64(f, 0);
        }
        catch (...) { return 0; }
    };

    std::vector<bool> isString(fieldCount, false);
    std::vector<uint64> colMin(fieldCount, uint64(-1));
    std::vector<uint64> colMax(fieldCount, 0);

    for (uint32 r = 0; r < maxSample; ++r)
    {
        DB2Record rec = db2.GetRecord(r);
        if (!rec)
            continue;
        for (uint32 f = 0; f < fieldCount; ++f)
        {
            uint64 v = readFieldU64(rec, f);
            if (colMin[f] > v) colMin[f] = v;
            if (colMax[f] < v) colMax[f] = v;
        }
    }

    for (uint32 f = 0; f < fieldCount; ++f)
    {
        // 浮点与整数的区别在无负荷布局时无法可靠从文件推断；统一按 BIGINT 存
        // 原始位模式值（对 8/16/32/64 位整数正确，对浮点显示为字节模式整数，
        // 便于后续查询与反查）。仅当采样值全部落在字符串表范围内才判为字符串列。
        if (stringTableSize == 0)
            continue; // 无字符串表则必无字符串列

        // 压缩类型即时/公共/调色板通常是整数索引；None 且值域收窄到字符串表，
        // 且进一步用 GetString 在采样上验证过滤（越界读写由 SEH 兜底）
        if (colComp[f] != 0)
            continue;

        if (colMin[f] >= stringTableSize || colMax[f] >= stringTableSize)
            continue;

        // 必须是"可直接文本化"的列：采样首条记录确认
        {
            DB2Record rec = db2.GetRecord(0);
            if (!rec)
                continue;
            char const* s = nullptr;
            try { s = rec.GetString(f, 0); } catch (...) { s = nullptr; }
            if (s && IsPrintableString(s, 256))
                isString[f] = true;
        }
    }

    // 构建 DDL：字符串列用 TEXT，判定为浮点的用 DOUBLE，其余统一用 BIGINT 存原始值
    std::string base = fs::path(fileName).stem().string();
    std::string tableName = SanitizeName(base);
    std::vector<std::string> colNames;
    std::vector<uint8> colKind; // 0=整数 1=字符串
    std::string sql;
    sql.reserve(2 << 20);
    sql += "CREATE TABLE IF NOT EXISTS `" + tableName + "` (\n";
    sql += "  `id` BIGINT NOT NULL,\n";
    for (uint32 f = 0; f < fieldCount; ++f)
    {
        std::string name = "field" + std::to_string(f);
        while (std::find(colNames.begin(), colNames.end(), name) != colNames.end())
            name += "_";
        colNames.push_back(name);
        uint8 kind = isString[f] ? 1 : 0;
        colKind.push_back(kind);
        // By leewheel 2026-09-10: 通用表存 uint64 原始位模式，BIGINT(有符号)对值超过
        // 2^63 的记录会 Out of range，统一改用 BIGINT UNSIGNED 覆盖 0..2^64-1。
        // End By leewheel
        sql += "  `" + name + "` " + (kind == 1 ? std::string("TEXT") : std::string("BIGINT UNSIGNED")) + ",\n";
    }
    sql.resize(sql.size() - 2);
    sql += "\n) CHARACTER SET utf8mb4;\n\n";

    uint32 const batch = 500;
    uint32 emitted = 0;
    uint32 inBatch = 0;
    auto startBatch = [&]()
    {
        sql += "INSERT INTO `" + tableName + "` (`id`";
        for (size_t ci = 0; ci < colNames.size(); ++ci)
            sql += ",`" + colNames[ci] + "`";
        sql += ") VALUES\n";
        inBatch = 0;
    };

    for (uint32 r = 0; r < recordCount; ++r)
    {
        DB2Record rec = db2.GetRecord(r);
        if (!rec)
            continue;

        uint32 id = 0;
        try { id = rec.GetUInt32(0, 0); } catch (...) { id = r; }

        if (inBatch == 0)
            startBatch();
        else
            sql += ",\n";

        sql += "(" + std::to_string(id);
        for (uint32 f = 0; f < fieldCount; ++f)
        {
            switch (colKind[f])
            {
                case 1: // 字符串
                {
                    std::string sv;
                    try { char const* s = rec.GetString(f, 0); if (s) sv = s; } catch (...) { }
                    sql += ",'" + SqlEscape(sv) + "'";
                    break;
                }
                case 2: // 浮点
                {
                    float fv = 0.f;
                    try { fv = rec.GetFloat(f, 0); } catch (...) { fv = 0.f; }
                    if (std::isnan(fv) || std::isinf(fv))
                        sql += ",NULL";
                    else
                    {
                        char buf[64];
                        std::snprintf(buf, sizeof(buf), "%.9g", double(fv));
                        sql += ",";
                        sql += buf;
                    }
                    break;
                }
                default: // 整数（按列位宽窄读，避免跨列垃圾值）
                {
                    uint64 iv = readFieldU64(rec, f);
                    sql += "," + std::to_string(iv);
                    break;
                }
            }
        }
        sql += ")";
        ++emitted;

        if (++inBatch == batch)
        {
            sql += ";\n";
            inBatch = 0;
        }
    }
    if (inBatch != 0)
        sql += ";\n";

    std::string outPath = (fs::path(outDir) / (base + ".sql")).string();
    FILE* fout = std::fopen(outPath.c_str(), "wb");
    if (!fout)
    {
        std::fprintf(stderr, "Cannot write %s\n", outPath.c_str());
        return 2;
    }
    std::fwrite(sql.data(), 1, sql.size(), fout);
    std::fclose(fout);

    std::fprintf(stdout, "GEN %-42s rows=%u cols=%u\n", fileName.c_str(), emitted, fieldCount);
    fflush(stdout);
    return 0;
}

// Pure-C-string match on the basename-without-extension, so the SEH `__try`
// function below needs no C++ temporaries (which would trigger C2712).
static bool TableMatchesFilter(char const* file, char const* filter)
{
    if (!filter || !*filter)
        return true;
    char const* start = file;
    char const* bs = strrchr(file, '\\');
    char const* fs_ = strrchr(file, '/');
    char const* slash = nullptr;
    if (bs && fs_)
        slash = (bs > fs_) ? bs : fs_;
    else
        slash = bs ? bs : fs_;
    if (slash)
        start = slash + 1;
    char const* dot = strrchr(start, '.');
    char const* end = dot ? dot : start + strlen(start);
    return std::size_t(end - start) == strlen(filter) && strncmp(start, filter, end - start) == 0;
}

static int DumpAllToSql(char const* dataDir, char const* outDir, char const* filter)
{
    unsigned ok = 0, skipped = 0, failed = 0, crashed = 0;
    unsigned okGen = 0, failedGen = 0, crashedGen = 0;

    // 记录已通过 TC loadInfo 处理过的 basename，避免通用解析重复覆盖
    std::vector<std::string> knownFiles;
    knownFiles.reserve(g_tableCount);
    for (unsigned t = 0; t < g_tableCount; ++t)
        knownFiles.emplace_back(g_tables[t].File);

    // 第一阶段：TC loadInfo 认识的表（原逻辑）
    for (unsigned t = 0; t < g_tableCount; ++t)
    {
        g_currentFile = g_tables[t].File;
        if (!TableMatchesFilter(g_currentFile, filter))
            continue;

        int rc = 0;
        try
        {
            rc = ProcessTable(dataDir, outDir);
        }
        catch (...)
        {
            std::fprintf(stderr, "CRASH skipped: %s\n", g_currentFile);
            fflush(stderr);
            ++crashed;
            continue;
        }

        if (rc == 0)
            ++ok;
        else if (rc == 1)
            ++skipped;
        else
            ++failed;
    }

    // By leewheel 2026-09-10:
    // 第二阶段：通用 WDC5 解析，覆盖目录中所有 loadInfo 未认识、但确实存在的
    // db2 文件（zhCN 共 815 个，其中约 550 张 TC 不认识），保证一个不遗漏。
    {
        std::vector<std::string> allFiles;
        for (auto const& entry : fs::directory_iterator(dataDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".db2")
                allFiles.emplace_back(entry.path().filename().string());
        }
        std::sort(allFiles.begin(), allFiles.end());

        auto isKnown = [&](std::string const& f) {
            return std::find(knownFiles.begin(), knownFiles.end(), f) != knownFiles.end();
        };

        for (std::string const& f : allFiles)
        {
            if (isKnown(f))
                continue;
            if (!TableMatchesFilter(f.c_str(), filter))
                continue;

            g_currentFile = f.c_str();
            int rc = 0;
            try
            {
                rc = ProcessGenericTable(dataDir, outDir);
            }
            catch (...)
            {
                std::fprintf(stderr, "CRASH skipped(gen): %s\n", g_currentFile);
                fflush(stderr);
                ++crashedGen;
                continue;
            }

            if (rc == 0)
                ++okGen;
            else if (rc == 1)
                ++skipped;
            else
                ++failedGen;
        }
    }
    // End By leewheel

    std::fprintf(stdout, "done: ok=%u skipped=%u failed=%u crashed=%u | gen: ok=%u failed=%u crashed=%u\n",
        ok, skipped, failed, crashed, okGen, failedGen, crashedGen);
    return 0;
}

int main(int argc, char** argv)
{
#ifdef _WIN32
    // Convert structured exceptions (e.g. access violations in the DB2 parser)
    // into catchable C++ exceptions so the batch survives a single bad file.
    _set_se_translator([](unsigned, EXCEPTION_POINTERS*) { throw std::runtime_error("SEH exception"); });
#endif

    if (argc < 3)
    {
        std::fprintf(stderr, "Usage: db2dump <dataDir> <outDir> [onlyTableBasename]\n");
        return 2;
    }

    char const* dataDir = argv[1];
    char const* outDir  = argv[2];
    char const* filter  = argc >= 4 ? argv[3] : nullptr;

    fs::create_directories(outDir);
    return DumpAllToSql(dataDir, outDir, filter);
}