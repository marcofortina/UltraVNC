# Native Linux DSM/SecureVNC provider ABI

The native Linux server does not load Windows `.dsm` DLLs.  It exposes a Linux-native shared-object provider ABI so DSM/SecureVNC-style stream transforms can be implemented without importing Win32 plugin code.

Required exported symbols:

```c
unsigned int uvnc_dsm_provider_abi_version(void);       /* must return 1 */
const char *uvnc_dsm_provider_name(void);               /* provider display name */
int uvnc_dsm_provider_transform(
    int direction,
    const unsigned char *input,
    size_t input_length,
    unsigned char *output,
    size_t *output_length);
```

Optional exported symbol:

```c
const char *uvnc_dsm_provider_capabilities(void);
```

`direction` is `0` for client-to-server data and `1` for server-to-client data.  The provider must return `0` on success.  If `output_length` is too small, the provider should set the required size and return non-zero.

Server configuration:

```text
dsm_provider=/usr/local/lib/ultravnc/libuvnc-securevnc-provider.so
```

CLI equivalent:

```sh
uvnc_winvnc_memory_server --dsm-provider /usr/local/lib/ultravnc/libuvnc-securevnc-provider.so --validate-config
```

Operational constraints:

- the provider path must be absolute;
- legacy Windows `--dsm-plugin` / `.dsm` DLL loading remains rejected;
- this ABI is the Linux boundary for SecureVNC-compatible providers, but a real SecureVNC provider shared object still needs to be implemented and reviewed;
- provider code runs in the server process and must be treated as trusted native code.
