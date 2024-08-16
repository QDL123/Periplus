#ifndef HTTP_EXCEPTION_H
#define HTTP_EXCEPTION_H

#include <stdexcept>
#include <string>

class HttpException : public std::runtime_error {
public:
    HttpException(int status_code, const std::string& message)
        : std::runtime_error(message), status_code(status_code) {}

    int getStatusCode() const {
        return status_code;
    }

private:
    int status_code;
};

#endif // HTTP_EXCEPTION_H
