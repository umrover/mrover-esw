/*
  influxdb-cpp -- 💜 C++ client for InfluxDB.

  Copyright (c) 2010-2018 <http://ez8.co> <orca.zhang@yahoo.com>
  This library is released under the MIT License.

  Please see LICENSE file or visit https://github.com/orca-zhang/influxdb-cpp for details.
 */

#ifndef INFLUXDB_CPP_HPP
#define INFLUXDB_CPP_HPP

#include <cstdio>
#include <cstring>
#include <sstream>

#ifdef _WIN32
#define NOMINMAX
#include <algorithm>
#include <windows.h>
#pragma comment(lib, "ws2_32")
typedef struct iovec {
    void* iov_base;
    size_t iov_len;
} iovec;
inline __int64 writev(int sock, struct iovec* iov, int cnt) {
    __int64 r = send(sock, (char const*) iov->iov_base, iov->iov_len, 0);
    return (r < 0 || cnt == 1) ? r : r + writev(sock, iov + 1, cnt - 1);
}
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#define closesocket close
#endif

namespace influxdb_cpp {
    struct server_info {
        std::string host_;
        int port_;
        std::string db_;
        std::string usr_;
        std::string pwd_;
        std::string precision_;
        std::string token_;
        server_info(std::string const& host, int port, std::string const& db = "", std::string const& usr = "", std::string const& pwd = "", std::string const& precision = "ms", std::string const& token = "")
            : host_(host), port_(port), db_(db), usr_(usr), pwd_(pwd), precision_(precision), token_(token) {}
    };
    namespace detail {
        struct meas_caller;
        struct tag_caller;
        struct field_caller;
        struct ts_caller;
        struct inner {
            static int http_request(char const*, char const*, std::string const&, std::string const&, server_info const&, std::string*, unsigned timeout_sec = 0);
            static inline unsigned char to_hex(unsigned char x) { return x > 9 ? x + 55 : x + 48; }
        };
    } // namespace detail
    static void url_encode(std::string& out, std::string const& src);

    inline int query(std::string& resp, std::string const& query, server_info const& si, unsigned timeout_sec = 0) {
        std::string qs("&q=");
        url_encode(qs, query);
        return detail::inner::http_request("GET", "query", qs, "", si, &resp, timeout_sec);
    }
    inline int create_db(std::string& resp, std::string const& db_name, server_info const& si, unsigned timeout_sec = 0) {
        std::string qs("&q=create+database+");
        url_encode(qs, db_name);
        return detail::inner::http_request("POST", "query", qs, "", si, &resp, timeout_sec);
    }

    struct builder {
        detail::tag_caller& meas(std::string const& m) {
            lines_.imbue(std::locale("C"));
            lines_.clear();
            return _m(m);
        }

    protected:
        detail::tag_caller& _m(std::string const& m) {
            _escape(m, ", ");
            return reinterpret_cast<detail::tag_caller&>(*this);
        }
        detail::tag_caller& _t(std::string const& k, std::string const& v) {
            lines_ << ',';
            _escape(k, ",= ");
            lines_ << '=';
            _escape(v, ",= ");
            return reinterpret_cast<detail::tag_caller&>(*this);
        }
        detail::field_caller& _f_s(char delim, std::string const& k, std::string const& v) {
            lines_ << delim;
            _escape(k, ",= ");
            lines_ << "=\"";
            _escape(v, "\"");
            lines_ << '\"';
            return reinterpret_cast<detail::field_caller&>(*this);
        }
        detail::field_caller& _f_i(char delim, std::string const& k, long long v) {
            lines_ << delim;
            _escape(k, ",= ");
            lines_ << '=';
            lines_ << v << 'i';
            return reinterpret_cast<detail::field_caller&>(*this);
        }
        detail::field_caller& _f_f(char delim, std::string const& k, double v, int prec) {
            lines_ << delim;
            _escape(k, ",= ");
            lines_.precision(prec);
            lines_.setf(std::ios::fixed);
            lines_ << '=' << v;
            return reinterpret_cast<detail::field_caller&>(*this);
        }
        detail::field_caller& _f_b(char delim, std::string const& k, bool v) {
            lines_ << delim;
            _escape(k, ",= ");
            lines_ << '=' << (v ? 't' : 'f');
            return reinterpret_cast<detail::field_caller&>(*this);
        }
        detail::ts_caller& _ts(long long ts) {
            lines_ << ' ' << ts;
            return reinterpret_cast<detail::ts_caller&>(*this);
        }
        int _post_http(server_info const& si, std::string* resp, unsigned timeout_sec = 0) {
            return detail::inner::http_request("POST", "write", "", lines_.str(), si, resp, timeout_sec);
        }
        int _send_udp(std::string const& host, int port) {
            int sock, ret = 0;
            struct sockaddr_in addr;

            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            if ((addr.sin_addr.s_addr = inet_addr(host.c_str())) == INADDR_NONE) return -1;

            if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return -2;

            lines_ << '\n';
            if (sendto(sock, &lines_.str()[0], lines_.str().length(), 0, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)), static_cast<int>(lines_.str().length()))
                ret = -3;

            closesocket(sock);
            return ret;
        }
        void _escape(std::string const& src, char const* escape_seq) {
            size_t pos = 0, start = 0;
            while ((pos = src.find_first_of(escape_seq, start)) != std::string::npos) {
                lines_.write(src.c_str() + start, pos - start);
                lines_ << '\\' << src[pos];
                start = ++pos;
            }
            lines_.write(src.c_str() + start, src.length() - start);
        }

        std::stringstream lines_;
    };
    inline void url_encode(std::string& out, std::string const& src) {
        size_t pos = 0, start = 0;
        while ((pos = src.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~", start)) != std::string::npos) {
            out.append(src.c_str() + start, pos - start);
            if (src[pos] == ' ')
                out += "+";
            else {
                out += '%';
                out += detail::inner::to_hex(static_cast<unsigned char>(src[pos]) >> 4);
                out += detail::inner::to_hex(static_cast<unsigned char>(src[pos]) & 0xF);
            }
            start = ++pos;
        }
        out.append(src.c_str() + start, src.length() - start);
    }

    namespace detail {
        struct tag_caller : public builder {
            detail::tag_caller& tag(std::string const& k, std::string const& v) { return _t(k, v); }
            detail::field_caller& field(std::string const& k, std::string const& v) { return _f_s(' ', k, v); }
            detail::field_caller& field(std::string const& k, bool v) { return _f_b(' ', k, v); }
            detail::field_caller& field(std::string const& k, short v) { return _f_i(' ', k, v); }
            detail::field_caller& field(std::string const& k, int v) { return _f_i(' ', k, v); }
            detail::field_caller& field(std::string const& k, long v) { return _f_i(' ', k, v); }
            detail::field_caller& field(std::string const& k, long long v) { return _f_i(' ', k, v); }
            detail::field_caller& field(std::string const& k, double v, int prec = 2) { return _f_f(' ', k, v, prec); }

        private:
            detail::tag_caller& meas(std::string const& m);
        };
        struct ts_caller : public builder {
            detail::tag_caller& meas(std::string const& m) {
                lines_ << '\n';
                return _m(m);
            }
            int post_http(server_info const& si, std::string* resp = NULL,
                          unsigned timeout_sec = 0) { return _post_http(si, resp, timeout_sec); }
            int send_udp(std::string const& host, int port) { return _send_udp(host, port); }
        };
        struct field_caller : public ts_caller {
            detail::field_caller& field(std::string const& k, std::string const& v) { return _f_s(',', k, v); }
            detail::field_caller& field(std::string const& k, bool v) { return _f_b(',', k, v); }
            detail::field_caller& field(std::string const& k, short v) { return _f_i(',', k, v); }
            detail::field_caller& field(std::string const& k, int v) { return _f_i(',', k, v); }
            detail::field_caller& field(std::string const& k, long v) { return _f_i(',', k, v); }
            detail::field_caller& field(std::string const& k, long long v) { return _f_i(',', k, v); }
            detail::field_caller& field(std::string const& k, double v, int prec = 2) { return _f_f(',', k, v, prec); }
            detail::ts_caller& timestamp(unsigned long long ts) { return _ts(ts); }
        };
        inline int inner::http_request(char const* method, char const* uri,
                                       std::string const& querystring, std::string const& body, server_info const& si, std::string* resp, unsigned timeout_sec) {
            std::string header;
            struct iovec iv[2];
            struct sockaddr_in addr;
            int sock, ret_code = 0, content_length = 0, len = 0;
            char ch;
            unsigned char chunked = 0;

            addr.sin_family = AF_INET;
            addr.sin_port = htons(si.port_);
            if ((addr.sin_addr.s_addr = inet_addr(si.host_.c_str())) == INADDR_NONE) return -1;

            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -2;

            struct timeval timeout;
            timeout.tv_sec = static_cast<long>(timeout_sec);
            timeout.tv_usec = 0;
            if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout)) < 0 ||
                setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout)) < 0) return -2;

            if (connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
                closesocket(sock);
                return -3;
            }

            header.resize(len = 0x100);

            for (;;) {
                iv[0].iov_len = snprintf(&header[0], len,
                                         "%s /%s?db=%s%s%s%s%s%s%s%s HTTP/1.1\r\nHost: %s%s%s\r\nContent-Length: %d\r\n\r\n",
                                         method, uri, si.db_.c_str(), !si.token_.empty() ? "" : "&u=", !si.token_.empty() ? "" : si.usr_.c_str(), !si.token_.empty() ? "" : "&p=", !si.token_.empty() ? "" : si.pwd_.c_str(),
                                         strcmp(uri, "write") ? "&epoch=" : "&precision=", si.precision_.c_str(), querystring.c_str(), si.host_.c_str(), si.token_.empty() ? "" : "\r\nAuthorization: Token ", si.token_.c_str(), (int) body.length());
                if (static_cast<int>(iv[0].iov_len) >= len)
                    header.resize(len *= 2);
                else
                    break;
            }
            iv[0].iov_base = &header[0];
            iv[1].iov_base = const_cast<char*>(&body[0]);
            iv[1].iov_len = body.length();

            if (writev(sock, iv, 2) < static_cast<int>(iv[0].iov_len + iv[1].iov_len)) {
                ret_code = -6;
                goto END;
            }

            iv[0].iov_len = len;

#define _NO_MORE() (len >= static_cast<int>(iv[0].iov_len) && \
                    (iv[0].iov_len = recv(sock, &header[0], header.length(), len = 0)) == static_cast<size_t>(-1))
#define _GET_NEXT_CHAR() (ch = _NO_MORE() ? 0 : header[len++])
#define _LOOP_NEXT(statement)      \
    for (;;) {                     \
        if (!(_GET_NEXT_CHAR())) { \
            ret_code = -7;         \
            goto END;              \
        }                          \
        statement                  \
    }
#define _UNTIL(c) _LOOP_NEXT(if (ch == c) break;)
#define _GET_NUMBER(n) _LOOP_NEXT(if (ch >= '0' && ch <= '9') n = n * 10 + (ch - '0'); else break;)
#define _GET_CHUNKED_LEN(n, c) _LOOP_NEXT(if (ch >= '0' && ch <= '9') n = n * 16 + (ch - '0');           \
                                          else if (ch >= 'A' && ch <= 'F') n = n * 16 + (ch - 'A') + 10; \
                                          else if (ch >= 'a' && ch <= 'f') n = n * 16 + (ch - 'a') + 10; else {if(ch != c) { ret_code = -8; goto END; } break; })
#define _(c) \
    if ((_GET_NEXT_CHAR()) != c) break;
#define __(c)                      \
    if ((_GET_NEXT_CHAR()) != c) { \
        ret_code = -9;             \
        goto END;                  \
    }

            if (resp) resp->clear();

            _UNTIL(' ')
            _GET_NUMBER(ret_code)
            for (;;) {
                _UNTIL('\n')
                switch (_GET_NEXT_CHAR()) {
                    case 'C':
                        _('o')
                        _('n')
                        _('t')
                        _('e')
                        _('n')
                        _('t')
                        _('-')
                        _('L')
                        _('e')
                        _('n')
                        _('g')
                        _('t')
                        _('h')
                        _(':')
                        _(' ')
                        _GET_NUMBER(content_length)
                        break;
                    case 'T':
                        _('r')
                        _('a')
                        _('n')
                        _('s')
                        _('f')
                        _('e')
                        _('r')
                        _('-')
                        _('E')
                        _('n')
                        _('c')
                        _('o')
                        _('d')
                        _('i')
                        _('n')
                        _('g')
                        _(':')
                        _(' ')
                        _('c')
                        _('h')
                        _('u')
                        _('n')
                        _('k')
                        _('e')
                        _('d')
                        chunked = 1;
                        break;
                    case '\r':
                        __('\n')
                        switch (chunked) {
                            do {
                                __('\r')
                                __('\n')
                                case 1:
                                    _GET_CHUNKED_LEN(content_length, '\r')
                                    __('\n')
                                    if (!content_length) {
                                        __('\r')
                                        __('\n')
                                        goto END;
                                    }
                                case 0:
                                    while (content_length > 0 && !_NO_MORE()) {
                                        content_length -= (iv[1].iov_len = std::min(content_length, static_cast<int>(iv[0].iov_len) - len));
                                        if (resp) resp->append(&header[len], iv[1].iov_len);
                                        len += iv[1].iov_len;
                                    }
                            } while (chunked);
                        }
                        goto END;
                }
                if (!ch) {
                    ret_code = -10;
                    goto END;
                }
            }
            ret_code = -11;
        END:
            closesocket(sock);
            return ret_code / 100 == 2 ? 0 : ret_code;
#undef _NO_MORE
#undef _GET_NEXT_CHAR
#undef _LOOP_NEXT
#undef _UNTIL
#undef _GET_NUMBER
#undef _GET_CHUNKED_LEN
#undef _
#undef __
        }
    } // namespace detail
} // namespace influxdb_cpp

#endif // INFLUXDB_CPP_HPP
