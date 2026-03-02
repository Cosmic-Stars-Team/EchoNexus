#ifndef ECHONEXUS_RESPONSE_HPP
#define ECHONEXUS_RESPONSE_HPP

#include <map>
#include <string>
#include <string_view>

#include <glaze/glaze.hpp>

namespace echo::type {

    /// @brief Get the standard HTTP status message for a given status code.
    constexpr std::string_view get_status_message(unsigned int status) {
        switch (status) {
            // --- 1xx Informational ---
            case 100: return "Continue";
            case 101: return "Switching Protocols";
            case 102: return "Processing";
            case 103: return "Early Hints";

            // --- 2xx Success ---
            case 200: return "OK";
            case 201: return "Created";
            case 202: return "Accepted";
            case 203: return "Non-Authoritative Information";
            case 204: return "No Content";
            case 205: return "Reset Content";
            case 206: return "Partial Content";
            case 207: return "Multi-Status";
            case 208: return "Already Reported";
            case 226: return "IM Used";

            // --- 3xx Redirection ---
            case 300: return "Multiple Choices";
            case 301: return "Moved Permanently";
            case 302: return "Found";
            case 303: return "See Other";
            case 304: return "Not Modified";
            case 305: return "Use Proxy";
            case 307: return "Temporary Redirect";
            case 308: return "Permanent Redirect";

            // --- 4xx Client Error ---
            case 400: return "Bad Request";
            case 401: return "Unauthorized";
            case 402: return "Payment Required";
            case 403: return "Forbidden";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            case 406: return "Not Acceptable";
            case 407: return "Proxy Authentication Required";
            case 408: return "Request Timeout";
            case 409: return "Conflict";
            case 410: return "Gone";
            case 411: return "Length Required";
            case 412: return "Precondition Failed";
            case 413: return "Payload Too Large";
            case 414: return "URI Too Long";
            case 415: return "Unsupported Media Type";
            case 416: return "Range Not Satisfiable";
            case 417: return "Expectation Failed";
            case 418: return "I'm a teapot";
            case 421: return "Misdirected Request";
            case 422: return "Unprocessable Entity";
            case 423: return "Locked";
            case 424: return "Failed Dependency";
            case 425: return "Too Early";
            case 426: return "Upgrade Required";
            case 428: return "Precondition Required";
            case 429: return "Too Many Requests";
            case 431: return "Request Header Fields Too Large";
            case 451: return "Unavailable For Legal Reasons";

            // --- 5xx Server Error ---
            case 500: return "Internal Server Error";
            case 501: return "Not Implemented";
            case 502: return "Bad Gateway";
            case 503: return "Service Unavailable";
            case 504: return "Gateway Timeout";
            case 505: return "HTTP Version Not Supported";
            case 506: return "Variant Also Negotiates";
            case 507: return "Insufficient Storage";
            case 508: return "Loop Detected";
            case 510: return "Not Extended";
            case 511: return "Network Authentication Required";

            default:  return "Unknown";
        }
    }

    struct response {
        /// @brief The HTTP status code of the response.
        unsigned int status = 200;
        /// @brief The HTTP status message of the response.
        std::string message = "OK";
        /// @brief The body of the response.
        std::string body = "";
        /// @brief Custom headers for the response.
        std::map<std::string, std::string> headers;

        response() = default;
        response(unsigned int code) : status(code), message(get_status_message(code)) {}
        response(unsigned int code, std::string message)
            : status(code), message(std::move(message)) {}

        /// @brief Set a custom header for the response.
        void set_header(const std::string &key, const std::string &value) {
            headers[key] = value;
        }

        /// @brief Set the body of the response and update Content-Length header.
        void set_body(const std::string &content) {
            body = content;

            set_header("Content-Length", std::to_string(content.size()));
        }

        /// @brief Set the Content-Type header.
        void set_content_type(const std::string &content_type) {
            set_header("Content-Type", content_type);
        }

        /// @brief Create an text response.
        static response text(std::string content, unsigned int code = 200) {
            response res(code);

            res.set_content_type("text/plain; charset=utf-8");
            res.set_body(std::move(content));

            return res;
        };

        /// @brief Create an HTML response from a string.
        static response html(std::string content, unsigned int code = 200) {
            response res(code);

            res.set_content_type("text/html; charset=utf-8");
            res.set_body(std::move(content));

            return res;
        };

        /// @brief Create a JSON response from a string.
        static response json(std::string data, unsigned int code) {
            response res(code);

            res.set_content_type("application/json; charset=utf-8");
            res.set_body(std::move(data));

            return res;
        }

        /// @brief Create a JSON response from a serializable object.
        template <typename T>
        requires (!std::is_same_v<std::decay_t<T>, std::string> &&
                  !std::is_same_v<std::decay_t<T>, const char*>)
        static response json(T data, unsigned int code = 200) {
            std::string buffer;
            auto ec = glz::write_json(data, buffer);

            if (ec) {
                response res(500);
                res.set_content_type("text/plain; charset=utf-8");
                res.set_body("Failed to serialize JSON response.");
                return res;
            }

            return json(std::move(buffer), code);
        }

        static response redirect(const std::string &url, unsigned int code = 302) {
            response res(code);

            res.set_header("Location", url);
            res.set_body("");

            return res;
        }
    };
}

#endif //ECHONEXUS_RESPONSE_HPP
