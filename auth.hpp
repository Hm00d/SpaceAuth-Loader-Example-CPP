#pragma once

#include <cstdint>
#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace SpaceAuth {

    struct ClientConfig {
        std::string base_url;
        std::string app_public_id;
        std::string app_version;
        std::string device_label;
        std::string hwid_hash;
        std::string client_hash;
        std::string client_token;
        bool anti_debug_passed = true;
        std::string app_name;
    };

    struct ReleaseInfo {
        bool found = false;
        bool download_authorized = false;
        bool update_available = false;
        bool update_required = false;
        bool is_required = false;
        std::string channel;
        std::string latest_version;
        std::string file_id;
        std::string version;
        std::string file_name;
        std::string sha256;
    };

    struct RuntimeInfo {
        std::string app_public_id;
        std::string name;
        std::string slug;
        std::string status;
        std::string latest_version;
        std::string min_supported_version;
        std::string download_url;
        std::string disabled_reason;
        std::string maintenance_message;
        bool force_update_enabled = false;
        bool update_available = false;
        bool update_required = false;
        bool anti_crack_enabled = false;
    };

    struct BlacklistStatus {
        bool blacklisted = false;
        std::string rule_type;
        std::string match_value;
        std::string reason;
    };

    struct SubscriptionTierInfo {
        std::string id;
        std::string name;
        int level = -1;
        std::string status;
    };

    struct UserData {
        std::string license_expires_at;
        SubscriptionTierInfo subscription;
        std::vector<std::string> subscriptions;
    };

    struct LiveGuardEvent {
        bool should_exit = false;
        bool network_issue = false;
        std::string code;
        std::string message;
    };

    struct diagnostic_request {
        std::string id;
        std::string type;
        std::string prompt;
        std::string requested_at;
        std::string expires_at;
    };

    using LiveGuardCallback = std::function<void(const LiveGuardEvent&)>;

    struct response_data {
        bool success = false;
        std::string code;
        std::string message;
        std::string session_id;
    };

    struct application_data {
        std::string name;
        std::string app_public_id;
        std::string version;
        std::string status;
        std::string latest_version;
        std::string min_supported_version;
        std::string download_url;
        std::string disabled_reason;
        std::string maintenance_message;
        bool force_update_enabled = false;
        bool update_available = false;
        bool update_required = false;
        bool anti_crack_enabled = false;
    };

    namespace detail {

        constexpr std::uint64_t kObfuscationSalt = 0x8D2A6C3F9E17B5A1ULL;

        constexpr std::uint64_t fnv1a_constexpr(
            const char* value,
            std::uint64_t hash = 14695981039346656037ULL) {
            return *value == '\0'
                ? hash
                : fnv1a_constexpr(
                    value + 1,
                    (hash ^ static_cast<std::uint64_t>(*value)) * 1099511628211ULL);
        }

        constexpr std::uint64_t mix_seed(std::uint64_t value) {
            value ^= value >> 33;
            value *= 0xff51afd7ed558ccdULL;
            value ^= value >> 33;
            value *= 0xc4ceb9fe1a85ec53ULL;
            value ^= value >> 33;
            return value;
        }

        constexpr std::uint64_t make_seed(const char* file, int line, int counter) {
            return mix_seed(
                fnv1a_constexpr(file) ^
                (static_cast<std::uint64_t>(line) << 32) ^
                static_cast<std::uint64_t>(counter) ^
                kObfuscationSalt);
        }

        constexpr std::uint8_t key_byte(std::uint64_t seed, std::size_t index) {
            const auto mixed = mix_seed(seed + (index + 1) * 0x9E3779B97F4A7C15ULL);
            return static_cast<std::uint8_t>((mixed >> ((index % 8) * 8)) & 0xFFU);
        }

        template <std::size_t N, std::uint64_t Seed>
        class encrypted_literal {
        public:
            constexpr explicit encrypted_literal(const char(&value)[N]) : encrypted_{} {
                for (std::size_t i = 0; i < N; ++i) {
                    encrypted_[i] = static_cast<std::uint8_t>(value[i]) ^ key_byte(Seed, i);
                }
            }

            std::string str() const {
                std::string output;
                output.resize(N > 0 ? N - 1 : 0);
                for (std::size_t i = 0; i + 1 < N; ++i) {
                    output[i] = static_cast<char>(encrypted_[i] ^ key_byte(Seed, i));
                }
                return output;
            }

        private:
            std::array<std::uint8_t, N> encrypted_;
        };

        template <std::uint64_t Seed, std::size_t N>
        constexpr encrypted_literal<N, Seed> make_encrypted(const char(&value)[N]) {
            return encrypted_literal<N, Seed>(value);
        }

    }  // namespace detail

    class api {
    public:
        explicit api(ClientConfig config);
        api(const std::string& app_name,
            const std::string& app_public_id,
            const std::string& version,
            const std::string& url,
            const std::string& device_label = std::string(),
            const std::string& hwid_hash = "",
            const std::string& client_token = "",
            bool anti_debug_passed = true);
        ~api();

        api(api&& other) noexcept;
        api& operator=(api&& other) noexcept;

        api(const api&) = delete;
        api& operator=(const api&) = delete;

        void init();
        void fetch_runtime();
        void license(const std::string& key);
        void web_login(std::uint32_t timeout_seconds = 900, bool open_browser = true);
        void button(const std::string& key, std::uint32_t poll_interval_seconds = 2);
        bool check();
        BlacklistStatus checkblack();
        void ban(const std::string& reason = "");
        void logout();

        std::string var(const std::string& key);
        std::string getvar(const std::string& key);
        void setvar(const std::string& key, const std::string& value);
        void log(const std::string& message,
            const std::string& event_type = std::string());
        std::vector<diagnostic_request> diagnostics();
        void respond_diagnostic(const std::string& request_id,
            const std::string& status,
            const std::string& artifact_mime = std::string(),
            const std::string& artifact_base64 = std::string(),
            const std::string& client_message = std::string());
        std::size_t process_diagnostics(bool prompt_customer = true);
        void report_anti_crack(const std::string& code,
            const std::string& message,
            bool ban_on_detection);
        void report_anti_crack(const std::string& code,
            const std::string& message,
            const std::string& detector = std::string(),
            const std::string& evidence = std::string(),
            bool ban_on_detection = true);

        ReleaseInfo check_release(const std::string& channel = std::string());
        std::vector<std::uint8_t> download(const std::string& file_id);

        void start_heartbeat(std::uint32_t interval_seconds = 20,
            std::uint32_t offline_grace_seconds = 300,
            LiveGuardCallback callback = {});
        void stop_heartbeat();

        bool is_initialized() const;
        bool is_authenticated() const;
        bool subscription_active(const std::string& subscription_name) const;
        std::string client_hash() const;

        response_data response;
        application_data app_data;
        UserData user_data;

    private:
        class Impl;

        void set_success(const std::string& message);
        void set_failure(const std::string& fallback_code,
            const std::string& fallback_message);
        void sync_runtime(const RuntimeInfo& runtime);
        void sync_user_data();

        std::unique_ptr<Impl> impl_;
    };

    struct security_check_result {
        bool ok = true;
        bool baseline_ready = false;
        bool debugger_detected = false;
        bool memory_modified = false;
        std::string code;
        std::string message;
        std::string detector;
        std::string evidence;
    };

    class auth_guard {
    public:
        static security_check_result capture_baseline();
        static security_check_result check();
        static bool enforce(api* app = nullptr,
            bool ban_on_fail = false,
            const std::string& ban_reason = "");
    };

    class protected_loader_runtime {
    public:
        explicit protected_loader_runtime(api& app);
        ~protected_loader_runtime();

        protected_loader_runtime(protected_loader_runtime&& other) noexcept;
        protected_loader_runtime& operator=(protected_loader_runtime&& other) noexcept;

        protected_loader_runtime(const protected_loader_runtime&) = delete;
        protected_loader_runtime& operator=(const protected_loader_runtime&) = delete;

        bool start_anti_crack(std::uint32_t poll_seconds = 2,
            const std::string& startup_reason = "startup_security_check_failed");
        void start_live_guard(std::uint32_t interval_seconds = 20,
            std::uint32_t offline_grace_seconds = 300);
        void start_diagnostics(std::uint32_t poll_seconds = 7,
            bool prompt_customer = true);
        void stop();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

    bool report_and_enforce_security(api& app,
        const std::string& reason = "security_check_failed");
    std::string download_and_launch(api& app,
        const std::string& file_id,
        const std::string& output_file_name = "protected.bin",
        bool launch_after_download = true);

    struct protected_loader_options {
        std::string app_name;
        std::string app_public_id;
        std::string version;
        std::string base_url;
        std::string device_label;
        std::string file_id;
        std::string output_file_name;
        std::uint32_t heartbeat_interval_seconds = 20;
        std::uint32_t heartbeat_offline_grace_seconds = 300;
        std::uint32_t security_poll_seconds = 2;
        std::uint32_t diagnostics_poll_seconds = 7;
        bool launch_after_download = true;
        bool pause_before_exit = true;
    };

    int run_protected_loader(const protected_loader_options& options);
    int run_embedded_loader();

    inline ClientConfig make_config(const std::string& base_url,
        const std::string& app_public_id,
        const std::string& version,
        const std::string& device_label = std::string(),
        const std::string& hwid_hash = "",
        const std::string& client_token = "",
        bool anti_debug_passed = true,
        const std::string& app_name = "") {
        return ClientConfig{
            base_url,
            app_public_id,
            version,
            device_label,
            hwid_hash,
            "",
            client_token,
            anti_debug_passed,
            app_name,
        };
    }

}  // namespace SpaceAuth

#define SA_OBF(literal)                                                                    \
  ([]() -> std::string {                                                                   \
    constexpr auto encrypted = ::SpaceAuth::detail::make_encrypted<                       \
        ::SpaceAuth::detail::make_seed(__FILE__, __LINE__, __COUNTER__)>(literal);         \
    return encrypted.str();                                                                \
  }())

#define SPACEAUTH_OBF(literal) SA_OBF(literal)
