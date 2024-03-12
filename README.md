# gemma-cpp-server: server for [gemma.cpp](https://github.com/google/gemma.cpp)

`gemma-cpp-server` provides a implement socket server for `gemma.cpp` and support python client for test.

## 🖥 Usage
- open vscode
  vscode remote connect wsl ubuntu 22.04

- download gemma-cpp-server
  ```bash
  git clone https://github.com/tdr1991/gemma-cpp-server.git
  ```

### release
- build release binary
  ```bash
  cmake -B build
  cd build
  make -j6 server
  ```
- run
  ```bash
  # start server, you should change model path
  ./server --tokenizer /mnt/d/work/models/gemma/tokenizer.spm --compressed_weights /mnt/d/work/models/gemma/2b-it-sfp.sbs --model 2b-it
  # start client 
  python -B client.py
  ```
- video example
  https://youtu.be/LWsYPaoJSl0?si=jP1irvllnRrv8Vvp

### debug


- build debug binary
  ```bash
  cmake -B debug -DCMAKE_BUILD_TYPE=Debug
  cd debug
  make -j6 server
  ```
- install gdb(if not)
  ```bash
  sudo apt install gdb
  ```

- start debug
  1. config setting json
   ```json
    # set these parameters in lauch.json 
    "program": "/mnt/d/work/llm/gemma-cpp-server/debug/server",    
    "cwd": "/mnt/d/work/llm/gemma-cpp-server",
    "MIMode": "gdb",
    "miDebuggerPath": "/usr/bin/gdb",

    # set these paramters in c_cpp_properties.json 
    "includePath": [
               "/mnt/d/work/llm/gemma-cpp-server"
            ],
   ```
   2. set input parameters
   ```bash
   # set args in lauch.json 
   "args": ["--tokenizer", 
            "/mnt/d/work/models/gemma/tokenizer.spm", 
            "--compressed_weights", 
            "/mnt/d/work/models/gemma/2b-it-sfp.sbs", "--model", "2b-it"],
   ```
   3. start debug
   insert breakpoint(F9)
   start debugging(F5)
   ```bash
   # if error: Cannot insert breakpoint xxx. Cannot access memory at address xxx
    # gdb版本为 12.1 执行以下命令
    echo -en '\x90\x90' | sudo dd of=/usr/bin/gdb count=2 bs=1 conv=notrunc seek=$((0x335C7D))
    # gdb版本为 12.0.90 ,执行以下命令
    echo -ne '\x90\x90' | sudo dd of=/usr/bin/gdb seek=$((0x335bad)) bs=1 count=2 conv=notrunc
    # problem detail : http://www.chn520.cn/article_detail/1699352563451046
   ```

## 🤝 Contributing
Contributions are welcome. Please clone the repository, push your changes to a new branch, and submit a pull request.

## 🙏 Acknowledgments
[gemma.cpp](https://github.com/google/gemma.cpp)
[gemma-cpp-python](https://github.com/namtranase/gemma-cpp-python)
