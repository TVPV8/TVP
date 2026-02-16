# TVP – The Forgotten Server Fork

A custom fork of [The Forgotten Server](https://github.com/otland/forgottenserver).

This repository contains changes done by the community to improve distributed TVP project.

---

## 🚀 Quick Start (Using Build Artifacts)

This repository uses GitHub Actions to automatically build the server.

Every push produces **ready-to-run artifacts**, so you don’t need to compile manually.

### Run using artifacts:
1. Go to the **Actions** tab in this repository.
2. Open the latest successful workflow.
3. Download the generated artifact.
4. Extract it on your server.
5. Import `schema.sql` to your mysql database.
6. Configure `config.lua`.
7. Run the server binary.

---

## 🛠 Manual Build (Optional)

If you prefer building locally:

### Requirements

- CMake  
- C++17 compatible compiler  
- MySQL or MariaDB  
- Boost  
- LuaJIT  

### Build

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

The compiled binary will be available inside the `build` directory.

---

## ⚙️ Configuration

1. Copy:

```bash
cp config.lua.dist config.lua
```

2. Edit database credentials and server settings.
3. Import the database schema.
4. Start the server.

---

## 🔁 CI/CD

GitHub Actions automatically:

- Build the server  
- Package the executable  
- Generate downloadable artifacts  

---

## 📁 Project Structure

```
data/        Game data (scripts, spells, monsters, etc.)
src/         Core C++ source code
cmake/       Build configuration
.github/     CI workflows
```

---

## 📜 License

This project follows the same license as The Forgotten Server.
