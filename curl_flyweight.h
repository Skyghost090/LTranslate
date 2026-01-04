#include <curl/curl.h>
#include <curl/multi.h>

class curl_data{
    public:
        CURL *curl;
        CURLcode res;
};
