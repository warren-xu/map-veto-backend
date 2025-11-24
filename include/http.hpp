#pragma once

#include <string>

struct HttpRequest {
    std::string method;
    std::string path;   // path without query
    std::string query;  // query string (after ?)
};

// Decode URL-encoded strings (e.g. %20, + to space)
std::string url_decode(const std::string &s);

// Extract a query parameter by name from "k1=v1&k2=v2"
std::string get_query_param(const std::string &query, const std::string &key);

// Parse the first line of an HTTP request into method/path/query
bool parse_http_request(const std::string &raw, HttpRequest &out);

// Build a simple HTTP response with status, content-type, and body
std::string make_http_response(const std::string &body,
                               const std::string &contentType = "application/json",
                               int statusCode = 200,
                               const std::string &statusText = "OK");
