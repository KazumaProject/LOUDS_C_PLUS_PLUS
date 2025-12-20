# LOUDS Trie (C++) — Writer / Reader split + Succinct Rank/Select

---

# 日本語

## 概要

このリポジトリは、Trie（Prefix Tree）を **LOUDS（Level-Order Unary Degree Sequence）** 形式に変換し、バイナリに保存してロードできるようにする C++ 実装です。

- **Writer 側**（`LOUDS`, `LOUDSWithTermId`）
  - PrefixTree を LOUDS に変換
z
  - LOUDS をバイナリファイルとして保存（シリアライズ）
- **Reader 側**（`LOUDSReader`, `LOUDSWithTermIdReader`）
  - Writer が出力した LOUDS バイナリをロード
  - `SuccinctBitVector`（rank/select の前計算）で探索を高速化

> `termId` 版について:  
> `commonPrefixSearch` は **termId を同時に返しません**。必要な場合は `getTermId(nodeIndex)` を別途呼び出します。

---

## ディレクトリ構成

```text
project/
  src/
    common/
      bit_vector.hpp
      succinct_bit_vector.hpp

    prefix/
      prefix_tree.hpp
      prefix_tree.cpp

    louds/
      louds.hpp
      louds.cpp
      converter.hpp
      converter.cpp
      louds_reader.hpp
      louds_reader.cpp

    prefix_with_term_id/
      prefix_tree_with_term_id.hpp
      prefix_tree_with_term_id.cpp

    louds_with_term_id/
      louds_with_term_id.hpp
      louds_with_term_id.cpp
      converter_with_term_id.hpp
      converter_with_term_id.cpp
      louds_with_term_id_reader.hpp
      louds_with_term_id_reader.cpp

  tests/
    test_prefix_tree.cpp
    test_louds.cpp
    test_louds_with_term_id.cpp

    test_louds_reader.cpp
    test_louds_with_term_id_reader.cpp
```

---

## 依存関係

- Linux / WSL / macOS を想定
- C++17 対応の `g++`

Ubuntu / WSL の例:

```bash
sudo apt update
sudo apt install -y g++
```

---

## 仕組み

### LOUDS とは

LOUDS は Trie の木構造を BFS（幅優先）順にビット列へエンコードします。

- 各ノードの子の数だけ `1` を並べ、最後に `0` を 1 つ置く
- 追加情報として:
  - `labels[]`: `1` に対応するエッジのラベル（文字）
  - `isLeaf`: 単語終端ノードかどうかを示すビット列

### rank / select

探索のために以下を使います:

- `rank1(i)`: `LBS[0..i]` の 1 の数
- `rank0(i)`: `LBS[0..i]` の 0 の数
- `select1(k)`: k 番目の 1 の位置
- `select0(k)`: k 番目の 0 の位置

`SuccinctBitVector` は Kotlin 実装と同様に、ブロック（256bit / 8bit）ごとの累積を前計算して高速化します。

---

## ベンチマーク / 入力データ情報

本リポジトリでは、jawiki の「all titles (ns0)」gzip ファイルを入力として LOUDS を構築する想定です。  
以下は `jawiki_build` 実行時に計測した統計および時間です。

### 入力データ統計

- 総単語数（タイトル数）: **2,412,128**
- 総文字数: **20,334,605**
- 入力 gzip サイズ: **15,417,082 bytes**（約 **15.42 MB** / **14.70 MiB**）
- UTF-8 展開後合計（バイト総量）: **53,364,837 bytes**（約 **53.36 MB** / **50.89 MiB**）

### 変換時間（内部計測）

- total: **6.03564 sec**
- convert LOUDS: **0.944134 sec**
- convert LOUDS with termId: **0.956271 sec**

### 実行時間（`/usr/bin/time -v`）

- Elapsed (wall clock): **0:11.02**
- User time: **9.20 sec**, System time: **1.80 sec**
- Max RSS: **4,200,128 KB**（約 **4.01 GiB**）

> 補足: `metrics.json` の秒数は「処理の一部（内部区間）」の計測で、`/usr/bin/time` はプロセス全体（I/O や初期化等を含む）を測定します。そのため、両者の値が一致しないのは自然です。

---

## テスト実行

以下は `project/` ディレクトリ直下で実行する想定です。  
（`-I src` により `src/...` を include できるようにしています）

### 1) PrefixTree テスト

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  src/prefix/prefix_tree.cpp \
  tests/test_prefix_tree.cpp \
  -I src -o test_prefix_tree

./test_prefix_tree
```

### 2) LOUDS（Writer）テスト

`commonPrefixSearch` と、Writer 側のバイナリ round-trip（`saveToFile -> loadFromFile`）を検証します。

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  src/louds/louds.cpp \
  src/louds/converter.cpp \
  src/prefix/prefix_tree.cpp \
  tests/test_louds.cpp \
  -I src -o test_louds

./test_louds
```

### 3) LOUDSWithTermId（Writer）テスト

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  src/louds_with_term_id/louds_with_term_id.cpp \
  src/louds_with_term_id/converter_with_term_id.cpp \
  src/prefix_with_term_id/prefix_tree_with_term_id.cpp \
  tests/test_louds_with_term_id.cpp \
  -I src -o test_louds_with_term_id

./test_louds_with_term_id
```

### 4) LOUDSReader テスト（Writer 出力を Reader が読む）

Writer が作った `.bin` を Reader がロードできること、`commonPrefixSearch` 等が動くことを検証します。

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  src/louds/louds_reader.cpp \
  src/louds/louds.cpp \
  src/louds/converter.cpp \
  src/prefix/prefix_tree.cpp \
  tests/test_louds_reader.cpp \
  -I src -o test_louds_reader

./test_louds_reader
```

### 5) LOUDSWithTermIdReader テスト（Writer 出力を Reader が読む）

Writer が作った `.bin` を Reader がロードできること、`getTermId(nodeIndex)` が動くことも含めて検証します。

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  src/louds_with_term_id/louds_with_term_id_reader.cpp \
  src/louds_with_term_id/louds_with_term_id.cpp \
  src/louds_with_term_id/converter_with_term_id.cpp \
  src/prefix_with_term_id/prefix_tree_with_term_id.cpp \
  tests/test_louds_with_term_id_reader.cpp \
  -I src -o test_louds_with_term_id_reader

./test_louds_with_term_id_reader
```

---

## ライセンス

本プロジェクトは **MIT License** です。`LICENSE` ファイルを参照してください。

---

# English

## Summary

This repository provides a C++ implementation to convert a trie (prefix tree) into **LOUDS (Level-Order Unary Degree Sequence)**, serialize it to a binary file, and load it back for fast query operations.

- **Writer side** (`LOUDS`, `LOUDSWithTermId`)
  - Convert PrefixTree into LOUDS
  - Save LOUDS as a binary file (serialization)
- **Reader side** (`LOUDSReader`, `LOUDSWithTermIdReader`)
  - Load LOUDS binaries generated by the writer
  - Use `SuccinctBitVector` (precomputed rank/select) to accelerate traversal

> About the `termId` variant:  
> `commonPrefixSearch` does **not** return termIds together. If needed, call `getTermId(nodeIndex)` separately.

---

## Directory Structure

```text
project/
  src/
    common/
      bit_vector.hpp
      succinct_bit_vector.hpp

    prefix/
      prefix_tree.hpp
      prefix_tree.cpp

    louds/
      louds.hpp
      louds.cpp
      converter.hpp
      converter.cpp
      louds_reader.hpp
      louds_reader.cpp

    prefix_with_term_id/
      prefix_tree_with_term_id.hpp
      prefix_tree_with_term_id.cpp

    louds_with_term_id/
      louds_with_term_id.hpp
      louds_with_term_id.cpp
      converter_with_term_id.hpp
      converter_with_term_id.cpp
      louds_with_term_id_reader.hpp
      louds_with_term_id_reader.cpp

  tests/
    test_prefix_tree.cpp
    test_louds.cpp
    test_louds_with_term_id.cpp

    test_louds_reader.cpp
    test_louds_with_term_id_reader.cpp
```

---

## Requirements

- Linux / WSL / macOS recommended
- `g++` with C++17 support

Example on Ubuntu / WSL:

```bash
sudo apt update
sudo apt install -y g++
```

---

## How It Works

### What is LOUDS?

LOUDS encodes a trie shape into a bit sequence in BFS order:

- For each node, write `1` for each child, followed by one terminating `0`.
- Additional arrays:
  - `labels[]`: edge labels aligned with `1` bits
  - `isLeaf`: marks terminal nodes

### rank / select

Queries rely on:

- `rank1(i)`: number of `1`s in `LBS[0..i]`
- `rank0(i)`: number of `0`s in `LBS[0..i]`
- `select1(k)`: position of k-th `1`
- `select0(k)`: position of k-th `0`

`SuccinctBitVector` precomputes block summaries (256-bit big blocks, 8-bit small blocks) to accelerate rank/select similar to the Kotlin implementation.

---

## Benchmark / Input Data Notes

This repo commonly uses a jawiki “all titles (ns0)” gzip file as input.  
Below are dataset stats and timing results recorded during `jawiki_build`.

### Dataset stats

- Total titles (word_count): **2,412,128**
- Total characters (char_count): **20,334,605**
- Input gzip size: **15,417,082 bytes** (≈ **15.42 MB** / **14.70 MiB**)
- Total UTF-8 bytes after decoding: **53,364,837 bytes** (≈ **53.36 MB** / **50.89 MiB**)

### Conversion time (internal measurements)

- total: **6.03564 sec**
- convert LOUDS: **0.944134 sec**
- convert LOUDS with termId: **0.956271 sec**

### End-to-end time (`/usr/bin/time -v`)

- Elapsed (wall clock): **0:11.02**
- User time: **9.20 sec**, System time: **1.80 sec**
- Max RSS: **4,200,128 KB** (≈ **4.01 GiB**)

> Note: `metrics.json` reflects internal section timings, while `/usr/bin/time` measures the full process (including I/O and initialization). Therefore it is expected that these numbers do not match exactly.

---

## Testing Commands

All commands assume you are in the `project/` directory.  
`-I src` is used so headers under `src/...` can be included.

### 1) PrefixTree test

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  src/prefix/prefix_tree.cpp \
  tests/test_prefix_tree.cpp \
  -I src -o test_prefix_tree

./test_prefix_tree
```

### 2) LOUDS (Writer) test

This validates `commonPrefixSearch` and writer-side round-trip (`saveToFile -> loadFromFile`).

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  src/louds/louds.cpp \
  src/louds/converter.cpp \
  src/prefix/prefix_tree.cpp \
  tests/test_louds.cpp \
  -I src -o test_louds

./test_louds
```

### 3) LOUDSWithTermId (Writer) test

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  src/louds_with_term_id/louds_with_term_id.cpp \
  src/louds_with_term_id/converter_with_term_id.cpp \
  src/prefix_with_term_id/prefix_tree_with_term_id.cpp \
  tests/test_louds_with_term_id.cpp \
  -I src -o test_louds_with_term_id

./test_louds_with_term_id
```

### 4) LOUDSReader test (Reader loads writer output)

This validates that `LOUDSReader` can load the `.bin` produced by the writer and run queries like `commonPrefixSearch`.

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  src/louds/louds_reader.cpp \
  src/louds/louds.cpp \
  src/louds/converter.cpp \
  src/prefix/prefix_tree.cpp \
  tests/test_louds_reader.cpp \
  -I src -o test_louds_reader

./test_louds_reader
```

### 5) LOUDSWithTermIdReader test (Reader loads writer output)

This validates that `LOUDSWithTermIdReader` can load the `.bin` and that `getTermId(nodeIndex)` works.

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  src/louds_with_term_id/louds_with_term_id_reader.cpp \
  src/louds_with_term_id/louds_with_term_id.cpp \
  src/louds_with_term_id/converter_with_term_id.cpp \
  src/prefix_with_term_id/prefix_tree_with_term_id.cpp \
  tests/test_louds_with_term_id_reader.cpp \
  -I src -o test_louds_with_term_id_reader

./test_louds_with_term_id_reader
```

---

## License

This project is licensed under the **MIT License**. See `LICENSE` for details.
