# Space Auth C++ Loader

Space Auth C++ Loader is a ready-to-ship Windows loader package for protected
Space Auth applications. It includes the public C++ SDK header, the x64 release
library, a compiled loader, runtime DLLs, and clean examples that show both the
simple protected-loader flow and the manual API flow.

The goal is simple: initialize your application, validate a license, keep the
session alive, enforce blacklist and anti-crack rules, then download and launch
the protected file that belongs to that application.

## What is Space Auth?

Space Auth is a licensing and protection platform for software developers. It
helps you manage applications, licenses, subscriptions, hosted files, runtime
variables, sessions, release checks, access rules, and client security events
from one dashboard.

The C++ SDK is designed for Windows loaders and protected desktop applications.
It keeps important enforcement on the server while giving the client a compact
API for license login, blacklist checks, protected downloads, anti-crack reports,
live heartbeat checks, and diagnostics.

## Package Contents

This release package contains:

- `bin/x64/Space Auth.exe` - compiled x64 loader from the current build.
- `bin/x64/libcurl.dll` - runtime DLL required by the loader.
- `bin/x64/libcrypto-3-x64.dll` - OpenSSL runtime DLL required by the loader.
- `bin/x64/z.dll` - zlib runtime DLL required by the loader.
- `include/auth.hpp` - public C++ SDK header.
- `lib/x64/Space Auth.lib` - x64 release static/import library.
- `src/main.cpp` - production loader source used for this build.
- `examples/main.cpp` - clean starter example for your own loader.

Keep the DLL files beside your final EXE unless your project links and deploys
those dependencies another way.

## Requirements

- Windows 10 or Windows 11.
- Visual Studio with the MSVC x64 C++ toolchain.
- C++20 enabled.
- Build platform set to `x64`.
- HTTPS backend URL for production.
- A Space Auth dashboard application with matching settings.

## Dashboard Setup

Before compiling your loader, create or open your application in the Space Auth
dashboard and make sure these values are ready:

1. Application Name
2. App Public ID
3. Current Version
4. Backend URL
5. License keys or subscriptions
6. Hosted file ID if your loader downloads a protected file
7. Anti Crack setting if you want security reports and automatic blacklist rules

The application name compiled in the loader must exactly match the dashboard
application name. If it does not match, the encrypted handshake is rejected.

The App Public ID is public by design. It identifies the application, but it is
not a secret. Never place server secrets, database credentials, dashboard tokens,
or private webhook URLs in a client binary.

## Quick Start

This is the recommended starter style. It behaves like a complete loader with a
single configuration object.

```cpp
#include <iostream>
#include "auth.hpp"

int main() {
    SpaceAuth::protected_loader_options options{};
    options.app_name = SA_OBF("Your Application Name");
    options.app_public_id = SA_OBF("YOUR_APP_PUBLIC_ID");
    options.version = SA_OBF("1.0.0");
    options.base_url = SA_OBF("https://your-domain.com");
    options.device_label = SA_OBF("Windows Loader");
    options.file_id = SA_OBF("your-file-word");
    options.output_file_name = SA_OBF("protected_payload.exe");

    options.heartbeat_interval_seconds = 20;
    options.heartbeat_offline_grace_seconds = 300;
    options.security_poll_seconds = 2;
    options.diagnostics_poll_seconds = 7;
    options.launch_after_download = true;
    options.pause_before_exit = true;

    try {
        return SpaceAuth::run_protected_loader(options);
    } catch (const std::exception& error) {
        std::cerr << "Space Auth loader failed: " << error.what() << "\n";
        return 1;
    }
}
```

`run_protected_loader` handles the common loader flow:

- Encrypted client handshake
- Application name verification
- Runtime application status checks
- Blacklist check
- License key login
- Anti Crack startup check
- Anti Crack background monitor
- Live heartbeat monitor
- Diagnostics polling
- Protected file download
- Optional protected file launch
- Logout on clean exit

## Instance Definition

Use the `api` class when you want full control over each step.

```cpp
#include "auth.hpp"

SpaceAuth::api SpaceAuthApp(
    SA_OBF("Your Application Name"),
    SA_OBF("YOUR_APP_PUBLIC_ID"),
    SA_OBF("1.0.0"),
    SA_OBF("https://your-domain.com"),
    SA_OBF("Windows Loader")
);
```

Parameters:

- `app_name` - must match the dashboard application name exactly.
- `app_public_id` - copied from the dashboard application.
- `version` - client version you want the backend to validate.
- `url` - your Space Auth backend URL.
- `device_label` - readable label shown in logs/sessions.

Optional constructor parameters also allow a custom HWID hash, client token, and
anti-debug state. Most loaders should keep the default values unless they have a
specific integration reason.

## Initialize Application

You must call `init()` before using authenticated SDK features.

```cpp
SpaceAuthApp.init();
if (!SpaceAuthApp.response.success) {
    std::cout << "\nStatus: " << SpaceAuthApp.response.message << "\n";
    return 1;
}
```

After init, the SDK fills `SpaceAuthApp.app_data` with runtime information such
as status, version rules, update state, and Anti Crack setting.

## Application Runtime Data

```cpp
std::cout << "Application: " << SpaceAuthApp.app_data.name << "\n";
std::cout << "Status: " << SpaceAuthApp.app_data.status << "\n";
std::cout << "Latest version: " << SpaceAuthApp.app_data.latest_version << "\n";
std::cout << "Anti Crack: "
          << (SpaceAuthApp.app_data.anti_crack_enabled ? "active" : "disabled")
          << "\n";
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
- `force_update_enabled`
- `update_available`
- `update_required`
- `anti_crack_enabled`

## Check Application State

You can block disabled applications or required updates immediately after init.

```cpp
if (SpaceAuthApp.app_data.status == SA_OBF("disabled")) {
    std::cout << "Application is disabled.\n";
    return 1;
}

if (SpaceAuthApp.app_data.update_required) {
    std::cout << "Required update available: "
              << SpaceAuthApp.app_data.latest_version << "\n";
    return 1;
}
```

## Check Blacklist Status

Call this early if you do not want blacklisted devices to reach the login prompt.

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

## Login With License Key

Use `license()` when your application only needs license-key login.

```cpp
std::string key;
std::cout << "Enter license key: ";
std::getline(std::cin >> std::ws, key);

SpaceAuthApp.license(key);
if (!SpaceAuthApp.response.success) {
    std::cout << "\nStatus: " << SpaceAuthApp.response.message << "\n";
    return 1;
}
```

After successful login, `SpaceAuthApp.user_data` contains the license
subscription information returned by the backend.

## User Data

```cpp
std::cout << "Session ID: " << SpaceAuthApp.response.session_id << "\n";

if (!SpaceAuthApp.user_data.subscription.name.empty()) {
    std::cout << "Subscription: "
              << SpaceAuthApp.user_data.subscription.name << "\n";
}

if (!SpaceAuthApp.user_data.license_expires_at.empty()) {
    std::cout << "License expires at: "
              << SpaceAuthApp.user_data.license_expires_at << "\n";
}
```

Useful fields:

- `license_expires_at`
- `subscription.id`
- `subscription.name`
- `subscription.level`
- `subscription.status`
- `subscriptions`

## Check Session Validation

Use `check()` to verify that the current session is still valid.

```cpp
if (!SpaceAuthApp.check()) {
    std::cout << "Session check failed: "
              << SpaceAuthApp.response.message << "\n";
    return 1;
}
```

For continuous checking, prefer the live heartbeat monitor.

## Live Heartbeat Monitor

The heartbeat monitor checks the session in the background and calls your
callback if the backend says the client should exit.

```cpp
SpaceAuthApp.start_heartbeat(
    20,
    300,
    [](const SpaceAuth::LiveGuardEvent& event) {
        std::cerr << "Live guard: " << event.message << "\n";
        if (!event.code.empty()) {
            std::cerr << "Code: " << event.code << "\n";
        }
        std::exit(1);
    });

// Before clean exit:
SpaceAuthApp.stop_heartbeat();
```

Parameters:

- `interval_seconds` - how often the client checks in.
- `offline_grace_seconds` - how long the client can tolerate network problems.
- `callback` - called when the guard wants the client to exit.

## Protected Loader Runtime

`protected_loader_runtime` is a helper that manages the security monitor,
heartbeat, and diagnostics threads for manual API users.

```cpp
SpaceAuth::protected_loader_runtime runtime(SpaceAuthApp);

if (SpaceAuthApp.app_data.anti_crack_enabled &&
    !runtime.start_anti_crack(2, SA_OBF("startup_security_check_failed"))) {
    return 1;
}

runtime.start_live_guard(20, 300);
runtime.start_diagnostics(7);

// Your protected work here.

runtime.stop();
```

Use this helper when you want a KeyAuth-like manual flow but do not want to
rewrite the background thread handling yourself.

## Anti Crack Guard

The SDK includes `auth_guard` for client-side security checks.

```cpp
SpaceAuth::auth_guard::capture_baseline();

if (!SpaceAuth::auth_guard::enforce(
        &SpaceAuthApp,
        true,
        SA_OBF("runtime_integrity_failed"))) {
    return 1;
}
```

When `ban_on_fail` is `true`, a security failure is reported to the backend. The
backend can create or reuse blacklist rules and log evidence for the application
owner.

Anti Crack reports can include:

- Detection code
- Detector name
- Human-readable message
- Blacklist application state
- Running process inventory
- Suspicious process/window information
- PNG screenshot evidence when screenshot capture is available

The client closes after the report path finishes. This protects the application
from continuing under a suspicious runtime.

## Report Anti Crack Manually

Use this if your own detector catches something suspicious.

```cpp
SpaceAuthApp.report_anti_crack(
    SA_OBF("CUSTOM_SECURITY_DETECTION"),
    SA_OBF("Suspicious behavior detected by custom guard."),
    SA_OBF("custom_guard"),
    SA_OBF("optional evidence text"),
    true);
```

The last parameter controls whether the backend should apply a blacklist/ban
action for the security event.

## Application Variables

Application variables are server-side values configured in the dashboard.

```cpp
std::string status = SpaceAuthApp.var(SA_OBF("status"));
std::cout << "status = " << status << "\n";
```

Variables can be global or authenticated depending on dashboard settings.

## User Variables

User variables belong to a license/user context.

```cpp
std::string discord = SpaceAuthApp.getvar(SA_OBF("discord"));
std::cout << "discord = " << discord << "\n";
```

Set a user variable:

```cpp
SpaceAuthApp.setvar(SA_OBF("discord"), SA_OBF("test#0001"));
if (!SpaceAuthApp.response.success) {
    std::cout << SpaceAuthApp.response.message << "\n";
}
```

## Application Logs

Use `log()` for important application events. Logs are stored in the Space Auth
dashboard.

```cpp
SpaceAuthApp.log(
    SA_OBF("Loader login completed."),
    SA_OBF("loader.license_login"));
```

Discord webhook forwarding is disabled in this package until it is enabled again
on the backend. Dashboard logs still work.

Do not log raw passwords, private tokens, full secrets, or sensitive file bytes.

## Ban The Current Client

Use `ban()` after login when your application decides the current client should
be blocked.

```cpp
SpaceAuthApp.ban(SA_OBF("Manual security ban"));
if (!SpaceAuthApp.response.success) {
    std::cout << SpaceAuthApp.response.message << "\n";
}
```

For pre-login security detections, use `report_anti_crack(..., true)` or
`auth_guard::enforce(&app, true, reason)`.

## Web Login

Use web login if you want the customer to authenticate through the website.

```cpp
SpaceAuthApp.web_login(900, true);
if (!SpaceAuthApp.response.success) {
    std::cout << SpaceAuthApp.response.message << "\n";
    return 1;
}

SpaceAuthApp.button(SA_OBF("continue"), 2);
```

Parameters:

- `timeout_seconds` - how long to wait for the website login.
- `open_browser` - whether the SDK should open the browser automatically.
- `button()` waits for a named web loader button/action.

## Release Check

Use release checks when your dashboard manages channels or required versions.

```cpp
const auto release = SpaceAuthApp.check_release(SA_OBF("stable"));
if (release.update_required) {
    std::cout << "Required version: " << release.latest_version << "\n";
    return 1;
}
```

Returned fields:

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

## Download Protected File

`download()` returns bytes for a protected dashboard file.

```cpp
std::vector<std::uint8_t> bytes = SpaceAuthApp.download(SA_OBF("your-file-word"));
if (!SpaceAuthApp.response.success) {
    std::cout << "Download failed: "
              << SpaceAuthApp.response.message << "\n";
    return 1;
}

std::ofstream file("protected_payload.exe", std::ios::binary);
file.write(reinterpret_cast<const char*>(bytes.data()),
           static_cast<std::streamsize>(bytes.size()));
```

The file ID must be the exact file word shown in the Files page of the
dashboard. File IDs are application-scoped, and the backend authorizes the
download against the current application/session.

## Download And Launch Helper

For loaders, use `download_and_launch()`.

```cpp
const std::string saved_path = SpaceAuth::download_and_launch(
    SpaceAuthApp,
    SA_OBF("your-file-word"),
    SA_OBF("protected_payload.exe"),
    true);

if (!SpaceAuthApp.response.success || saved_path.empty()) {
    std::cout << SpaceAuthApp.response.message << "\n";
    return 1;
}
```

Parameters:

- `app` - initialized and authenticated `SpaceAuth::api` instance.
- `file_id` - dashboard file word.
- `output_file_name` - local filename to write.
- `launch_after_download` - launches the file after saving when `true`.

The SDK sanitizes the output filename before writing to disk.

## Diagnostics

Diagnostics allow the dashboard to request client-side information from an
active loader session.

```cpp
std::size_t handled = SpaceAuthApp.process_diagnostics(true);
std::cout << "Diagnostics handled: " << handled << "\n";
```

Manual flow:

```cpp
auto requests = SpaceAuthApp.diagnostics();
for (const auto& request : requests) {
    SpaceAuthApp.respond_diagnostic(
        request.id,
        SA_OBF("completed"),
        SA_OBF("text/plain"),
        std::string(),
        SA_OBF("Diagnostic completed."));
}
```

Most loaders should use `protected_loader_runtime::start_diagnostics()` instead
of manually polling.

## Logout

Call `logout()` before a clean exit.

```cpp
SpaceAuthApp.stop_heartbeat();
SpaceAuthApp.logout();
```

If you use `protected_loader_runtime`, call `runtime.stop()` before `logout()`.

## Complete Manual Example

```cpp
#include <fstream>
#include <iostream>
#include <string>
#include "auth.hpp"

int main() {
    SpaceAuth::api app(
        SA_OBF("Your Application Name"),
        SA_OBF("YOUR_APP_PUBLIC_ID"),
        SA_OBF("1.0.0"),
        SA_OBF("https://your-domain.com"),
        SA_OBF("Windows Loader")
    );

    app.init();
    if (!app.response.success) {
        std::cout << app.response.message << "\n";
        return 1;
    }

    SpaceAuth::protected_loader_runtime runtime(app);
    if (app.app_data.anti_crack_enabled &&
        !runtime.start_anti_crack(2, SA_OBF("startup_security_check_failed"))) {
        return 1;
    }

    const auto blacklist = app.checkblack();
    if (blacklist.blacklisted) {
        std::cout << "Blacklisted: " << blacklist.reason << "\n";
        return 1;
    }

    std::string key;
    std::cout << "Enter license key: ";
    std::getline(std::cin >> std::ws, key);

    app.license(key);
    if (!app.response.success) {
        std::cout << app.response.message << "\n";
        return 1;
    }

    runtime.start_live_guard(20, 300);
    runtime.start_diagnostics(7);

    app.log(SA_OBF("Loader login completed."), SA_OBF("loader.license_login"));

    const std::string path = SpaceAuth::download_and_launch(
        app,
        SA_OBF("your-file-word"),
        SA_OBF("protected_payload.exe"),
        true);

    if (!app.response.success || path.empty()) {
        std::cout << app.response.message << "\n";
        runtime.stop();
        app.logout();
        return 1;
    }

    std::cout << "Protected file launched: " << path << "\n";

    runtime.stop();
    app.logout();
    return 0;
}
```

## Build With Visual Studio

Recommended project settings:

1. Create or open a C++ project.
2. Set the platform to `x64`.
3. Set the language standard to C++20.
4. Add `include` to your Additional Include Directories.
5. Add `lib/x64` to your Additional Library Directories.
6. Add `Space Auth.lib` to Additional Dependencies.
7. Copy `libcurl.dll`, `libcrypto-3-x64.dll`, and `z.dll` beside your output EXE.
8. Build in Release x64 for shipping.

Example layout:

```text
YourLoader/
  include/
    auth.hpp
  lib/
    x64/
      Space Auth.lib
  bin/
    x64/
      libcurl.dll
      libcrypto-3-x64.dll
      z.dll
  src/
    main.cpp
```

## Build From Developer Command Prompt

Visual Studio projects are recommended, but a simple command-line build can look
like this:

```bat
cl /std:c++20 /EHsc /I include examples\main.cpp /link /LIBPATH:lib\x64 "Space Auth.lib"
```

Copy the runtime DLLs next to the generated EXE before running it.

## Shipping Checklist

Before sending a loader to customers:

- Dashboard application name matches `options.app_name`.
- App Public ID matches `options.app_public_id`.
- Version matches your dashboard rules.
- Hosted file exists and the loader uses the correct file ID word.
- License/subscription rules are tested.
- Anti Crack is enabled if you expect security reports.
- Runtime DLLs are beside the EXE.
- Backend is deployed with the latest API build.
- HTTPS works on the production domain.
- No secrets are committed to GitHub.
- Debug messages and test file IDs are removed.
- The final EXE is tested from a clean folder.

## Common Errors

### APPLICATION_NAME_MISMATCH

The application name compiled into the loader does not exactly match the
dashboard application name. Check spaces, capitalization, and hidden characters.

### APP_NOT_FOUND

The App Public ID is wrong, deleted, or belongs to another application.

### LICENSE_INVALID

The license key is invalid, expired, inactive, already bound incorrectly, banned,
or not allowed by the current subscription/application rules.

### HWID_SESSION_MISMATCH

The session is being reused from another device or the HWID/session state no
longer matches the backend.

### Protected download failed

Check that the file ID word is correct, the file belongs to the same
application, the license has access, the backend can read the hosted file, and
the output filename is valid.

### Security check failed

Anti Crack detected a debugger, suspicious window, hook, memory modification, or
other analysis signal. The SDK reports the event and closes the loader.

### No screenshot in logs

Screenshot capture depends on Windows desktop access. If a screenshot is not
available, the backend should still receive the Anti Crack event and process
inventory. Test from an interactive desktop session, not a locked or headless
session.

### Missing DLL error

Place `libcurl.dll`, `libcrypto-3-x64.dll`, and `z.dll` in the same folder as
your EXE.

## Security Practices

Client-side code can always be inspected by a determined attacker, so treat the
loader as one layer of protection, not the whole system.

Recommended practices:

- Keep license validation, file authorization, blacklist rules, and subscription
  checks server-side.
- Use HTTPS only.
- Do not store server secrets in the client.
- Use `SA_OBF(...)` for static strings that appear in the binary.
- Keep Anti Crack enabled for production loaders.
- Keep heartbeat enabled so sessions can be revoked while running.
- Check blacklist status before login.
- Obfuscate and protect final production builds with your chosen commercial
  protector if needed.
- Do not write downloaded sensitive payloads to predictable public locations.
- Keep logs useful but avoid storing private customer secrets.

`SA_OBF(...)` helps hide static strings from simple string scans. It is not a
replacement for backend enforcement.

## Privacy And Compliance Note

Anti Crack evidence may include a screenshot and running process inventory when
a suspicious event is detected. Make sure your customer terms clearly explain
that security evidence can be collected after tampering, debugging, dumping, or
analysis attempts.

## Support Notes

When reporting an integration issue, include:

- SDK/package version.
- Visual Studio version.
- Build platform.
- Backend URL domain only, not secrets.
- The `response.code` and `response.message`.
- Whether the issue happens before init, after license login, during heartbeat,
  or during protected download.

Do not share license keys, database credentials, dashboard sessions, webhook
URLs, or private customer data in public issue reports.

## License

This package is intended for applications owned by the Space Auth customer who
received it. Do not publish private backend secrets, admin tokens, or customer
data with your loader source.
