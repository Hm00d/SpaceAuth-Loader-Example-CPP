# Space Auth C++ Loader

Ready Windows C++ loader project for a Space Auth application.

This repo is meant to be opened in Visual Studio, configured with your
application values, built as x64 Release, then shipped with the required runtime
DLLs beside the EXE.

## Project Layout

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
- Visual Studio with MSVC C++ x64 tools.
- C++20.
- Release x64 build.
- Your backend URL must be reachable over HTTPS in production.

## Configure The Loader

Open:

```text
Space Auth/Space Auth.cpp
```

Edit these values:

```cpp
std::string name = SA_OBF("Your Application Name");
std::string appid = SA_OBF("YOUR_APP_PUBLIC_ID");
std::string version = SA_OBF("1.0.0");
std::string url = SA_OBF("https://your-domain.com");

api SpaceAuthApp(name, appid, version, url);
```

Required match rules:

- `name` must exactly match the application name in the dashboard.
- `appid` must match the App Public ID in the dashboard.
- `version` must match the application version configured in the dashboard.
- `url` must point to your backend.

If the version is changed and does not match the dashboard version, the backend
rejects the loader.

## Build

1. Open `Space Auth.slnx` in Visual Studio.
2. Select `Release`.
3. Select `x64`.
4. Build the project.

Output:

```text
Space Auth/x64/Release/Space Auth.exe
```

Keep these files beside the EXE:

```text
libcurl.dll
libcrypto-3-x64.dll
z.dll
```

## Basic Flow

The included loader does this:

1. Connects to the backend.
2. Validates application name, App Public ID, and version.
3. Checks blacklist status.
4. Asks for a license key.
5. Starts runtime security monitors.
6. Downloads the configured protected file.
7. Launches it.
8. Logs out on exit.

## Protected File

In `Space Auth.cpp`, update the file ID and output filename if needed:

```cpp
const std::string saved_path = download_and_launch(
    SpaceAuthApp,
    SA_OBF("your-file-id"),
    SA_OBF("protected.exe"),
    true);
```

The file ID must belong to the same application.

## Important API Data

Most SDK calls update:

```cpp
SpaceAuthApp.response
```

Useful fields:

- `response.success` - `true` when the last call worked.
- `response.code` - backend error/success code.
- `response.message` - readable status message.
- `response.session_id` - current runtime session ID when available.

Example:

```cpp
SpaceAuthApp.init();

if (!SpaceAuthApp.response.success) {
    std::cout << SpaceAuthApp.response.message << "\n";
    return 1;
}
```

## Application Data

After `init()`, runtime settings are available in:

```cpp
SpaceAuthApp.app_data
```

Useful fields:

- `name`
- `app_public_id`
- `version`
- `status`
- `latest_version`
- `min_supported_version`
- `download_url`
- `disabled_reason`
- `maintenance_message`
- `update_available`
- `update_required`
- `anti_crack_enabled`
- `min_sdk_version`
- `sdk_update_url`
- `sdk_update_required`

Example:

```cpp
if (SpaceAuthApp.app_data.status == SA_OBF("disabled")) {
    std::cout << "Application is disabled.\n";
    return 1;
}

if (SpaceAuthApp.app_data.update_required) {
    std::cout << "Loader update required.\n";
    return 1;
}
```

## Blacklist

Use `checkblack()` before asking for a license key:

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

Returned fields:

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

After a successful login, user data is available in:

```cpp
SpaceAuthApp.user_data
```

Useful fields:

- `license_expires_at`
- `subscription.name`
- `subscription.level`
- `subscriptions`

## Runtime Monitors

Use `protected_loader_runtime` to keep the loader protected after login:

```cpp
SpaceAuth::protected_loader_runtime runtime(SpaceAuthApp);

runtime.start_anti_crack();
runtime.start_live_guard();
runtime.start_diagnostics();
```

Useful methods:

- `start_anti_crack()` - detects debugging/tampering and reports it.
- `start_live_guard()` - keeps the session alive and exits if revoked.
- `start_diagnostics()` - handles dashboard diagnostic requests.
- `stop()` - stops background monitors before exit.

## API Reference

### Initialization

```cpp
SpaceAuthApp.init();
SpaceAuthApp.fetch_runtime();
SpaceAuthApp.logout();
```

- `init()` - opens the secure runtime session and loads app rules.
- `fetch_runtime()` - refreshes app runtime settings without license login.
- `logout()` - closes the current session.

### Session Checks

```cpp
bool ok = SpaceAuthApp.check();
bool active = SpaceAuthApp.is_authenticated();
```

- `check()` - validates the current live session.
- `is_initialized()` - returns whether `init()` succeeded.
- `is_authenticated()` - returns whether a license/web login is active.
- `subscription_active("pro")` - checks if the current user has a subscription name.

### Variables

```cpp
std::string global_value = SpaceAuthApp.var(SA_OBF("status"));
std::string user_value = SpaceAuthApp.getvar(SA_OBF("discord"));

SpaceAuthApp.setvar(SA_OBF("discord"), SA_OBF("user#0001"));
```

- `var(key)` - reads an application/global variable.
- `getvar(key)` - reads a user/license variable.
- `setvar(key, value)` - writes a user/license variable.

### Logs

```cpp
SpaceAuthApp.log(
    SA_OBF("Loader login completed."),
    SA_OBF("loader.license_login")
);
```

- `log(message, event_type)` - writes a client event to dashboard logs.

### Web Login And Buttons

```cpp
SpaceAuthApp.web_login(900, true);
SpaceAuthApp.button(SA_OBF("start"), 2);
```

- `web_login(timeout_seconds, open_browser)` - starts a web loader login flow.
- `button(key, poll_interval_seconds)` - waits for a dashboard/web-loader button command.

### Releases

```cpp
auto release = SpaceAuthApp.check_release(SA_OBF("stable"));

if (release.found && release.update_available) {
    std::cout << "Latest: " << release.latest_version << "\n";
}
```

`ReleaseInfo` fields:

- `found`
- `download_authorized`
- `update_available`
- `update_required`
- `is_required`
- `channel`
- `latest_version`
- `file_id`
- `version`
- `file_name`
- `sha256`

### Download Bytes

```cpp
auto bytes = SpaceAuthApp.download(SA_OBF("your-file-id"));

if (!SpaceAuthApp.response.success) {
    std::cout << SpaceAuthApp.response.message << "\n";
}
```

- `download(file_id)` - downloads a hosted file as bytes.
- `download_and_launch(app, file_id, output_file_name, true)` - downloads to disk and optionally launches.

### Manual Anti Crack

```cpp
auto check = SpaceAuth::auth_guard::check();

if (!check.ok) {
    SpaceAuthApp.report_anti_crack(
        check.code,
        check.message,
        check.detector,
        check.evidence,
        true
    );
    return 1;
}
```

Useful security fields:

- `ok`
- `debugger_detected`
- `memory_modified`
- `code`
- `message`
- `detector`
- `evidence`

Manual helpers:

- `auth_guard::capture_baseline()` - captures process baseline.
- `auth_guard::check()` - returns a security result.
- `auth_guard::enforce(&app, true, "reason")` - reports and blocks on failure.
- `report_anti_crack(...)` - sends a security event to logs and can blacklist.
- `ban(reason)` - manually requests a blacklist/ban action.

### Diagnostics

```cpp
auto requests = SpaceAuthApp.diagnostics();

for (const auto& request : requests) {
    SpaceAuthApp.respond_diagnostic(
        request.id,
        SA_OBF("completed"),
        {},
        {},
        SA_OBF("Handled by loader")
    );
}
```

- `diagnostics()` - reads pending diagnostic requests.
- `respond_diagnostic(...)` - responds to one request.
- `process_diagnostics(true)` - polls and handles diagnostics automatically.

`diagnostic_request` fields:

- `id`
- `type`
- `prompt`
- `requested_at`
- `expires_at`

### Heartbeat

```cpp
SpaceAuthApp.start_heartbeat(20, 300, [](const SpaceAuth::LiveGuardEvent& event) {
    std::cerr << event.message << "\n";
    std::exit(1);
});

SpaceAuthApp.stop_heartbeat();
```

`LiveGuardEvent` fields:

- `should_exit`
- `network_issue`
- `code`
- `message`

### Full Config Object

Use `ClientConfig` only when you need lower-level control:

```cpp
SpaceAuth::ClientConfig config{};
config.base_url = SA_OBF("https://your-domain.com");
config.app_public_id = SA_OBF("YOUR_APP_PUBLIC_ID");
config.app_version = SA_OBF("1.0.0");
config.app_name = SA_OBF("Your Application Name");
config.client_token = SA_OBF("optional-client-token");

SpaceAuth::api app(config);
```

Common fields:

- `base_url`
- `app_public_id`
- `app_version`
- `app_name`
- `hwid_hash`
- `client_hash`
- `client_token`
- `anti_debug_passed`
- `sdk_version`
- `sdk_build_id`

Most loaders should use the simple constructor unless they need these advanced
fields.

### One-Call Loader

For a compact loader, use `run_protected_loader`:

```cpp
SpaceAuth::protected_loader_options options{};
options.app_name = SA_OBF("Your Application Name");
options.app_public_id = SA_OBF("YOUR_APP_PUBLIC_ID");
options.version = SA_OBF("1.0.0");
options.base_url = SA_OBF("https://your-domain.com");
options.file_id = SA_OBF("your-file-id");
options.output_file_name = SA_OBF("protected.exe");

return SpaceAuth::run_protected_loader(options);
```

Useful options:

- `app_name`
- `app_public_id`
- `version`
- `base_url`
- `file_id`
- `output_file_name`
- `heartbeat_interval_seconds`
- `heartbeat_offline_grace_seconds`
- `security_poll_seconds`
- `diagnostics_poll_seconds`
- `launch_after_download`
- `pause_before_exit`

### SDK Helpers

```cpp
std::cout << SpaceAuthApp.client_hash() << "\n";
std::cout << SpaceAuthApp.sdk_version() << "\n";
std::cout << SpaceAuthApp.sdk_build_id() << "\n";
```

- `client_hash()` - current executable hash used by hash checks.
- `sdk_version()` - SDK version compiled into the loader.
- `sdk_build_id()` - SDK build identifier.

## SDK Version Gate

The SDK has its own version:

```cpp
SpaceAuth::SDK_VERSION
SpaceAuth::SDK_BUILD_ID
```

The platform admin can block old SDK builds from the admin panel. This is
separate from the application version in `Space Auth.cpp`.

## Common Errors

`APPLICATION_NAME_MISMATCH`

The compiled app name does not match the dashboard application name.

`APPLICATION_VERSION_MISMATCH`

The loader version does not match the dashboard application version.

`SDK_UPDATE_REQUIRED`

The SDK build is older than the platform minimum SDK version.

`LICENSE_INVALID`

The license key is invalid, expired, banned, or not allowed for this app.

`Protected download failed`

Check the file ID, license access, hosted file status, and backend deployment.

`Missing DLL`

Put `libcurl.dll`, `libcrypto-3-x64.dll`, and `z.dll` next to the EXE.

## Notes

- Do not put backend secrets, admin tokens, database credentials, or private
  webhook URLs in the loader.
- Use `SA_OBF(...)` for static strings you compile into the EXE.
- Test the final EXE from a clean folder before uploading it.
