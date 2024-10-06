# async-fs

`async-fs` is a powerful C++ extension for Python that provides asynchronous file system operations using `liburing` for high-performance I/O. This extension allows you to perform read, write, delete, copy, move, and file information retrieval operations asynchronously, making it an ideal choice for applications requiring high efficiency.

## Features

- **Asynchronous File Reading**: Read data from files without blocking the main thread.
- **Asynchronous File Writing**: Write data to files asynchronously, ensuring high performance.
- **Asynchronous File Deletion**: Delete files without blocking, useful for managing temporary files.
- **Asynchronous File Copying**: Copy the contents of one file to another asynchronously.
- **Asynchronous File Moving**: Move files from one location to another without blocking.
- **Asynchronous File Information Retrieval**: Get file information such as size, last modification time, and access permissions.

## Installation

Before installing `async-fs`, ensure that the `liburing` library is installed.

### Installing liburing

#### On Ubuntu/Debian:

```bash
sudo apt install liburing-dev
```

#### On Fedora:

```bash
sudo dnf install liburing-devel
```

#### On Arch Linux:

```bash
sudo pacman -S liburing
```

### Installing async-fs

Once `liburing` is installed, you can install `async-fs` via pip:

```bash
pip install async-fs
```

## Usage

Here's an example of using `async-fs` for asynchronous file reading and writing:

```python
import asyncio
import async_fs

async def main():
    # Asynchronous file reading
    content = await async_fs.read_file('test.txt', 1024)
    print(content)

    # Asynchronous file writing
    result = await async_fs.write_file('output.txt', 'Hello, async world!')
    print(f"Bytes written: {result}")

    # Asynchronous file deletion
    await async_fs.delete_file('output.txt')

    # Asynchronous file copying
    await async_fs.copy_file('test.txt', 'copy_test.txt')

    # Asynchronous file moving
    await async_fs.move_file('copy_test.txt', 'moved_test.txt')

    # Asynchronous file information retrieval
    info = await async_fs.file_info('moved_test.txt')
    print(info)

asyncio.run(main())
```

## Contributing

We welcome contributions from the community! If you have ideas for improving `async-fs`, please create an issue or submit a pull request. We appreciate your suggestions and enhancements.

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Acknowledgements

I would like to express my gratitude to the following tools and platforms that greatly contributed to the development of this project:

- **ChatGPT** and **OpenAI** — for providing powerful AI assistance and generating insights and code suggestions that accelerated the development process.
- **Cursor IDE** — for offering a seamless and efficient coding environment that helped improve the speed and quality of the project's development.
- **JetBrains** — for their comprehensive suite of development tools and IDEs, which significantly enhanced productivity and code management throughout the project.

These tools and platforms played an essential role in making this project possible, and I am truly grateful for their contributions.

I also want to extend a big thank you to everyone who supports and contributes to this project. Your help continues to make `async-fs` better!
