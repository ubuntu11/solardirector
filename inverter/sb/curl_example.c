
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>


static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	char *p = userp;

	printf("contents: %p, size: %d, nmemb: %d, userp: %p\n", contents, (int)size, (int)nmemb, userp);
	strncat(p,(char *)contents, size * nmemb);
	return size * nmemb;
}

int main(void)
{
  CURL *curl;
  CURLcode res;
 	char readBuffer[1048576];

printf("rb: %p\n", readBuffer);
	readBuffer[0] = 0;
  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "http://www.google.com");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, readBuffer);
    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

	printf("buffer: %s\n", readBuffer);
  }
  return 0;
}
