#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace SpaceAuth {

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
        std::string reserved0;
        std::string reserved1;
        bool force_update_enabled = false;
        bool update_available = false;
        bool update_required = false;
        bool reserved2 = false;
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

    namespace detail {
        constexpr std::uint64_t x0 = 0x8D2A6C3F9E17B5A1ULL;

        constexpr std::uint64_t x1(const char* v, std::uint64_t h = 14695981039346656037ULL) {
            return *v == '\0' ? h : x1(v + 1, (h ^ static_cast<std::uint64_t>(*v)) * 1099511628211ULL);
        }

        constexpr std::uint64_t x2(std::uint64_t v) {
            v ^= v >> 33;
            v *= 0xff51afd7ed558ccdULL;
            v ^= v >> 33;
            v *= 0xc4ceb9fe1a85ec53ULL;
            v ^= v >> 33;
            return v;
        }

        constexpr std::uint64_t x3(const char* f, int l, int c) {
            return x2(x1(f) ^ (static_cast<std::uint64_t>(l) << 32) ^ static_cast<std::uint64_t>(c) ^ x0);
        }

        constexpr std::uint8_t x4(std::uint64_t s, std::size_t i) {
            const auto m = x2(s + (i + 1) * 0x9E3779B97F4A7C15ULL);
            return static_cast<std::uint8_t>((m >> ((i % 8) * 8)) & 0xFFU);
        }

        template <std::size_t N, std::uint64_t S>
        class x5 {
        public:
            constexpr explicit x5(const char(&v)[N]) : d_{} {
                for (std::size_t i = 0; i < N; ++i) {
                    d_[i] = static_cast<std::uint8_t>(v[i]) ^ x4(S, i);
                }
            }

            std::string str() const {
                std::string out;
                out.resize(N > 0 ? N - 1 : 0);
                for (std::size_t i = 0; i + 1 < N; ++i) {
                    out[i] = static_cast<char>(d_[i] ^ x4(S, i));
                }
                return out;
            }

        private:
            std::array<std::uint8_t, N> d_;
        };

        template <std::uint64_t S, std::size_t N>
        constexpr x5<N, S> x6(const char(&v)[N]) {
            return x5<N, S>(v);
        }
    }

    class api {
    public:
        api(const std::string&,
            const std::string&,
            const std::string&,
            const std::string&,
            const std::string& = std::string(),
            const std::string& = "",
            const std::string& = "",
            bool = true);
        ~api();

        api(api&& other) noexcept;
        api& operator=(api&& other) noexcept;
        api(const api&) = delete;
        api& operator=(const api&) = delete;

        void init();
        void license(const std::string&);
        BlacklistStatus checkblack();
        void log(const std::string&, const std::string& = std::string());
        void logout();

        response_data response;
        application_data app_data;
        UserData user_data;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

    class protected_loader_runtime {
    public:
        explicit protected_loader_runtime(api& app);
        ~protected_loader_runtime();

        protected_loader_runtime(protected_loader_runtime&& other) noexcept;
        protected_loader_runtime& operator=(protected_loader_runtime&& other) noexcept;
        protected_loader_runtime(const protected_loader_runtime&) = delete;
        protected_loader_runtime& operator=(const protected_loader_runtime&) = delete;

        bool start_anti_crack(std::uint32_t = 2,
            const std::string& = std::string());
        void start_live_guard(std::uint32_t = 20,
            std::uint32_t = 300);
        void start_diagnostics(std::uint32_t = 7,
            bool = true);
        void stop();

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

    std::string download_and_launch(api&,
        const std::string&,
        const std::string& = std::string(),
        bool = true);
}

#define SA_OBF(literal) ([]() -> std::string { \
    constexpr auto x = ::SpaceAuth::detail::x6< \
        ::SpaceAuth::detail::x3(__FILE__, __LINE__, __COUNTER__)>(literal); \
    return x.str(); \
}())

#define SPACEAUTH_OBF(literal) SA_OBF(literal)
