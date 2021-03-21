# loader

This is an EFI application written in Rust, which loads liumOS.

## References
- https://os.phil-opp.com/
- https://medium.com/@gil0mendes/an-efi-app-a-bit-rusty-82c36b745f49

## Tests

Invoke `make test` or `cargo test` to run the whole tests.

For running a specific test (ex. `src/physical_page_allocator`), run with `--test <test_name>` flag, like: `cargo test --test physical_page_allocator`
