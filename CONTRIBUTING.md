# Contributing to RaisaBrowser

<details open>
<summary><strong>🇬🇧 English</strong></summary>

<br>

Thank you for your interest in contributing to **RaisaBrowser** — a lightweight, privacy-first browser for Linux built with Qt WebEngine and C++. Contributions of all kinds are welcome: bug fixes, features, documentation improvements, or just filing a good issue.

---

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [How to Contribute](#how-to-contribute)
- [Reporting Bugs](#reporting-bugs)
- [Suggesting Features](#suggesting-features)
- [Pull Request Guidelines](#pull-request-guidelines)
- [Code Style](#code-style)
- [License](#license)

---

## Code of Conduct

Be respectful, constructive, and patient. This is an open-source project maintained in personal time. Harassment of any form will not be tolerated.

---

## Getting Started

### Prerequisites

- Linux (any distro — Arch is the primary development environment)
- Qt 6 or Qt 5 with `Qt WebEngine` module
- `g++` / `clang++` with C++17 support
- `qmake` or `cmake`

### Build locally

```bash
git clone https://github.com/ramdanolii14/RaisaBrowser.git
cd RaisaBrowser
qmake
make -j$(nproc)
./RaisaBrowser
```

---

## How to Contribute

1. **Fork** the repository.
2. **Create a branch** off `main` with a descriptive name:
   ```
   git checkout -b fix/crash-on-close
   git checkout -b feat/bookmark-support
   ```
3. **Make your changes**, keeping commits focused and atomic.
4. **Test** your changes on Linux before submitting.
5. **Open a Pull Request** against the `main` branch with a clear description of what you changed and why.

---

## Reporting Bugs

Before opening an issue, please check if it is already listed in the [Issues](https://github.com/ramdanolii14/RaisaBrowser/issues) tab.

When filing a bug report, include:

- Your Linux distro and version
- Qt version (`qmake --version`)
- Steps to reproduce the bug
- Expected vs actual behavior
- Any relevant terminal output or crash logs

> Known bugs (e.g., Google login not working, crash on close) are already tracked — please avoid duplicate reports for those.

---

## Suggesting Features

Open an issue with the label `enhancement` and describe:

- What the feature does
- Why it fits RaisaBrowser's goals (privacy + minimal RAM usage)
- Any implementation ideas you have (optional)

Keep in mind: RaisaBrowser is intentionally minimal. Features that would significantly increase RAM usage or add unnecessary complexity may be declined.

---

## Pull Request Guidelines

- Keep PRs focused — one fix or feature per PR.
- Write a clear PR title, e.g.: `fix: prevent crash when closing window` or `feat: add basic bookmark bar`.
- If your PR fixes an open issue, reference it: `Closes #12`.
- Make sure the code compiles without warnings on a standard Linux + Qt setup.
- Do not include unrelated formatting changes in the same PR.

---

## Code Style

- Follow the existing Qt/C++ coding conventions already used in `mainwindow.cpp` and `sessionmanager.cpp`.
- Use 4-space indentation.
- Class names in `PascalCase`, variables and functions in `camelCase`.
- Keep comments concise and in English.

---

## License

By contributing, you agree that your contributions will be licensed under the [GPL-3.0 License](./LICENSE) — the same license that covers RaisaBrowser.

</details>

---

<details>
<summary><strong>🇮🇩 Bahasa Indonesia</strong></summary>

<br>

Terima kasih sudah tertarik berkontribusi ke **RaisaBrowser** — browser Linux yang ringan dan mengutamakan privasi, dibangun dengan Qt WebEngine dan C++. Semua bentuk kontribusi disambut: perbaikan bug, fitur baru, peningkatan dokumentasi, atau sekadar melaporkan issue yang berguna.

---

## Daftar Isi

- [Kode Etik](#kode-etik)
- [Persiapan Awal](#persiapan-awal)
- [Cara Berkontribusi](#cara-berkontribusi)
- [Melaporkan Bug](#melaporkan-bug)
- [Mengusulkan Fitur](#mengusulkan-fitur)
- [Panduan Pull Request](#panduan-pull-request)
- [Gaya Kode](#gaya-kode)
- [Lisensi](#lisensi)

---

## Kode Etik

Bersikaplah saling menghormati, konstruktif, dan sabar. Ini adalah proyek open-source yang dikelola di waktu luang. Segala bentuk pelecehan atau tindakan tidak menyenangkan tidak akan ditoleransi.

---

## Persiapan Awal

### Prasyarat

- Linux (distro apa saja — Arch adalah lingkungan pengembangan utama)
- Qt 6 atau Qt 5 dengan modul `Qt WebEngine`
- `g++` / `clang++` dengan dukungan C++17
- `qmake` atau `cmake`

### Build di lokal

```bash
git clone https://github.com/ramdanolii14/RaisaBrowser.git
cd RaisaBrowser
qmake
make -j$(nproc)
./RaisaBrowser
```

---

## Cara Berkontribusi

1. **Fork** repositori ini.
2. **Buat branch** dari `main` dengan nama yang deskriptif:
   ```
   git checkout -b fix/crash-saat-tutup
   git checkout -b feat/dukungan-bookmark
   ```
3. **Lakukan perubahan**, usahakan setiap commit fokus dan tidak bertele-tele.
4. **Uji perubahan** kamu di Linux sebelum mengirim PR.
5. **Buka Pull Request** ke branch `main` dengan deskripsi yang jelas tentang apa yang diubah dan alasannya.

---

## Melaporkan Bug

Sebelum membuka issue, periksa dulu apakah bug tersebut sudah ada di tab [Issues](https://github.com/ramdanolii14/RaisaBrowser/issues).

Saat melaporkan bug, sertakan:

- Distro Linux dan versinya
- Versi Qt (`qmake --version`)
- Langkah-langkah untuk mereproduksi bug
- Perilaku yang diharapkan vs yang terjadi
- Output terminal atau log crash yang relevan

> Bug yang sudah diketahui (misalnya Google login tidak bisa, crash saat tutup) sudah tercatat — mohon hindari duplikasi laporan untuk hal tersebut.

---

## Mengusulkan Fitur

Buka issue dengan label `enhancement` dan jelaskan:

- Apa yang dilakukan fitur tersebut
- Mengapa fitur itu sesuai dengan tujuan RaisaBrowser (privasi + penggunaan RAM minimal)
- Ide implementasi jika ada (opsional)

Perlu diingat: RaisaBrowser sengaja dibuat minimal. Fitur yang akan meningkatkan penggunaan RAM secara signifikan atau menambah kompleksitas yang tidak perlu mungkin akan ditolak.

---

## Panduan Pull Request

- Fokuskan PR — satu perbaikan atau satu fitur per PR.
- Tulis judul PR yang jelas, misal: `fix: mencegah crash saat menutup jendela` atau `feat: tambah bilah bookmark dasar`.
- Jika PR kamu memperbaiki issue yang ada, referensikan: `Closes #12`.
- Pastikan kode bisa dikompilasi tanpa warning di setup Linux + Qt standar.
- Jangan sertakan perubahan formatting yang tidak berkaitan dalam PR yang sama.

---

## Gaya Kode

- Ikuti konvensi Qt/C++ yang sudah digunakan di `mainwindow.cpp` dan `sessionmanager.cpp`.
- Gunakan indentasi 4 spasi.
- Nama kelas menggunakan `PascalCase`, variabel dan fungsi menggunakan `camelCase`.
- Komentar kode boleh dalam Bahasa Indonesia atau Inggris.

---

## Lisensi

Dengan berkontribusi, kamu menyetujui bahwa kontribusimu akan dilisensikan di bawah [Lisensi GPL-3.0](./LICENSE) — lisensi yang sama yang mencakup RaisaBrowser.

</details>
