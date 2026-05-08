# Space Auth C++ Loader

Windows C++ loader project for a Space Auth application.

This package exposes only the loader-facing API needed by `Space Auth.cpp`.
Advanced SDK internals are kept inside `Space Auth.lib` and are not declared in
the public loader header.

## Layout

```text
Space Auth Loader/
  README.md
  Space Auth.slnx
  Space Auth/
    Space Auth.cpp
    auth.hpp
    Space Auth.lib
    Space Auth.vcxproj
    x64/Release/
      Space Auth.exe
      libcurl.dll
      libcrypto-3-x64.dll
      z.dll
```

## Requirements

- Windows 10 or Windows 11.
- Visual Studio with MSVC x64 tools.
- C++20.
- Release x64 build.
- Backend URL reachable by the customer machine.

## Configure

Open:

```text
Space Auth/Space Auth.cpp
```

Edit:

```cpp
std::string name = SA_OBF("Your Application Name");
std::string appid = SA_OBF("YOUR_APP_PUBLIC_ID");
std::string version = SA_OBF("1.0.0");
std::string url = SA_OBF("https://your-domain.com");

api SpaceAuthApp(name, appid, version, url);
```

Rules:

- `name` must match the dashboard application name.
- `appid` must match the App Public ID.
- `version` must match the dashboard application version.
- `url` must point to your backend.

If the name, App Public ID, or version is wrong, the backend rejects the loader.

## Build

1. Open `Space Auth.slnx`.
2. Select `Release`.
3. Select `x64`.
4. Build.

Output:

```text
Space Auth/x64/Release/Space Auth.exe
```

Keep these DLLs beside the EXE:

```text
libcurl.dll
libcrypto-3-x64.dll
z.dll
```

## Loader Flow

The included loader:

1. Connects to the backend.
2. Validates app name, App Public ID, and version.
3. Starts the runtime security layer when enabled.
4. Checks blacklist status.
5. Asks for a license key.
6. Starts live guard and diagnostics.
7. Downloads the protected file.
8. Launches it.
9. Logs out on exit.

## Response Data

Most calls update:

```cpp
SpaceAuthApp.response
```

Fields:

- `success`
- `code`
- `message`
- `session_id`

Example:

```cpp
SpaceAuthApp.init();

if (!SpaceAuthApp.response.success) {
    std::cout << SpaceAuthApp.response.message << "\n";
    return 1;
}
```

## Application Data

After `init()`:

```cpp
SpaceAuthApp.app_data
```

Fields:

- `name`
- `app_public_id`
- `version`
- `status`
- `latest_version`
- `min_supported_version`
- `download_url`
- `disabled_reason`
- `maintenance_message`
- `force_update_enabled`
- `update_available`
- `update_required`
- `anti_crack_enabled`

Example:

```cpp
if (SpaceAuthApp.app_data.status == SA_OBF("disabled")) {
    return 1;
}

if (SpaceAuthApp.app_data.update_required) {
    return 1;
}
```

## Blacklist

```cpp
const auto blacklist = SpaceAuthApp.checkblack();

if (blacklist.blacklisted) {
    std::cout << "This device is blacklisted.\n";

    if (!blacklist.reason.empty()) {
        std::cout << "Reason: " << blacklist.reason << "\n";
    }

    return 1;
}
```

Fields:

- `blacklisted`
- `rule_type`
- `match_value`
- `reason`

## License Login

```cpp
std::string license_key;
std::cout << "Enter license key: ";
std::getline(std::cin >> std::ws, license_key);

SpaceAuthApp.license(license_key);

if (!SpaceAuthApp.response.success) {
    std::cout << SpaceAuthApp.response.message << "\n";
    return 1;
}
```

After login:

```cpp
SpaceAuthApp.user_data
```

Fields:

- `license_expires_at`
- `subscription.id`
- `subscription.name`
- `subscription.level`
- `subscription.status`
- `subscriptions`

## Runtime Protection

```cpp
SpaceAuth::protected_loader_runtime runtime(SpaceAuthApp);

if (SpaceAuthApp.app_data.anti_crack_enabled &&
    !runtime.start_anti_crack()) {
    return 1;
}

runtime.start_live_guard(20, 300);
runtime.start_diagnostics(7);
```

Methods:

- `start_anti_crack(...)`
- `start_live_guard(...)`
- `start_diagnostics(...)`
- `stop()`

## Protected Download

```cpp
const std::string saved_path = download_and_launch(
    SpaceAuthApp,
    SA_OBF("your-file-id"),
    SA_OBF("protected.exe"),
    true);

if (!SpaceAuthApp.response.success || saved_path.empty()) {
    std::cout << SpaceAuthApp.response.message << "\n";
    return 1;
}
```

The file ID must belong to the same application.

## Public Loader API

The loader header exposes only:

- `api(...)`
- `init()`
- `license(...)`
- `checkblack()`
- `log(...)`
- `logout()`
- `protected_loader_runtime`
- `download_and_launch(...)`
- `SA_OBF(...)`

Other SDK behavior is handled internally by `Space Auth.lib`.

## SDK Version Gate

The SDK version is embedded inside the library and sent internally to the
backend. It is not configured in `Space Auth.cpp` and is not exposed as a public
constant in `auth.hpp`.

The platform admin can block old SDK builds from the admin panel. This gate is
separate from the application version.

## Common Errors

`APPLICATION_NAME_MISMATCH`

The compiled app name does not match the dashboard application name.

`APPLICATION_VERSION_MISMATCH`

The loader version does not match the dashboard application version.

`SDK_UPDATE_REQUIRED`

The SDK library is older than the platform minimum SDK version.

`LICENSE_INVALID`

The license key is invalid, expired, banned, or not allowed for this app.

`Protected download failed`

Check the file ID, license access, hosted file status, and backend deployment.

`Missing DLL`

Put `libcurl.dll`, `libcrypto-3-x64.dll`, and `z.dll` next to the EXE.

## Notes

- Do not put backend secrets, admin tokens, database credentials, or webhook
  URLs in the loader.
- Use `SA_OBF(...)` for strings compiled into the EXE.
- Test the final EXE from a clean folder before uploading it.
